/*
 * Native definition support
 *
 * This file is part of System Informer.
 */

#ifndef _PHNT_NTDEF_H
#define _PHNT_NTDEF_H

// Mode
#define PHNT_MODE_KERNEL 0
#define PHNT_MODE_USER 1

// Version
#define PHNT_WIN2K 50
#define PHNT_WINXP 51
#define PHNT_WS03 52
#define PHNT_VISTA 60
#define PHNT_WIN7 61
#define PHNT_WIN8 62
#define PHNT_WINBLUE 63
#define PHNT_THRESHOLD 100
#define PHNT_THRESHOLD2 101
#define PHNT_REDSTONE 102
#define PHNT_REDSTONE2 103
#define PHNT_REDSTONE3 104
#define PHNT_REDSTONE4 105
#define PHNT_REDSTONE5 106
#define PHNT_19H1 107
#define PHNT_19H2 108
#define PHNT_20H1 109
#define PHNT_20H2 110
#define PHNT_21H1 111
#define PHNT_WIN10_21H2 112
#define PHNT_WIN10_22H2 113
#define PHNT_WIN11 114
#define PHNT_WIN11_22H2 115
#define PHNT_WIN11_23H2 116
#define PHNT_WIN11_24H2 117

#ifndef PHNT_MODE
#define PHNT_MODE PHNT_MODE_USER
#endif

#ifndef PHNT_VERSION
#define PHNT_VERSION PHNT_WIN11
#endif

// Options

//#define PHNT_NO_INLINE_INIT_STRING

#if (PHNT_MODE != PHNT_MODE_KERNEL)
#include "win-polyfill-arch.h"

#ifndef INT_ERROR
#define INT_ERROR (-1)
#endif

#ifndef ULONG64_MAX
#define ULONG64_MAX 0xffffffffffffffffui64
#endif

#ifndef SIZE_T_MAX
#ifdef _WIN64
#define SIZE_T_MAX 0xffffffffffffffffui64
#else
#define SIZE_T_MAX 0xffffffffUL
#endif
#endif

#ifndef ENABLE_RTL_NUMBER_OF_V2
#define ENABLE_RTL_NUMBER_OF_V2
#endif

#ifndef __cplusplus
// This is needed to workaround C17 preprocessor errors when using legacy versions of the Windows SDK. (dmex)
#ifndef MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS
#define MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS 0
#endif
#endif

#ifndef UMDF_USING_NTSTATUS
#define UMDF_USING_NTSTATUS
#endif
#include <ntstatus.h>

#include <windef.h>

typedef double DOUBLE;
typedef GUID *PGUID;

#ifndef __PCGUID_DEFINED__
#define __PCGUID_DEFINED__
typedef const GUID* PCGUID;
#endif

#ifndef _NTDEF_
#define _NTDEF_

// This header file provides basic NT types not included in Win32. If you have included winnt.h
// (perhaps indirectly), you must use this file instead of ntdef.h.

#ifndef NOTHING
#define NOTHING
#endif

// Basic types

typedef struct _QUAD
{
    union
    {
        __int64 UseThisFieldToCopy;
        double DoNotUseThisField;
    };
} QUAD, *PQUAD;

// This isn't in NT, but it's useful.
typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _QUAD_PTR
{
    ULONG_PTR DoNotUseThisField1;
    ULONG_PTR DoNotUseThisField2;
} QUAD_PTR, *PQUAD_PTR;

typedef ULONG LOGICAL;
typedef ULONG *PLOGICAL;

typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;

// Cardinal types

typedef char CCHAR;
typedef short CSHORT;
typedef ULONG CLONG;

typedef CCHAR *PCCHAR;
typedef CSHORT *PCSHORT;
typedef CLONG *PCLONG;

typedef PCSTR PCSZ;
typedef CONST WCHAR *LPCWCHAR, *PCWCHAR;

typedef PVOID* PPVOID;

// Specific

typedef UCHAR KIRQL, *PKIRQL;
typedef LONG KPRIORITY, *PKPRIORITY;
typedef USHORT RTL_ATOM, *PRTL_ATOM;

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

typedef struct _LARGE_INTEGER_128
{
    LONGLONG QuadPart[2];
} LARGE_INTEGER_128, *PLARGE_INTEGER_128;

// NT status macros

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define NT_INFORMATION(Status) ((((ULONG)(Status)) >> 30) == 1)
#define NT_WARNING(Status) ((((ULONG)(Status)) >> 30) == 2)
#define NT_ERROR(Status) ((((ULONG)(Status)) >> 30) == 3)

#define NT_CUSTOMER_SHIFT 29
#define NT_CUSTOMER(Status) ((((ULONG)(Status)) >> NT_CUSTOMER_SHIFT) & 1)

#define NT_FACILITY_MASK 0xfff
#define NT_FACILITY_SHIFT 16
#define NT_FACILITY(Status) ((((ULONG)(Status)) >> NT_FACILITY_SHIFT) & NT_FACILITY_MASK)

#define NT_NTWIN32(Status) (NT_FACILITY(Status) == FACILITY_NTWIN32)
#define WIN32_FROM_NTSTATUS(Status) (((ULONG)(Status)) & 0xffff)

// Functions

#ifndef _WIN64
#define FASTCALL __fastcall
#else
#define FASTCALL
#endif

// Synchronization enumerations

typedef enum _EVENT_TYPE
{
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE;

typedef enum _TIMER_TYPE
{
    NotificationTimer,
    SynchronizationTimer
} TIMER_TYPE;

typedef enum _WAIT_TYPE
{
    WaitAll,
    WaitAny,
    WaitNotification
} WAIT_TYPE;

// Strings

typedef struct _STRING
{
    USHORT Length;
    USHORT MaximumLength;
    _Field_size_bytes_part_opt_(MaximumLength, Length) PCHAR Buffer;
} STRING, *PSTRING, ANSI_STRING, *PANSI_STRING, OEM_STRING, *POEM_STRING;

typedef STRING UTF8_STRING;
typedef PSTRING PUTF8_STRING;

typedef const STRING *PCSTRING;
typedef const ANSI_STRING *PCANSI_STRING;
typedef const OEM_STRING *PCOEM_STRING;

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    _Field_size_bytes_part_opt_(MaximumLength, Length) PWCH Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef const UNICODE_STRING *PCUNICODE_STRING;

#define RTL_CONSTANT_STRING(s) { sizeof(s) - sizeof((s)[0]), sizeof(s), s }

#define DECLARE_CONST_UNICODE_STRING(_var, _str) \
const WCHAR _var ## _buffer[] = _str; \
const UNICODE_STRING _var = { sizeof(_str) - sizeof(WCHAR), sizeof(_str), (PWCH) _var ## _buffer }

#define DECLARE_GLOBAL_CONST_UNICODE_STRING(_var, _str) \
extern const DECLSPEC_SELECTANY UNICODE_STRING _var = RTL_CONSTANT_STRING(_str)

#define DECLARE_UNICODE_STRING_SIZE(_var, _size) \
WCHAR _var ## _buffer[_size]; \
UNICODE_STRING _var = { 0, (_size) * sizeof(WCHAR) , _var ## _buffer }

// Balanced tree node

#define RTL_BALANCED_NODE_RESERVED_PARENT_MASK 3

typedef struct _RTL_BALANCED_NODE
{
    union
    {
        struct _RTL_BALANCED_NODE *Children[2];
        struct
        {
            struct _RTL_BALANCED_NODE *Left;
            struct _RTL_BALANCED_NODE *Right;
        };
    };
    union
    {
        UCHAR Red : 1;
        UCHAR Balance : 2;
        ULONG_PTR ParentValue;
    };
} RTL_BALANCED_NODE, *PRTL_BALANCED_NODE;

#define RTL_BALANCED_NODE_GET_PARENT_POINTER(Node) \
    ((PRTL_BALANCED_NODE)((Node)->ParentValue & ~RTL_BALANCED_NODE_RESERVED_PARENT_MASK))

// Portability

typedef struct _SINGLE_LIST_ENTRY32
{
    ULONG Next;
} SINGLE_LIST_ENTRY32, *PSINGLE_LIST_ENTRY32;

typedef struct _STRING32
{
    USHORT Length;
    USHORT MaximumLength;
    ULONG Buffer;
} STRING32, *PSTRING32;

typedef STRING32 UNICODE_STRING32, *PUNICODE_STRING32;
typedef STRING32 ANSI_STRING32, *PANSI_STRING32;

typedef struct _STRING64
{
    USHORT Length;
    USHORT MaximumLength;
    ULONGLONG Buffer;
} STRING64, *PSTRING64;

typedef STRING64 UNICODE_STRING64, *PUNICODE_STRING64;
typedef STRING64 ANSI_STRING64, *PANSI_STRING64;

// Object attributes

#define OBJ_PROTECT_CLOSE                   0x00000001L
#define OBJ_INHERIT                         0x00000002L
#define OBJ_AUDIT_OBJECT_CLOSE              0x00000004L
#define OBJ_NO_RIGHTS_UPGRADE               0x00000008L
#define OBJ_PERMANENT                       0x00000010L
#define OBJ_EXCLUSIVE                       0x00000020L
#define OBJ_CASE_INSENSITIVE                0x00000040L
#define OBJ_OPENIF                          0x00000080L
#define OBJ_OPENLINK                        0x00000100L
#define OBJ_KERNEL_HANDLE                   0x00000200L
#define OBJ_FORCE_ACCESS_CHECK              0x00000400L
#define OBJ_IGNORE_IMPERSONATED_DEVICEMAP   0x00000800L
#define OBJ_DONT_REPARSE                    0x00001000L
#define OBJ_VALID_ATTRIBUTES                0x00001FF2L

typedef struct _OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor; // PSECURITY_DESCRIPTOR;
    PVOID SecurityQualityOfService; // PSECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef const OBJECT_ATTRIBUTES *PCOBJECT_ATTRIBUTES;

#define InitializeObjectAttributes(p, n, a, r, s) { \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
    (p)->RootDirectory = r; \
    (p)->Attributes = a; \
    (p)->ObjectName = n; \
    (p)->SecurityDescriptor = s; \
    (p)->SecurityQualityOfService = NULL; \
    }

#define RTL_CONSTANT_OBJECT_ATTRIBUTES(n, a) { sizeof(OBJECT_ATTRIBUTES), NULL, n, a, NULL, NULL }
#define RTL_INIT_OBJECT_ATTRIBUTES(n, a) RTL_CONSTANT_OBJECT_ATTRIBUTES(n, a)

#define OBJ_NAME_PATH_SEPARATOR ((WCHAR)L'\\')
#define OBJ_NAME_ALTPATH_SEPARATOR ((WCHAR)L'/')

// Portability

typedef struct _OBJECT_ATTRIBUTES64
{
    ULONG Length;
    ULONG64 RootDirectory;
    ULONG64 ObjectName;
    ULONG Attributes;
    ULONG64 SecurityDescriptor;
    ULONG64 SecurityQualityOfService;
} OBJECT_ATTRIBUTES64, *POBJECT_ATTRIBUTES64;

typedef const OBJECT_ATTRIBUTES64 *PCOBJECT_ATTRIBUTES64;

typedef struct _OBJECT_ATTRIBUTES32
{
    ULONG Length;
    ULONG RootDirectory;
    ULONG ObjectName;
    ULONG Attributes;
    ULONG SecurityDescriptor;
    ULONG SecurityQualityOfService;
} OBJECT_ATTRIBUTES32, *POBJECT_ATTRIBUTES32;

typedef const OBJECT_ATTRIBUTES32 *PCOBJECT_ATTRIBUTES32;

// Product types

typedef enum _NT_PRODUCT_TYPE
{
    NtProductWinNt = 1,
    NtProductLanManNt,
    NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;

typedef enum _SUITE_TYPE
{
    SmallBusiness,
    Enterprise,
    BackOffice,
    CommunicationServer,
    TerminalServer,
    SmallBusinessRestricted,
    EmbeddedNT,
    DataCenter,
    SingleUserTS,
    Personal,
    Blade,
    EmbeddedRestricted,
    SecurityAppliance,
    StorageServer,
    ComputeServer,
    WHServer,
    PhoneNT,
    MaxSuiteType
} SUITE_TYPE;

// Specific

typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _CLIENT_ID32
{
    ULONG UniqueProcess;
    ULONG UniqueThread;
} CLIENT_ID32, *PCLIENT_ID32;

typedef struct _CLIENT_ID64
{
    ULONGLONG UniqueProcess;
    ULONGLONG UniqueThread;
} CLIENT_ID64, *PCLIENT_ID64;

#include <pshpack4.h>

typedef struct _KSYSTEM_TIME
{
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

#include <poppack.h>

#ifndef AFFINITY_MASK
#define AFFINITY_MASK(n) ((KAFFINITY)1 << (n))
#endif

#ifndef FlagOn
#define FlagOn(_F, _SF) ((_F) & (_SF))
#endif
#ifndef BooleanFlagOn
#define BooleanFlagOn(F, SF) ((BOOLEAN)(((F) & (SF)) != 0))
#endif
#ifndef SetFlag
#define SetFlag(_F, _SF) ((_F) |= (_SF))
#endif
#ifndef ClearFlag
#define ClearFlag(_F, _SF) ((_F) &= ~(_SF))
#endif

#endif

#if defined(_WIN64)
#define POINTER_ALIGNMENT DECLSPEC_ALIGN(8)
#else
#define POINTER_ALIGNMENT
#endif

// Desktop access rights
#define DESKTOP_ALL_ACCESS \
    (DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | \
    DESKTOP_HOOKCONTROL | DESKTOP_JOURNALPLAYBACK | DESKTOP_JOURNALRECORD | \
    DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP | DESKTOP_WRITEOBJECTS | \
    STANDARD_RIGHTS_REQUIRED)
#define DESKTOP_GENERIC_READ \
    (DESKTOP_ENUMERATE | DESKTOP_READOBJECTS | STANDARD_RIGHTS_READ)
#define DESKTOP_GENERIC_WRITE \
    (DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_HOOKCONTROL | \
    DESKTOP_JOURNALPLAYBACK | DESKTOP_JOURNALRECORD | DESKTOP_WRITEOBJECTS | \
    STANDARD_RIGHTS_WRITE)
#define DESKTOP_GENERIC_EXECUTE \
    (DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_EXECUTE)

// Window station access rights
#define WINSTA_GENERIC_READ \
    (WINSTA_ENUMDESKTOPS | WINSTA_ENUMERATE | WINSTA_READATTRIBUTES | \
    WINSTA_READSCREEN | STANDARD_RIGHTS_READ)
#define WINSTA_GENERIC_WRITE \
    (WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | WINSTA_WRITEATTRIBUTES | \
    STANDARD_RIGHTS_WRITE)
#define WINSTA_GENERIC_EXECUTE \
    (WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS | STANDARD_RIGHTS_EXECUTE)

// WMI access rights
#define WMIGUID_GENERIC_READ \
    (WMIGUID_QUERY | WMIGUID_NOTIFICATION | WMIGUID_READ_DESCRIPTION | \
    STANDARD_RIGHTS_READ)
#define WMIGUID_GENERIC_WRITE \
    (WMIGUID_SET | TRACELOG_CREATE_REALTIME | TRACELOG_CREATE_ONDISK | \
    STANDARD_RIGHTS_WRITE)
#define WMIGUID_GENERIC_EXECUTE \
    (WMIGUID_EXECUTE | TRACELOG_GUID_ENABLE | TRACELOG_LOG_EVENT | \
    TRACELOG_ACCESS_REALTIME | TRACELOG_REGISTER_GUIDS | \
    STANDARD_RIGHTS_EXECUTE)

// Note: Some parts of the Windows Runtime, COM or third party hooks are returning
// S_FALSE and null pointers on errors when S_FALSE is a success code. (dmex)
#define HR_SUCCESS(hr) (((HRESULT)(hr)) == S_OK)
#define HR_FAILED(hr) (((HRESULT)(hr)) != S_OK)

// Note: The CONTAINING_RECORD macro doesn't support UBSan and generates false positives,
// we redefine the macro with FIELD_OFFSET as a workaround until the WinSDK is fixed (dmex)
#undef CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field) \
    ((type *)((ULONG_PTR)(address) - UFIELD_OFFSET(type, field)))

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#endif
