#include "phnt.h"

#define EH_NONCONTINUABLE 0x01
#define EH_UNWINDING 0x02
#define EH_EXIT_UNWIND 0x04
#define EH_STACK_INVALID 0x08
#define EH_NESTED_CALL 0x10
#define EH_EXCEPTION_NUMBER ('msc' | 0xE0000000)

extern "C" {

#if _M_IX86

typedef struct _SCOPETABLE
{
    int previousTryLevel;
    int (*lpfnFilter)(PEXCEPTION_POINTERS);
    int (*lpfnHandler)(void);
} SCOPETABLE, *PSCOPETABLE;
struct _EXCEPTION_FRAME;

typedef EXCEPTION_DISPOSITION (*PEXCEPTION_HANDLER)(
    struct _EXCEPTION_RECORD *ExceptionRecord,
    struct _EXCEPTION_FRAME *EstablisherFrame,
    struct _CONTEXT *ContextRecord,
    struct _EXCEPTION_FRAME **DispatcherContext);

typedef struct _EXCEPTION_FRAME
{
    struct _EXCEPTION_FRAME *prev;
    PEXCEPTION_HANDLER handler;
} EXCEPTION_FRAME, *PEXCEPTION_FRAME;

typedef struct _NESTED_FRAME
{
    EXCEPTION_FRAME frame;
    EXCEPTION_FRAME *prev;
} NESTED_FRAME;

typedef struct _MSVCRT_EXCEPTION_FRAME
{
    EXCEPTION_FRAME *prev;
    void (*handler)(PEXCEPTION_RECORD, PEXCEPTION_FRAME, PCONTEXT, PEXCEPTION_RECORD);
    PSCOPETABLE scopetable;
    int trylevel;
    int _ebp;
    PEXCEPTION_POINTERS xpointers;
} MSVCRT_EXCEPTION_FRAME;

#define TRYLEVEL_END (-1) // End of trylevel list

static __inline EXCEPTION_FRAME *push_frame(EXCEPTION_FRAME *frame)
{
    auto tib = &NtCurrentTeb()->NtTib;
    frame->prev = (EXCEPTION_FRAME *)(tib->ExceptionList);
    tib->ExceptionList = (EXCEPTION_REGISTRATION_RECORD *)frame;
    return frame->prev;
}

static __inline EXCEPTION_FRAME *pop_frame(EXCEPTION_FRAME *frame)
{
    auto tib = &NtCurrentTeb()->NtTib;
    tib->ExceptionList = (EXCEPTION_REGISTRATION_RECORD *)frame->prev;
    return frame->prev;
}

#pragma warning(                                                                         \
    disable : 4731) // C4731: frame pointer register 'ebp' modified by inline assembly code

static void call_finally_block(void *code_block, void *base_ptr)
{
    __asm {
   mov eax, [code_block]
   mov ebp, [base_ptr]
   call eax
    }
}

static DWORD call_filter(void *func, void *arg, void *base_ptr)
{
    DWORD rc;

    __asm {
    push ebp
    push [arg]
    mov eax, [func]
    mov ebp, [base_ptr]
    call eax
    pop ebp
    pop ebp
    mov [rc], eax
    }

    return rc;
}

static EXCEPTION_DISPOSITION msvcrt_nested_handler(
    EXCEPTION_RECORD *rec,
    EXCEPTION_FRAME *frame,
    CONTEXT *ctxt,
    EXCEPTION_FRAME **dispatcher)
{
    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
        return ExceptionContinueSearch;
    *dispatcher = frame;
    return ExceptionCollidedUnwind;
}

void _global_unwind2(PEXCEPTION_FRAME frame) { RtlUnwind(frame, 0, 0, 0); }

void _local_unwind2(MSVCRT_EXCEPTION_FRAME *frame, int trylevel)
{
    MSVCRT_EXCEPTION_FRAME *curframe = frame;
    EXCEPTION_FRAME reg;

    //syslog(LOG_DEBUG, "_local_unwind2(%p,%d,%d)",frame, frame->trylevel, trylevel);

    // Register a handler in case of a nested exception
    reg.handler = (PEXCEPTION_HANDLER)msvcrt_nested_handler;
    auto tib = &NtCurrentTeb()->NtTib;
    reg.prev = (PEXCEPTION_FRAME)tib->ExceptionList;
    push_frame(&reg);

    while (frame->trylevel != TRYLEVEL_END && frame->trylevel != trylevel)
    {
        int curtrylevel = frame->scopetable[frame->trylevel].previousTryLevel;
        curframe = frame;
        curframe->trylevel = curtrylevel;
        if (!frame->scopetable[curtrylevel].lpfnFilter)
        {
            // TODO: Remove current frame, set ebp, call frame->scopetable[curtrylevel].lpfnHandler()
        }
    }
    pop_frame(&reg);
}

// from https://github.com/ringgaard/sanos/blob/b6ea5cb56f9f6b76c7664865067ac812a7efdc96/src/win32/msvcrt/except.c
int _except_handler3(
    PEXCEPTION_RECORD rec,
    MSVCRT_EXCEPTION_FRAME *frame,
    PCONTEXT context,
    void *dispatcher)
{
    long retval;
    int trylevel;
    EXCEPTION_POINTERS exceptPtrs;
    PSCOPETABLE pScopeTable;

    //syslog(LOG_DEBUG, "msvcrt: exception %lx flags=%lx at %p handler=%p %p %p semi-stub",
    //       rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
    //       frame->handler, context, dispatcher);

    __asm cld;

    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        // Unwinding the current frame
        _local_unwind2(frame, TRYLEVEL_END);
        return ExceptionContinueSearch;
    }
    else
    {
        // Hunting for handler
        exceptPtrs.ExceptionRecord = rec;
        exceptPtrs.ContextRecord = context;
        *((DWORD *)frame - 1) = (DWORD)&exceptPtrs;
        trylevel = frame->trylevel;
        pScopeTable = frame->scopetable;

        while (trylevel != TRYLEVEL_END)
        {
            if (pScopeTable[trylevel].lpfnFilter)
            {
                //syslog(LOG_DEBUG, "filter = %p", pScopeTable[trylevel].lpfnFilter);

                retval = call_filter(
                    pScopeTable[trylevel].lpfnFilter, &exceptPtrs, &frame->_ebp);

                //syslog(LOG_DEBUG, "filter returned %s", retval == EXCEPTION_CONTINUE_EXECUTION ?
                //      "CONTINUE_EXECUTION" : retval == EXCEPTION_EXECUTE_HANDLER ?
                //      "EXECUTE_HANDLER" : "CONTINUE_SEARCH");

                if (retval == EXCEPTION_CONTINUE_EXECUTION)
                    return ExceptionContinueExecution;

                if (retval == EXCEPTION_EXECUTE_HANDLER)
                {
                    // Unwind all higher frames, this one will handle the exception
                    _global_unwind2((PEXCEPTION_FRAME)frame);
                    _local_unwind2(frame, trylevel);

                    // Set our trylevel to the enclosing block, and call the __finally code, which won't return
                    frame->trylevel = pScopeTable->previousTryLevel;
                    //syslog(LOG_DEBUG, "__finally block %p",pScopeTable[trylevel].lpfnHandler);
                    call_finally_block(pScopeTable[trylevel].lpfnHandler, &frame->_ebp);
                }
            }
            trylevel = pScopeTable->previousTryLevel;
        }
    }

    return ExceptionContinueSearch;
}

int __cdecl _except_handler3_noexcept(
    PEXCEPTION_RECORD ExceptionRecord,
    MSVCRT_EXCEPTION_FRAME *frame,
    PCONTEXT context,
    void *dispatcher)
{
    int Excption = _except_handler3(ExceptionRecord, frame, context, dispatcher);
    if (IS_DISPATCHING(ExceptionRecord->ExceptionFlags) &&
        ExceptionRecord->ExceptionCode == EH_EXCEPTION_NUMBER &&
        Excption == ExceptionContinueSearch)
    {
        RtlExitUserProcess(-1);
    }
    return Excption;
}

typedef struct EHExceptionRecord EHExceptionRecord;
typedef struct EHRegistrationNode EHRegistrationNode;
typedef void DispatcherContext; // Meaningless on x86

void *__CxxFrameHandler3(
    EHExceptionRecord *pExcept,
    EHRegistrationNode *pRN,
    void *pContext,
    DispatcherContext *pDC)
{
    return nullptr;
}

UINT_PTR __security_cookie = 0xBB40E64E;

extern PVOID __safe_se_handler_table[];
extern BYTE __safe_se_handler_count;

typedef struct
{
    DWORD Size;
    DWORD TimeDateStamp;
    WORD MajorVersion;
    WORD MinorVersion;
    DWORD GlobalFlagsClear;
    DWORD GlobalFlagsSet;
    DWORD CriticalSectionDefaultTimeout;
    DWORD DeCommitFreeBlockThreshold;
    DWORD DeCommitTotalFreeThreshold;
    DWORD LockPrefixTable;
    DWORD MaximumAllocationSize;
    DWORD VirtualMemoryThreshold;
    DWORD ProcessHeapFlags;
    DWORD ProcessAffinityMask;
    WORD CSDVersion;
    WORD Reserved1;
    DWORD EditList;
    PUINT_PTR SecurityCookie;
    PVOID *SEHandlerTable;
    DWORD SEHandlerCount;
} IMAGE_LOAD_CONFIG_DIRECTORY32_2;

const IMAGE_LOAD_CONFIG_DIRECTORY32_2 _load_config_used = {
    sizeof(IMAGE_LOAD_CONFIG_DIRECTORY32_2),
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    &__security_cookie,
    __safe_se_handler_table,
    (DWORD)(DWORD_PTR)&__safe_se_handler_count};

#elif _M_AMD64

EXCEPTION_DISPOSITION __CxxFrameHandler4(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PVOID EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext)
{
    return ExceptionContinueExecution;
}

EXCEPTION_DISPOSITION
__C_specific_handler(
    struct _EXCEPTION_RECORD *ExceptionRecord,
    void *EstablisherFrame,
    struct _CONTEXT *ContextRecord,
    struct _DISPATCHER_CONTEXT *DispatcherContext)
{
    typedef EXCEPTION_DISPOSITION Function(
        struct _EXCEPTION_RECORD *, void *, struct _CONTEXT *, _DISPATCHER_CONTEXT *);
    static Function *FunctionPtr;

    if (!FunctionPtr)
    {
        HMODULE Library = LoadLibraryA("msvcrt.dll");
        FunctionPtr = (Function *)GetProcAddress(Library, "__C_specific_handler");
    }

    return FunctionPtr(
        ExceptionRecord, EstablisherFrame, ContextRecord, DispatcherContext);
}

#endif

//
// Run-Time-Checks
//
void __cdecl _RTC_UninitUse(const char *varname) {}

void __cdecl _RTC_InitBase(void) {}
void __cdecl _RTC_Shutdown(void) {}
void __cdecl _RTC_CheckEsp(void) {}
void __fastcall _RTC_CheckStackVars(void *_Esp, struct _RTC_framedesc *_Fd) {}

} // extern "C"
