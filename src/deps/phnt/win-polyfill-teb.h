/*
 * Copyright 2024 Yonggang Luo
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "ntpebteb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PEB PEB;

// ==TEB==
// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/pebteb/teb/index.htm
// The Thread Environment Block is a thread’s user-mode representation. It has
// the highest-level knowledge of a thread in kernel mode and the lowest-level
// in user mode. If a thread has no user-mode footprint, it has no TEB. If only
// in principle, if anything about a thread is shared with kernel mode but can
// be properly managed in user mode without needing a transition to kernel mode,
// it goes in the TEB.
//
// That said, not all the TEB is about sharing with the kernel. Indeed, there is
// thankfully less and less of that. It’s one thing that the kernel creates the
// TEB and initialises many of the members. It’s quite another that the kernel
// ever uses anything from the TEB once it has been exposed to—and possibly
// changed by—anything in user mode.
//
//
// Access
//
// Code executing in user mode can easily find the TEB for the current thread.
// While a thread that has a TEB executes in user mode, the fs or gs register,
// for 32-bit and 64-bit code respectively, addresses this TEB.
//
// The TEB conveniently holds its own address in its NtTib.Self member. Reading
// just this one member using a segment-register override gets a linear address
// for override-free access to all the rest of the TEB. The 32-bit NTDLL even
// exports a function, named NtCurrentTeb and declared in WINNT.H, which does
// exactly this. For 64-bit Windows, NtCurrentTeb is a macro.
//
//
// Kernel Mode
//
// It is similarly easy for kernel-mode code to find the current thread’s TEB
// (if it has one). In kernel mode, the fs and gs registers, again for 32-bit
// and 64-bit code respectively, select a segment whose base address is that of
// a KPCR. Though this structure represents the current processor, not the
// current thread, it too has an NtTib member. Though it might be thought, aided
// by comments in NTDDK.H, that the Self member in this NtTib at the beginning
// of the processor’s KPCR would point back to that same NtTib, what it in fact
// points to is the NtTib at the beginning of the TEB for the processor’s
// current thread.
//
// This method of finding the TEB from kernel mode is reliable outside of the
// kernel’s own code for switching processors and threads. At every switch of a
// processor to another thread the kernel updates the processor’s KPCR so that
// its NtTib.Self points to the incoming thread’s TEB. Still, this way of
// finding the current TEB from kernel mode is surely not what Microsoft
// prefers. It seems to have been designed so that code that works with the TEB
// can work unchanged in both user mode and kernel mode. This applies most
// notably to library routines whose names start with Rtl. Many exist in, and
// some are exported by, both the kernel and NTDLL, and certainly are compiled
// from the same source code.
//
// Each KTHREAD, which is the kernel’s representation of a thread, has a Teb
// member which, of course, holds the address of the thread’s TEB (again, if the
// thread has one). This Teb pointer from the KTHREAD in kernel-mode address
// space to the TEB in user-mode address space is as old as Windows itself, but
// because it moves between versions it was originally not easily usable except
// by the kernel. This changed in version 5.1, with the introduction of an
// exported function, named PsGetThreadTeb, that looks up the Teb pointer for an
// arbitrary thread. Although a PsGetCurrentThreadTeb had to wait for the build
// of version 5.2 from Windows Server 2003 SP1, it too gets the TEB from the Teb
// in the current thread’s KTHREAD (but also checks that the thread is not a
// system thread and is not presently attached to the address space of another
// process).
//
//
// Mechanism
//
// As noted above, the updating of the pointer from a processor’s KPCR to its
// current thread’s TEB is done when the processor is switched to that thread
// (necessarily in kernel mode). This also is when the kernel prepares for
// eventual user-mode access via fs and gs. This is simpler in 32-bit Windows.
// Switching a processor to a thread that has a TEB changes the base address for
// the processor’s GDT selector 0x0038 to be that of the incoming thread’s TEB.
// The magic number 0x0038 is known symbolically as KGDT_R3_TEB. Its ring-3 form
// 0x003B is what the 32-bit kernel loads into fs when starting a thread’s
// user-mode execution and in many cases of returning a thread to user-mode
// execution. In other cases, the user-mode fs is simply saved on entry to
// kernel mode and restored on leaving. See that user-mode code can load fs with
// some other selector, and continue executing, but until it restores the
// expected selector in fs it can’t hope that many Windows API functions will
// get far without faulting.
//
// The mechanism is a little more complicated in 64-bit Windows not only because
// of the reduced meaningfulness of segment registers for the amd64 architecture
// but also because the TEB exists simultaneously in 32-bit and 64-bit forms.
// The 32-bit TEB follows the 64-bit after page alignment. Its address becomes
// the base address for the GDT selector 0x0050 (KGDT_R3_CMTEB) whose ring 3
// form (0x0053) the kernel loads into fs. The address of the 64-bit TEB is
// loaded into the Model Specific Register 0xC0000102 (which Microsoft labels
// MSR_GS_SWAP). This makes it the base address that the processor will use for
// interpreting gs after the kernel executes the swapgs instruction when exiting
// to user mode.
//
//
// Other Threads
//
// The TEB of any thread can be located via a handle with sufficient access
// rights. The gatekeeper is the NtQueryInformationThread function. This is
// exported by NTDLL in all known Windows versions (but is not a kernel-mode
// export before version 5.1). This function’s ThreadBasicInformation (0x00)
// case fills a THREAD_BASIC_INFORMATION structure whose member named
// TebBaseAddress is, unsurprisingly, the address of the queried thread’s TEB.
// Another of the function’s information classes, ThreadDescriptorTableEntry
// (0x06), underpins a long-documented API function that gives an alternative on
// 32-bit Windows: ask GetThreadSelectorEntry about the KGDT_R3_TEB selector.
//
// Of course, for a thread that is in another process, which is the most useful
// case, the address thus obtained is not directly usable. It is meaningful in
// the other process’s address space. Just reading from it then requires such
// functions as ReadProcessMemory and the corresponding permission. To do much
// with what’s read, even for a thread in the same process, may require
// synchronisation with or defence against changes being made by arbitrary other
// code—and writing to the queried thread’s TEB certainly requires such
// synchronisation. Safe use of another thread’s TEB is beyond many programers
// who attempt it, e.g., for malware and more notably for some of what gets
// foisted onto consumers as anti-malware or merely recommended to them as
// supposedly helpful system tools.
//
//
// Documentation Status
//
// In an ideal world, the TEB might be opaque, or even unknown, outside a
// handful of kernel-mode and user-mode modules that are obviously at the very
// heart of Windows. For many years, in terms of what Microsoft disclosed
// formally, this ideal was not far from achieved. The fs and gs registers are
// well-known as addressing an NT_TIB in user mode. This structure is defined in
// NTDDK.H and WINNT.H. A comment in NTDDK.H notes that the NT_TIB “appears as
// the first part of the TEB for all threads which have a user mode component”
// and WINNT.H either declares as a function or defines as a macro something
// named NtCurrentTeb which produces the address of the current thread’s TEB.
//
// In the real world, however, the TEB is not entirely opaque. Various
// high-level modules supplied with Windows over the years have used a few
// members of the TEB, and this eventually had to be disclosed. A new header,
// named WINTERNL.H, for previously internal APIs was added to the Software
// Development Kit (SDK) apparently in 2002 as the main (if insubstantial)
// outcome of an anti-trust settlement, and remains to this day. It originally
// presented a modified TEB that has just the TlsSlots, ReservedForOle and
// TlsExpansionSlots members, plus padding that gets these members to the same
// offsets as in the true structure. It seems unlikely that Microsoft will
// change the TEB in any way that moves these members.
//
// The three members defined in WINTERNL.H may be the only ones that have yet
// been disclosed for regulatory compliance, but others are known to
// Microsoft-written modules even as far out as Internet Explorer such that the
// use surely ought to have compelled disclosure. For instance, had technical
// advisers appointed by regulators actually been expert at this sort of work
// and thought to look, they would have needed mere seconds to establish that
// IEFRAME.DLL knows of the ProcessEnvironmentBlock member. That it got missed
// by Microsoft is plausibly because of an overlooked macro or inlined routine,
// but surely part of the point to having regulators monitor compliance is that
// they’re awake to deficiencies of method. Whether it was missed by oversight
// or not, it’s put right for WINTERNL.H in the SDK for Windows 8. Better late
// than never, perhaps.
//
// Other high-level knowledge, though not subject to disclosure, is clearly
// deliberate. For instance, the 32-bit SHELL32 and SHLWAPI know at least
// something of the special meaning that GdiBatchCount, almost at the end of the
// structure, has for 32-bit code running on 64-bit Windows.
//
//
// Layout
//
// Because very many modules written by Microsoft for the lower levels of the
// Win32 subsystem know of this or that in the TEB, it should not surprise that
// the TEB is highly stable across Windows versions. It must almost certainly
// stay so for many versions yet. Many TEB members, not only at the start but
// deep into the structure, and especially if they are known to the kernel, have
// kept their offsets through the whole history. Except for a large-scale
// reorganisation when version 4.0 moved much of the windowing subsystem from
// WINSRV.DLL in the CSRSS.EXE process to WIN32K.SYS in kernel mode, almost all
// variation of the TEB is by extension rather than redefinition. The following
// changes of size are known:
//
// | Version                   | Size (x86) | Size (x64) |
// |---------------------------|------------|------------|
// | 3.10                      | 0x0F20     |            |
// | 3.50 to 3.51              | 0x0F28     |            |
// | 4.0                       | 0x0F88     |            |
// | 5.0                       | 0x0FA4     |            |
// | early 5.1 (before SP2)    | 0x0FB4     |            |
// | late 5.1 (SP2 and higher) | 0x0FB8     |            |
// | early 5.2 (before SP1)    | 0x0FB8     |            |
// | late 5.2 (SP1 and higher) | 0x0FBC     | 0x17D8     |
// | 6.0                       | 0x0FF8     | 0x1828     |
// | 6.1                       | 0x0FE4     | 0x1818     |
// | 6.2 to 6.3                | 0x0FE8     | 0x1820     |
// | 10.0 to 2004              | 0x1000     | 0x1838     |
//
// These sizes, and the offsets, types and names in the tables that follow, are
// from Microsoft’s public symbols for the kernel and NTDLL starting with
// Windows XP, but are something of a guess for earlier versions since the
// symbol files for these do not contain type information for the TEB. What’s
// known of Microsoft’s names and types for earlier versions is instead inferred
// from what use is made of the TEB wherever matching code can be found.
// Exhaustively tracking down all such use would be difficult, if not
// impossible, even with source code.
//
// That said, Microsoft’s names for members in some versions before Windows XP
// are known with good confidence from the output of the debugger’s !dso
// command. This is supported by the USEREXTS.DLL and USERKDX.DLL debugger
// extensions as supplied with the Device Driver Kit (DDK), though only starting
// with the kit for Windows NT 4.0. The command “Dumps Struct field(s)’s
// offset(s) and value(s)” for roughly 200 structures, many of which are
// otherwise undocumented. The tables it works from give names and offsets, but
// not types, and although it seems likely that the tables do reproduce from
// Microsoft’s private headers for the same version, it is not obvious that they
// must. Even if macros ensure that the names in the table are the same as used
// for computing the offsets, so that these are all correct, it is not obvious
// that all members must be listed.
//
// It turns out that the relative uncertainty of debugger output is not the end
// of the archaeology. Type information for the TEB in versions 3.51 and 4.0
// turns up not in symbol files but in statically linked libraries. Even then,
// the libraries are not for importing from such obvious sources as NTDLL.
// Instead, type information for the TEB is in a GDISRVL.LIB from the DDK for
// Windows NT 3.51 and in the import library SHELL32.LIB from the DDK for
// Windows NT 4.0.
//
// Before version 3.51, even the structure’s size is uncertain. In all versions,
// the kernel obtains the TEB as whole pages, having rounded up from a size in
// bytes. A coding error in the earliest versions has this size in bytes as just
// 8, no matter that the kernel itself proceeds to use the Vdm member at offset
// 0x0F18. That the size is 0x0F20 in version 3.10 seems a good guess, however:
// a pointer at offset 0x0F1C is known to be used by RPCRT4.DLL and no use is
// known of anything beyond in version 3.10; the DebugSsReserved array that
// settles at offset 0x0F20 was relocated there for version 3.50, plausibly by
// moving it to what had been the structure’s end.
//
// Still, even with help from early debugger extensions and type information,
// the TEB is different enough in the first few versions that these notes would
// better be regarded as being only for version 4.0 and higher. Where I touch on
// the earlier versions, please take it not as an attempt at comprehensiveness
// but as a (messy) bonus from my being unable to resist looking into the
// archaeology.
//
//
// Original
//
// A run of members at the start of the structure are vital to the basic
// management of any thread. Given that they are shared not just between the
// kernel and NTDLL but with numerous other user-mode modules, their stability
// through the whole history should not surprise, and yet there have been small
// variations.
typedef struct _TEB
{
    // 0x00 0x00 (all)
    NT_TIB NtTib;
    // 0x1C 0x38 (all)
    PVOID EnvironmentPointer;
    // 0x20 0x40 (all)
    CLIENT_ID ClientId;
    union
    {
#ifndef _WIN64
        // 0x28 0x50 (3.10 only) (next as structure at 0x01AC)
        PVOID UnknownPointerCsrQlpcTeb;
#endif
        // 0x28 0x50 (3.50 and higher)
        PVOID ActiveRpcHandle;
    };
    // 0x2C 0x58 (all)
    PVOID ThreadLocalStoragePointer;
    // 0x30 0x60 (all)
    PEB *ProcessEnvironmentBlock;
    // 0x34 0x68 (all)
    ULONG LastErrorValue;
    union
    {
#ifndef _WIN64
        // 0x38 none (3.10 only)
        BYTE UnknowByte;
#endif
        // 0x38 0x6C (3.50 and higher)
        ULONG CountOfOwnedCriticalSections;
    };
    // The EnvironmentPointer is meaningful to OS2.EXE, i.e., the user-mode “OS2
    // Subsystem Client” such as distributed with Windows in versions 3.10 to 5.0
    // inclusive. What sort of environment it ever pointed to is beyond my
    // interest. No use of this member is known in any later version.
    //
    // The ClientId provides the simplest special case of per-thread storage. Each
    // thread in Windows has an identifier. A thread should be able to learn its
    // own identifier—and that of the process it runs in—almost trivially. The
    // documented functions GetCurrentProcessId and GetCurrentThreadId do nothing
    // but pick the respective identifiers from the ClientId.
    //
    // Version 3.10 holds at offset 0x28 the address of a CSR_QLPC_TEB structure.
    // In version 3.51, the structure is instead at offset 0x01AC in the TEB
    // itself. When a thread in some client process connects to the CSRSS server,
    // the client thread and a server thread each get this structure. Each has
    // handles to a shared section and an event pair, a pointer to a view of the
    // section, and the difference between the threads’ addresses for that view.
    // The handles and pointers in each are relative to the handle and address
    // spaces of the respective processes. The section and view are used for
    // passing requests and callbacks between the client and server. The delta
    // allows that the messages can contain pointers. The event pair provides
    // synchronisation. This was at the time important enough that not only is the
    // event pair is its own kernel object but the waiting and signalling had
    // dedicated interrupt numbers for getting to the kernel. For the server
    // thread, the structure begins with a pointer to the CSRSRV DLL’s
    // representation of the client thread. This pointer is all that survives to
    // version 4.0, when it becomes the CsrClientThread at offset 0x3C.
    //
    // No other use of the pointer at offset 0x28 is known in any version. Perhaps
    // it was named ActiveRpcHandle from the start, the name’s suggestion of a
    // Remote Procedure Call (RPC) having never been meant generally but always as
    // specific to the calls between a CSRDLL embedded in NTDLL in an arbitrary
    // client process and CSRSRV in the CSRSS server process.
    //
    // The thread-local storage that is provided through the
    // ThreadLocalStoragePointer has nothing to do with the API functions such as
    // TlsAlloc. Those are supported through the TlsSlots array and
    // TlsExpansionSlots pointer much further into the TEB and through such PEB
    // members as the TlsBitmap. The ThreadLocalStoragePointer instead deals with
    // the thread-local storage that may show in the Thread Local Storage
    // directory of the module’s Portable Executable (PE) header. Such storage
    // typically exists because the program has data that is defined with the
    // Microsoft-specific __declspec (thread) storage class modifier. Microsoft’s
    // compiler keeps all such data together in a section named .tls. A C Run-Time
    // (CRT) header defines an IMAGE_TLS_DIRECTORY to describe this data and its
    // special requirements. The linker makes this description available to the
    // loader from the PE header. What the ThreadLocalStoragePointer points to,
    // once it is set up, is an array of pointers to the thread-local data for
    // each module that has any. Or so it goes in theory. The practice was much
    // diminished in the early days, specifically before version 6.0, because this
    // form of thread-local storage was supported only for modules that are loaded
    // with the process, not for DLLs that are loaded later. A separate article
    // might well be written about that, not least because Microsoft, in best
    // Microsoft fashion, has tended to speak of it only obliquely, hinting for
    // instance that “you will not be able to load the DLL explicitly using
    // LoadLibrary on versions prior to Windows Vista” or that the feature “may
    // interfere with delay loading of DLL imports.”
    //
    // The LastErrorValue is ordinarily a hidden result of the thread’s most
    // recent call towards the system. The native API functions that NTDLL calls
    // in the kernel mostly return an NTSTATUS as their error code. This is also
    // true of most functions that NTDLL exports for the use of lower-level Win32
    // DLLs such as KERNEL32. The Win32 API has its own scheme of error codes, in
    // part inherited from DOS, and its own attitude to reporting them. Few Win32
    // API functions, such as implemented in KERNEL32, return their Win32 error
    // code directly. Most instead favour a direct return of successful results,
    // e.g., of a pointer or handle to a requested resource, or a boolean
    // indicator of success, and bundle all cases of failure into a return of NULL
    // or FALSE or some other distinctive value. The principle, it seems, is that
    // after a Win32 API function fails (and in some cases even if the function
    // succeeds), the calling thread that wants to distinguish causes of failure
    // can retrieve a Win32 error code by calling GetLastError. This documented
    // function merely fetches the error code from the LastErrorValue. The
    // relevance of the retrieved error code to any recently called API function
    // depends, of course, on the API function to have set an error code into
    // place in all cases of failure and on callers not to do anything that might
    // call some other API function between the setting and the retrieval.
    // Curiously many programmers, even ones who rate themselves as good or
    // experienced writers of elegant code, are willing to chance that the
    // LastErrorValue will remain untouched even while they call other code, e.g.,
    // functions in the C Run Time library.
    //
    // For some pupose that is presently unknown, the version 3.10 implementation
    // of SetLastError also clears the byte at offset 0x38. Though symbol files
    // for later versions name this space as CountOfOwnedCriticalSections, no use
    // of it for that purpose or any other is yet known.
    //
    // 0x3C 0x70 (3.10 and higher)
    union
    {
#ifndef _WIN64
        // 0x3C none (3.10 only)
        struct
        {
            UCHAR SpareBytes[0x88];
        } nt_3_10_p1;
        // 0x3C none (3.50 to 3.51)
        struct
        {
            // 0x3C none (3.50 to 3.51) (previously at 0x01B0)
            PVOID Win32ProcessInfo;
            // 0x40 none (3.50 to 3.51) (previously at 0x01AC)
            PVOID Win32ThreadInfo;
            // 0x44 none (3.50 to 3.51) (previously at 0x06F0)
            PVOID CsrQlpcStack;
            // 0x48 none (3.50 to 3.51) (previously at 0x01C0)
            UCHAR SpareBytes[0x7C];
        } nt_3_50_p1;
#endif
        // 0x3C 0x70 (4.0 and higher)
        struct
        {
            // 0x3C 0x70 (4.0 and higher)
            PVOID CsrClientThread;
            // 0x40 0x78 (4.0 and higher)
            PVOID Win32ThreadInfo;
            // 0x44 0x80 (4.0 and higher)
            union
            {
#ifndef _WIN64
                struct
                {
                    // 0x44 none (4.0 only)
                    ULONG Win32ClientInfo[0x1F];
                } nt_4_0_p1;
#endif
                // 0x44 0x80 (5.0 and higher)
                struct
                {
                    // 0x44 0x80 (5.0 and higher)
                    ULONG User32Reserved[26];
                    // 0xAC 0xE8 (5.0 and higher)
                    ULONG UserReserved[5];
                };
            };
            // 0xC0 0x0100 (4.0 and higher)
            PVOID WOW32Reserved;
        };
    };
    // What the Win32ProcessInfo and Win32ThreadInfo point to are respectively a
    // PROCESSINFO and a THREADINFO structure. In version 4.0 and higher,
    // WIN32K.SYS in kernel mode creates these in kernel-mode address space. Yes,
    // despite Microsoft’s attention in recent years to stanch the leaking of
    // kernel-mode addresses into user-mode space, the Win32ThreadInfo pointer
    // holds an undisguised kernel-mode address even as late as the original
    // release of Windows 10. Microsoft’s names for the PROCESSINFO and THREADINFO
    // and their members are publicly available as type information in the symbol
    // file for WIN32K in Windows 7 (but apparently neither before nor since).
    //
    // The CsrQlpcStack addresses a server thread’s view of the section that is
    // used for communicating with the client thread. How it differs from the
    // MessageStack pointer in the CSR_QLPC_TEB that the same versions have as the
    // CsrQlpcTeb at offset 0x01AC is not yet known.
    //
    // In version 4.0, the first 0x60 bytes of the 0x7C-byte Win32ClientInfo are
    // shared between WIN32K.SYS and USER32.DLL, certainly as a CLIENTINFO
    // structure. It is here supposed that User32Reserved as known in all later
    // symbol files preserves the size that the CLIENTINFO had grown to during
    // development for version 5.0 before its further growth (to 0x84 bytes)
    // required its relocation if CurrentLocale (immediately below) was not to be
    // disturbed. But who’s ever to know?

    // 0xC4 0x0108 (all)
    ULONG CurrentLocale;
    // 0xC8 0x010C (all)
    ULONG FpSoftwareStatusRegister;
    // The CurrentLocale is exactly what the documented functions GetThreadLocale
    // and SetThreadLocale get and set. See that for all the reworking of
    // preceding members in the earliest versions, someone seems to have been
    // determined that CurrentLocale keeps the same offset through the whole
    // history. Possibly what’s distinctive about it for this purpose is that it’s
    // known to the kernel, which sets it into place during the kernel-mode part
    // of the thread’s initial execution.
    //
    // Although the FpSoftwareStatusRegister is shown as having been there
    // forever, no use is yet known of it in any version. Conspicuously, it is not
    // used by the code that the kernel and NTDLL retained for floating-point
    // emulation up to and including Windows Vista (but which does use much of the
    // reserved area that follows).

    //
    //
    // System Reservation
    //
    // The first that names from symbol files show of a version dependence in
    // order of increasing offsets is that a relatively large reservation, for
    // system use according to the name, gets partly reassigned for Windows
    // 10—though only to change what it’s reserved for.
    //
    // 0xCC 0x0110 (3.10 and higher)
    union
    {
        // 0xCC 0x0110 (3.10 to 6.3)
        struct
        {
            PVOID SystemReserved1[0x36];
        } nt_3_10_p2;
        // 0xCC 0x0110 (10.0 and higher)
        struct
        {
            // 0xCC 0x0110 (10.0 and higher)
            PVOID ReservedForDebuggerInstrumentation[0x10];
            // 0x010C 0x0190 (1709 and higher)
#ifdef _WIN64
            PVOID SystemReserved1[25];
            PVOID HeapFlsData;
            ULONG_PTR RngState[4];
#else
            PVOID SystemReserved1[26];
#endif
            // 0x0174 0x0280 (1709 and higher)
            CHAR PlaceholderCompatibilityMode;
            // 0x0175 0x0281 (1809 and higher)
            BOOLEAN PlaceholderHydrationAlwaysExplicit;
            // 0x0176 0x0282 (1809 and higher)
            CHAR PlaceholderReserved[10];
            // 0x0180 0x028C (1709 and higher)
            DWORD ProxiedProcessId;
            // 0x0184 0x0290 (1703 and higher)
            ACTIVATION_CONTEXT_STACK ActivationStack;
            // 0x019C 0x02B8 (1607 and higher)
            UCHAR WorkingOnBehalfOfTicket[8];
        };
    };

    // ExceptionCode reference to
    // https://github.com/win-polyfill/win-polyfill-pebteb/wiki/TEB::ExceptionCode
    // 0x01A4 0x02C0 (all)
    union
    {
#ifndef _WIN64
        struct
        {
            // 0x01A4 none (3.10 to 4.0)
            PVOID Spare1;
        } nt_3_10_p3;
#endif
        // 0x01A4 0x02C0 (5.0 and higher)
        LONG ExceptionCode;
    };
#ifdef _WIN64
    // none 0x02C4 (6.3 and higher)
    UCHAR Padding0[4];
#endif
    // Before version 6.1, the preceding space is not reserved in any sense of
    // being kept for future use: the first 0xA0 bytes of it actually were in use,
    // by both the kernel and NTDLL, to support floating-point emulation. No
    // structure for this is known from symbol files. The known labelling as
    // SystemReserved1 even while the area was in use is here taken as original.
    //
    // According to type information from .LIB files in early DDKs, the reserved
    // space was anyway followed by two spares. The second of these was put to use
    // as the ExceptionCode when version 4.0 introduced the
    // KiRaiseUserExceptionDispatcher function. NTDLL exports this function not to
    // be imported by other user-mode modules but to be found by the kernel. The
    // kernel-mode KeRaiseUserException, also introduced with version 4.0, puts
    // the exception code into the TEB and then re-targets the kernel’s exit from
    // ring 0 so that whatever system service was in progress does not return as
    // expected but is instead picked up by KiRaiseUserExceptionDispatcher. This
    // stub then has NTDLL proceed as if for an exception raised in user mode by a
    // call to the NTDLL function RtlRaiseException. The original purpose of this
    // machinery was to help with debugging user-mode closure of handles. Hardly
    // any programs, mine included, check the success or failure of their calls to
    // such functions as CloseHandle—yes, even though failure can mean a loss of
    // data that had not yet been written to the file. If the 0x00400000 bit is
    // set in the NtGlobalFlag or if the current process is being debugged, then a
    // user-mode caller of NtClose who presents either an invalid handle or one
    // that is protected from being closed can learn of it from having to deal
    // with an exception. It is not known what version 5.0 gained from shifting
    // the ExceptionCode, except presumably to give up on whatever Spare1 had been
    // intended for.

    // ===Before WIN32K.SYS===
    // The first system reservation (and its two spares) was originally followed
    // by a highly variable region that seems better to present version by
    // version.
    //
    // Although the Win32ThreadInfo in version 3.10 is ordinarily a pointer, it
    // can also hold 1 or 2 as indications of progress during the user-mode
    // startup of the thread.
    //
    // The CsrQlpcTeb member actually is a CSR_QLPC_TEB structure, but apparently
    // everything about this except its size is meant to be a CSRSS implementation
    // detail. This is the same structure that version 3.10 allocates from the
    // heap and points to from offset 0x28. The CsrQlpcTeb and Win32ClientInfo are
    // together an exact fit for 0x28 bytes that version 3.10 has at offset 0x01B4
    // but is not known to use.
    //
    // The DbgSsReserved array supports the thread as a debugger of other
    // processes. The early implementation—before version 5.1 introduced a debug
    // object in kernel mode—used a port, named \DbgUiApiPort, that is created by
    // SMSS.EXE. The first of the handles is to a semaphore to wait on for
    // notification that data is available from the port, the second is to the
    // port. By version 3.51 the array moved to what was then the end of the
    // structure.
    //
    // ===Activation Context Stack==
    // With the relocation of windowing functionality from the user-mode CSRSS to
    // the kernel-mode WIN32K for version 4.0, the preceding members were either
    // discontinued or shifted forwards. The space they occupied became explicitly
    // spare (as SpareBytes1). When activation contexts were introduced for
    // Windows XP, some of the start of these spare bytes got used for an
    // ACTIVATION_CONTEXT_STACK structure. This soon changed, however. In the
    // Windows versions that have both x86 and x64 builds, the TEB has just a
    // pointer to an ACTIVATION_CONTEXT_STACK. At first, this just returned almost
    // all the bytes of the ActivationContextStack to being spare. Except that
    // Windows Vista defined a member at the end, these spare bytes remained spare
    // until Windows 10 put some to use for the instrumentation callback that can
    // be set through the ProcessInstrumentationCallback case of
    // NtSetInformationProcess.
    //
    // It may be just coincidence that the 64-bit TEB places the GdiTebBatch
    // member (see below) exactly as if there had been space for a whole
    // ACTIVATION_CONTEXT_STACK. However, all known x64 builds have just a pointer
    // to the ACTIVATION_CONTEXT_STACK and then spare bytes that get reduced and
    // renamed until all that’s left of them is padding caused by an alignment
    // requirement. Note that the members that are added for Windows 10 have a
    // slightly different order for the different processors. The x86 builds keep
    // all four members together, again carving them from the beginning of
    // previously spare bytes, and leaving some still spare. Space is tighter in
    // the x64 builds, such that the single-byte InstrumentationCallbackDisabled
    // only fits by squeezing into the alignment requirement after the older
    // TxFsContext.
    //
    // ===GDI Support===
    // Space in roughly the middle of the TEB, after what was originally a large
    // system reservation, seems to have been dedicated from the start to
    // supporting GDI32.
    //
    // What GdiThreadLocalInfo points to in version 3.10 is a 0x2C-byte structure
    // whose first member points to local thread information from the server. In
    // version 3.51, GdiThreadLocalInfo points directly to this same local thread
    // information. The information is also pointed to from offset 0x0C in the
    // 0x14-byte structure that version 3.10 points to from offset 0x28 and
    // version 3.51 embeds at offset 0x01AC. Yes, this was all very much in flux
    // back then! No use is known of the GdiThreadLocalInfo in version 4.0 and
    // higher.
    //
    // The collection of members at offset 0x06FC in versions 3.10 to 3.51 may be
    // an early form of CLIENTINFO. Certainly, the first of the reserved members
    // is a counter that USER32 and GDI32 use the same way as the cSpins member of
    // the CLIENTINFO in later versions. Version 3.51 has the second reserved
    // member point to a SERVERINFO.
    //
    // It seems at least plausible that the pointer at 0x0704 in version 3.51 was
    // defined also for version 3.10 but not yet used. In both versions, USER32
    // obtains the pointer from the server and both saves it to a global variable
    // and then passes it to a called routine. In version 3.51, this routine saves
    // the pointer in the TEB. Version 3.10 saves just to the global variable.
    //
    // The bytes leading up to but not including UserReserved are used by KERNEL32
    // in versions 3.10 to 5.0 as a safety stack for ExitThread on its way to
    // calling NtTerminateThread. See that this does not allow much safety when
    // version 5.0 moves UserReserved nearer to the front of the TEB. This move
    // seems to have happened as some sort of exchange with the (growing)
    // allowance for the Win32ClientInfo.
    //
    // ===Graphics Library===
    // The several members that follow, whose names all begin with gl, are
    // meaningful to OPENGL32.DLL and GLSRV.DLL. The oldest versions yet known to
    // me of either are from Windows NT 3.51, but I suspect that the many
    // functions that are supported by the glDispatchTable have an earlier
    // existence somewhere or were at least planned even if not yet implemented.
    //
    // The glDispatchTable in version 3.51 is filled to capacity with pointers to
    // functions, leaving no space for what is known to be later labelled
    // glReserved1. The pointer that is later labelled glReserved2 appears to be
    // unused in version 3.51, but presumably had no need of a numbered suffix. As
    // later versions introduced ever more functions to point to, the
    // glDispatchTable apparently could not be expanded. No later version fills
    // the reduced glDispatchTable.
    //
    // https://github.com/win-polyfill/win-polyfill-pebteb/wiki/TEB::UserReserved
    // https://github.com/win-polyfill/win-polyfill-pebteb/wiki/TEB::glDispatchTable
    // 0x01A8 0x02C0 (all)
    union
    {
#ifndef _WIN64
        // 0x01A8 none (3.10 only)
        struct
        {
            // 0x01A8 none (3.10 to 3.51)
            PVOID Spare2;
            // 0x01AC none (3.10 only) (next at 0x40)
            PVOID Win32ThreadInfo;
            // 0x01B0 none (3.10 only) (next at 0x3C)
            PVOID Win32ProcessInfo;
            // 0x01B4 none (3.10 only)
            BYTE UnaccountedBytes_0x01B4[0x28];
            // 0x01DC none (3.10 only) (next at 0x0F20)
            HANDLE DbgSsReserved[2];
            // 0x01E4 none (3.10 only)
            PVOID SystemReserved2[0x0143];
            // 0x06F0 none (3.10 only) (next at 0x44)
            PVOID CsrQlpcStack;
            // 0x06F4 none (3.10 to 4.0)
            ULONG GdiClientPID;
            // 0x06F8 none (3.10 to 4.0)
            ULONG GdiClientTID;
            // 0x06FC none (3.10 to 4.0)
            PVOID GdiThreadLocalInfo;
            // 0x0700 none (3.10 to 3.51)
            PVOID User32Reserved0;
            // 0x0704 none (3.10 to 3.51)
            PVOID User32Reserved1;
            // 0x0708 none (3.10 only)
            PVOID UserReserved[0x013B];
        } nt_3_10;
#endif
        // 0x01A8 0x02C0 (3.50 and higher)
        struct
        {
            // 0x01A8 0x02C0 (3.50 and higher)
            union
            {
#ifndef _WIN64
                // 0x01A8 none (3.50 to 3.51)
                struct
                {
                    // 0x01A8 none (3.10 to 3.51)
                    PVOID Spare2;
                    // 0x01AC none (3.50 to 3.51) (previously as pointer at 0x28)
                    PVOID CsrQlpcTeb[5];
                    // 0x01C0 none (3.50 to 3.51) (next at 0x44)
                    PVOID Win32ClientInfo[5];
                    // 0x01D4 none (3.50 to 3.51)
                    PVOID SystemReserved2[0x0142];
                    // 0x06DC none (3.50 to 4.0)
                    ULONG gdiRgn;
                    // 0x06E0 none (3.50 to 4.0)
                    ULONG gdiPen;
                    // 0x06E4 none (3.50 to 4.0)
                    ULONG gdiBrush;
                    // 0x06E8 none (3.50 to 4.0)
                    CLIENT_ID RealClientId;
                    // 0x06F0 none (3.50 to 4.0);
                    PVOID GdiCachedProcessHandle;
                    // 0x06F4 none (3.10 to 4.0)
                    ULONG GdiClientPID;
                    // 0x06F8 none (3.10 to 4.0)
                    ULONG GdiClientTID;
                    // 0x06FC none (3.10 to 4.0)
                    PVOID GdiThreadLocalInfo;
                    // 0x0700 none (3.10 to 3.51)
                    PVOID User32Reserved0;
                    // 0x0704 none (3.10 to 3.51)
                    PVOID User32Reserved1;
                    // 0x0708 none (3.50 to 3.51)
                    PVOID UserReserved[3];
                    // 0x0714 none (3.50 to 3.51)
                    PVOID glDispatchTable[0x0133];
                } nt_3_50_p2;
                // 0x01A8 none (4.0 only)
                struct
                {
                    // 0x01A8 none (4.0 only)
                    NTSTATUS ExceptionCode;
                    // 0x01AC none (4.0 only)
                    UCHAR SpareBytes1[0x28];
                    // 0x01D4 none (4.0 only)
                    PVOID SystemReserved2[0x0A];
                    // 0x01FC none (4.0 only)
                    GDI_TEB_BATCH GdiTebBatch;
                    // 0x06DC none (3.50 to 4.0)
                    ULONG gdiRgn;
                    // 0x06E0 none (3.50 to 4.0)
                    ULONG gdiPen;
                    // 0x06E4 none (3.50 to 4.0)
                    ULONG gdiBrush;
                    // 0x06E8 none (3.50 to 4.0)
                    CLIENT_ID RealClientId;
                    // 0x06F0 none (3.50 to 4.0)
                    PVOID GdiCachedProcessHandle;
                    // 0x06F4 none (3.10 to 4.0)
                    ULONG GdiClientPID;
                    // 0x06F8 none (3.10 to 4.0)
                    ULONG GdiClientTID;
                    // 0x06FC none (3.10 to 4.0)
                    PVOID GdiThreadLocalInfo;
                    // 0x0700 none (4.0 only)
                    PVOID UserReserved[5];
                    // 0x0714 none (4.0 only)
                    PVOID glDispatchTable[0x0118];
                    // 0x0B74 none (4.0 only)
                    ULONG glReserved1[0x1A];
                    // 0x0BDC none (4.0 and higher)
                    PVOID glReserved2;
                } nt_4_0_p2;
#endif
                // 0x01A8 0x02C8 (5.0 and higher)
                struct
                {
                    // 0x01A8 0x02C8 (5.0 and higher)
                    union
                    {
#ifndef _WIN64
                        // 0x01A8 none (5.0 to early 5.2)
                        struct
                        {
                            // 0x01A8 (5.0 to early 5.2)
                            UCHAR SpareBytes1[48 - sizeof(PVOID)];
                        };
#endif
                        // 0x01A8 0x02C8 (late 5.2 and higher)
                        struct
                        {
                            // 0x01A8 0x02C8 (late 5.2 and higher)
                            ACTIVATION_CONTEXT_STACK *ActivationContextStackPointer;
                            // 0x01AC 0x02D0 (10.0 and higher)
                            ULONG_PTR InstrumentationCallbackSp;
                            // 0x01B0 0x02D8 (10.0 and higher)
                            ULONG_PTR InstrumentationCallbackPreviousPc;
                            // 0x01B4 0x02E0 (10.0 and higher)
                            ULONG_PTR InstrumentationCallbackPreviousSp;
#ifdef _WIN64
                            // 0x01D0 0x02E8 (6.0 and higher)
                            ULONG TxFsContext;
#endif
                            // 0x01B8 0x02EC (10.0 and higher)
                            BOOLEAN InstrumentationCallbackDisabled;
#ifdef _WIN64
                            // none 0x02ED (1809 and higher)
                            BOOLEAN UnalignedLoadStoreExceptions;
                            // none 0x02EE (1809 and higher)
                            UCHAR Padding1[2];
#else
                            // 0x01B9 none (10.0 and higher)
                            UCHAR SpareBytes[0x17];
                            // 0x01D0 0x02E8 (6.0 and higher)
                            ULONG TxFsContext;
#endif
                        };
                    };
                    // 0x01D4 0x02F0 (5.0 and higher)
                    GDI_TEB_BATCH GdiTebBatch;
                    // 0x06B4 0x07D8 (5.0 and higher)
                    CLIENT_ID RealClientId;
                    // 0x06BC 0x07E8 (5.0 and higher)
                    PVOID GdiCachedProcessHandle;
                    // 0x06C0 0x07F0 (5.0 and higher)
                    ULONG GdiClientPID;
                    // 0x06C4 0x07F4 (5.0 and higher)
                    ULONG GdiClientTID;
                    // 0x06C8 0x07F8 (5.0 and higher)
                    PVOID GdiThreadLocalInfo;
                    // 0x06CC 0x0800 (5.0 and higher) (previously at 0x44)
                    ULONG_PTR Win32ClientInfo[WIN32_CLIENT_INFO_LENGTH];
                    // 0x07C4 0x09F0 (5.0 and higher)
                    PVOID glDispatchTable[0xE9];
                    // 0x0B68 0x1138 (5.0 and higher)
                    ULONG_PTR glReserved1[0x1D];
                    // 0x0BDC 0x1220 (4.0 and higher)
                    PVOID glReserved2;
                };
            };
            // 0x0BE0 0x1228 (3.50 and higher)
            PVOID glSectionInfo;
            // 0x0BE4 0x1230 (3.50 and higher)
            PVOID glSection;
            // 0x0BE8 0x1238 (3.50 and higher)
            PVOID glTable;
            // 0x0BEC 0x1240 (3.50 and higher)
            PVOID glCurrentRC;
            // 0x0BF0 0x1248 (3.50 and higher)
            PVOID glContext;
        };
    };

    //
    //
    // Stable At The End
    //
    // Far into the TEB is a sequence of members that (so far) retain their
    // positions all the way from Windows NT 3.1 to Windows 10.
    //
    // 0x0BF4 0x1250 (all)
    ULONG LastStatusValue;
#ifdef _WIN64
    // none 0x1254 (6.3 and higher)
    UCHAR Padding2[4];
#endif
    // 0x0BF8 0x1258 (all)
    UNICODE_STRING StaticUnicodeString;
    // 0x0C00 0x1268 (all)
    WCHAR StaticUnicodeBuffer[STATIC_UNICODE_BUFFER_LENGTH];
#ifdef _WIN64
    // none 0x1472 (6.3 and higher)
    UCHAR Padding3[6];
#endif
    // 0x0E0C 0x1478 (all)
    PVOID DeallocationStack;
    // 0x0E10 0x1480 (all)
    PVOID TlsSlots[TLS_MINIMUM_AVAILABLE];
    // 0x0F10 0x1680 (all)
    LIST_ENTRY TlsLinks;
    // 0x0F18 0x1690 (all)
    PVOID Vdm;
    // 0x0F1C 0x1698 (all) (last member in 3.10)
    PVOID ReservedForNtRpc;
    // The LastStatusValue is whatever was last given to the ancient
    // RtlNtStatusToDosError function. This is nowadays documented as a kernel
    // export. It is also an undocumented NTDLL export. The difference between
    // that function and RtlNtStatusDosErrorNoTeb, which is similarly ancient as a
    // kernel export and has always existed in NTDLL but is exported only in
    // version 5.1 and higher, is precisely that the latter does not affect the
    // LastStatusValue in the TEB. Both functions convert an NTSTATUS to a Win32
    // error code, but as a look-up only: the LastErrorValue is left alone. The
    // strong suggestion is that an NTSTATUS should not be looked up through plain
    // RtlNtStatusToDosError without the intention of proceeding to set a Win32
    // error code either from that NTSTATUS or from another. In version 5.1 and
    // higher, NTDLL exports another undocumented function, informatively named
    // RtlSetLastWin32ErrorAndNtStatusFromNtStatus, that goes all the way. It is
    // not known why saving an NTSTATUS as the LastStatusValue should change it
    // from signed to unsigned.
    //
    // The StaticUnicodeString and its Buffer seem to be provided as a convenience
    // for pretty much any API function that has temporary need of a
    // pathname-sized buffer.
    //
    // The TlsLinks member presumably is defined in all versions, but I don’t know
    // how it’s used in any version.
    //
    // The ReservedForNtRpc member is used by RPCRT4.DLL from the very beginning
    // to hold per-thread data. From inspecting RPCRT4 binaries for routines that
    // access this member and matching with symbol files for RPCRT4 to get the C++
    // decorated names of these routines, it can be known that RPCRT4 versions 4.0
    // through to 10.0 model this per-thread data as a class named THREAD.

    //
    //
    // Appended for Windows NT 3.50
    //
    // 0x0F20 0x16A0 (3.50 and higher)
    PVOID DbgSsReserved[2];
    // That DbgSsReserved is an array of two handles may be vestigial from the
    // original implementation in which a thread that becomes a debugger connects
    // to a named port that’s created by the SMSS process. In version 5.1 and
    // higher, the connection is instead to a kernel-mode debug object. A handle
    // to the debug object is kept as the array’s second element, but the first is
    // thought to be unused.

    //
    //
    // Appended for Windows NT 4.0
    //
    // 0x0F28 0x16B0 (4.0 and higher)
    union
    {
        // 4.0 to 5.1
        ULONG HardErrorsAreDisabled;
        // 5.2 and higher
        ULONG HardErrorMode;
    };
#ifdef _WIN64
    // none 0x16B4 (6.3 and higher)
    UCHAR Padding4[4];
#endif
    // 0x0F2C 0x16B8 (4.0 and higher)
    union
    {
#ifndef _WIN64
        // 0x0F2C 0x16B8 (4.0 to early 5.2)
        struct
        {
            PVOID Instrumentation[0x10];
        } nt_4_0_p3;
#endif
        struct
        {
            // 0x0F2C 0x16B8 (late 5.2 only)
            PVOID Instrumentation[0x0E];
            // 0x0F64 0x1728 (late 5.2 only)
            PVOID SubProcessTag;
            // 0x0F68 0x1730 (late 5.2 and higher)
            PVOID EtwTraceData;
        } nt_5_2_sp1_p1;
        // 0x0F2C 0x16B8 (6.0 and higher)
        struct
        {
#ifdef _WIN64
            // 0x0F2C 0x16B8 (6.0 and higher (x64))
            PVOID Instrumentation[0x0B];
#else
            // 0x0F2C 0x16B8 (6.0 and higher (x86))
            PVOID Instrumentation[0x09];
#endif
            // 0x0F50 0x1710 (6.0 and higher)
            GUID ActivityId;
            // 0x0F60 0x1720 (6.0 and higher)
            PVOID SubProcessTag;
            // 0x0F64 0x1728 (6.0 and higher)
            union
            {
                // 6.0 to 6.1
                PVOID EtwLocalData;
                // 6.2 and higher
                PVOID PerflibData;
            };
            // 0x0F68 0x1730 (late 5.2 and higher)
            PVOID EtwTraceData;
        };
    };
    // 0x0F6C 0x1738 (4.0 and higher)
    PVOID WinSockData;
    // 0x0F70 0x1740 (4.0 and higher)
    ULONG GdiBatchCount;
    // No use of the next four bytes is yet known in version 4.0. Version 5.0 uses
    // the first as a boolean, version 5.1 defines two more booleans and an 8-bit
    // processor number, version 6.0 returns the three booleans to being
    // explicitly spare, and then version 6.1 widens the processor number to all
    // four bytes.
    // 0x0F74 0x1744 (4.0 and higher)
    union
    {
        // 4.0 only
        ULONG Spare2;
        // 6.1 and higher
        PROCESSOR_NUMBER CurrentIdealProcessor;
        // 6.1 and higher
        ULONG IdealProcessorValue;
        // 5.0 and higher
        struct
        {
            // 5.0 to 5.2
            BOOLEAN InDbgPrint;
            // 5.1 to 5.2
            BOOLEAN FreeStackOnTermination;
            // 5.1 to 5.2
            BOOLEAN HasFiberData_5_1;
            // 6.1 and higher
            UCHAR IdealProcessor;
        };
    };
    // The booleans do not all disappear, however. The InDbgPrint member guards
    // the NTDLL function vDbgPrintExWithPrefix—and thus a family of functions
    // such as DbgPrint and DbgPrintEx—against non-trivial re-entry by the same
    // thread. Version 6.0 reworks this boolean as the DbgInDebugPrint bit field
    // in the SameTebFlags at offsets 0x0FCA and 0x17EE. A non-zero HasFiberData
    // records that the thread has been converted to a fiber and that the Version
    // member of the NtTib is instead the FiberData. It continues in version 6.0
    // as the DbgHasFiberData bit field, again in the SameTebFlags.
    //
    // The FreeStackOnTermination member does not continue to version 6.0. It was
    // anyway a relic of an ancient defence during thread termination. Before
    // version 5.1, if the ExitThread function is to exit from just the current
    // thread, it switches to a user-mode stack in the TEB itself, frees what
    // virtual memory had been in use for the stack, and proceeds to
    // NtTerminateThread. How useful this can have been is unclear. The kernel
    // seems unlikely to need much of a user-mode stack, but version 5.0 sets the
    // stack pointer strikingly low in the TEB, specifically to offset 0xAC.
    // Version 5.1 leaves the stack alone but instead sets FreeStackOnTermination
    // and the kernel then frees whatever virtual memory contains the user-mode
    // stack.
    //
    // The kernel sets the IdealProcessor in the TEB first while readying the
    // thread for its first user-mode execution and subsequently whenever its
    // ideal processor gets changed through the NtSetInformationThread cases
    // ThreadIdealProcessor (0x0D) or ThreadIdealProcessorEx (0x21). Note that the
    // kernel export KeSetIdealProcessorThread does not itself set IdealProcessor
    // in the TEB.
    //
    // The 4-byte union models accurately that what the kernel sets for the ideal
    // processor in version 6.1 and higher is something of a muddle. The kernel
    // does indeed set a PROCESSOR_NUMBER with a 16-bit Group and 8-bit Number as
    // the first three bytes, but where zero might be expected for Reserved as the
    // fourth byte, the kernel duplicates the Number. This has the merit of
    // keeping an 8-bit IdealProcessor at the same offset since version 5.1 and
    // higher. As late as version 10.0, NTDLL still has code that uses it there!
    // 0x0F78 0x1748 (4.0 and higher)
    union
    {
        // 4.0 to early 5.2
        ULONG Spare3;
        // late 5.2 and higher
        ULONG GuaranteedStackBytes;
    };
    // 0x0F7C 0x1750 (4.0 and higher)
    union
    {
        // 4.0 only
        ULONG Spare4;
        // 5.0 and higher
        PVOID ReservedForPerf;
    };
    // 0x0F80 0x1758 (4.0 and higher)
    PVOID ReservedForOle;
    // 0x0F84 0x1760 (4.0 and higher)
    ULONG WaitingOnLoaderLock;
#ifdef _WIN64
    // none 0x1764 (6.3 and higher)
    UCHAR Padding6[4];
#endif
    // The particular critical section that’s known as the NTDLL loader lock
    // causes no end of anxiety to some programmers but arguably not enough to
    // some others. As suggested by its name, WaitingOnLoaderLock is ordinarily
    // zero but is incremented while NTDLL doesn’t just “spin” for a contended
    // critical section but actually does wait and it turns out that the critical
    // section is the loader lock.

    //
    //
    // Appended for Windows 2000
    //
    // 0x0F88 0x1768 (5.0 and higher)
    union
    {
#ifndef _WIN64
        // 0x0F88 (5.0 to early 5.2)
        struct
        {
            ULONG *CallBx86Eip;
            PVOID DeallocationCpu;
            UCHAR UseKnownWx86Dll;
            CHAR OleStubInvoked;
        } Wx86Thread;
#endif
        // 0x0F88 0x1768 (late 5.2 and higher)
        struct
        {
            // 0x0F88 0x1768 (late 5.2 and higher)
            union
            {
                // late 5.2 only
                ULONG_PTR SparePointer1;
                // 6.0 and higher
                PVOID SavedPriorityState;
            };
            // 0x0F8C 0x1770 (late 5.2 and higher)
            union
            {
                // late 5.2 to 6.1
                ULONG_PTR SoftPatchPtr1;
                //6.2 and higher
                ULONG_PTR ReservedForCodeCoverage;
            };
            // 0x0F90 0x1778 (late 5.2 and higher)
            union
            {
                // late 5.2 only
                ULONG_PTR SoftPatchPtr2;
                // 6.0 and higher
                PVOID ThreadPoolData;
            };
        };
    };
    // No use is yet known of Wx86Thread. Whatever it was for, and whenever it was
    // first defined, it was anyway discarded when Windows got both 32-bit and
    // 64-bit builds.
    //
    // 0x0F94 0x1780 (5.0 and higher)
    PVOID *TlsExpansionSlots;
#ifdef _WIN64
    // none 0x1788 (late 5.2 and higher)
    union
    {
        PVOID DeallocationBStore;
        PVOID ChpeV2CpuAreaInfo;
    };
    // none 0x1790 (late 5.2 and higher)
    union
    {
        PVOID BStoreLimit;
        PVOID Unused;
    };
#endif
    // 0x0F98 0x1798 (5.0 and higher)
    union
    {
        // 5.0 to 6.0
        ULONG ImpersonationLocale;
        // 6.1 and higher
        ULONG MuiGeneration;
    };
    // 0x0F9C 0x179C (5.0 and higher)
    ULONG IsImpersonating;
    // 0x0FA0 0x17A0 (5.0 and higher)
    PVOID NlsCache;

    //
    //
    // Appended for Windows XP
    //
    // 0x0FA4 0x17A8 (5.1 and higher)
    PVOID pShimData;
    // 0x0FA8 0x17B0 (5.1 and higher)
    union
    {
        // 5.1 to 6.1
        ULONG HeapVirtualAffinity_5_1;
        // 6.2 to 1803
        struct
        {
            // 0x0FA8 0x17B0 (6.2 to 1803)
            USHORT HeapVirtualAffinity;
            // 0x0FAA 0x17B2 (6.2 to 1803)
            USHORT LowFragHeapDataSlot;
        };
        // 1809 and higher
        ULONG HeapData;
    };
#ifdef _WIN64
    // none 0x17B4 (6.3 and higher)
    UCHAR Padding7[4];
#endif
    // 0x0FAC 0x17B8 (5.1 and higher)
    HANDLE CurrentTransactionHandle;
    // 0x0FB0 0x17C0 (5.1 and higher) (last member in early 5.1)
    PTEB_ACTIVE_FRAME ActiveFrame;
    // 0x0FB4 0x17C8 (late 5.1 and higher)
    union
    {
#ifndef _WIN64
        // late 5.1 only
        struct
        {
            // 0x0FB4 none (late 5.1 only)
            BOOLEAN SafeThunkCall;
            BOOLEAN BooleanSpare[3];
        } nt_5_1_sp2;
#endif
        // 0x0FB4 0x17C8 (5.2 and higher)
        struct
        {
            // 0x0FB4 0x17C8 (5.2 and higher) (last member in early 5.2)
            PVOID FlsData;
            union
            {
                // 0x0FB8 0x17D0 (late 5.2 only) (last member in late 5.2)
                struct
                {
                    BOOLEAN SafeThunkCall;
                    BOOLEAN BooleanSpare[3];
                } nt_5_2_sp1_p2;
                // 0x0FB8 0x17D0 (6.0 and higher)
                PVOID PreferredLanguages;
            };
        };
    };

    //
    //
    // Appended for Windows Vista to Windows 10
    //
    // 0x0FBC 0x17D8 (6.0 and higher)
    PVOID UserPrefLanguages;
    PVOID MergedPrefLanguages;
    ULONG MuiImpersonation;
    union
    {
        USHORT CrossTebFlags;
        USHORT SpareCrossTebBits : 16;
    };
    union
    {
        USHORT SameTebFlags;
        struct
        {
            USHORT SafeThunkCall : 1;
            USHORT InDebugPrint : 1;
            USHORT HasFiberData : 1;
            USHORT SkipThreadAttach : 1;
            USHORT WerInShipAssertCode : 1;
            USHORT RanProcessInit : 1;
            USHORT ClonedThread : 1;
            USHORT SuppressDebugMsg : 1;
            USHORT DisableUserStackWalk : 1;
            USHORT RtlExceptionAttached : 1;
            USHORT InitialThread : 1;
            USHORT SessionAware : 1;
            USHORT LoadOwner : 1;
            USHORT LoaderWorker : 1;
            USHORT SkipLoaderInit : 1;
            USHORT SkipFileAPIBrokering : 1;
        };
    };
    PVOID TxnScopeEnterCallback;
    PVOID TxnScopeExitCallback;
    PVOID TxnScopeContext;
    // 0x0FD8 0x1808 (6.0 and higher)
    ULONG LockCount;
    // The remaining additions for Windows Vista were all discarded immediately
    // afterwards, such that the TEB shrinks for Windows 7.
    //
    // 0x0FDC 0x180C (6.0 and higher)
    union
    {
        // 6.0 only
        ULONG ProcessRundown;
        // 6.1 to 6.3
        ULONG SpareUlong0;
        // 10.0 and higher
        LONG WowTebOffset;
    };
    // The previous union is for make sure the following union is 8 byte aligned
    // 0x0FE0 0x1810 (6.0 and higher)
    union
    {
        // 0x0FE0 0x1810 (6.0 only)
        struct
        {
            // 0x0FE0 0x1810 (6.0 only)
            ULONGLONG LastSwitchTime;
            // 0x0FE8 0x1818 (6.0 only)
            ULONGLONG TotalSwitchOutTime;
            // 0x0FF0 0x1820 (6.0 only) (last member in 6.0)
            LARGE_INTEGER WaitReasonBitMap;
            // 0x0FF8 0x1828 (6.0 only) (for padding)
            PVOID PaddingVista;
        };
        // 0x0FE0 0x1810 (6.1 and higher)
        struct
        {
            // 0x0FE0 0x1810 (6.1 and higher) (last member in 6.1)
            PVOID ResourceRetValue;
            // 0x0FE4 0x1818 (6.2 and higher) (last member in 6.2/6.3)
            PVOID ReservedForWdf;
            // 0x0FE8 0x1820 (10.0 and higher)
            ULONGLONG ReservedForCrt;
            // 0x0FF0 0x1828 (10.0 and higher) (last member in 10.0)
            GUID EffectiveContainerId;
        };
    };

    // 0x1000 0x1838 (10.0.22000 and higher)
    ULONGLONG LastSleepCounter; // Win11
    ULONG SpinCallCount;
    ULONGLONG ExtendedFeatureDisableMask;
    PVOID SchedulerSharedDataSlot; // 24H2
    PVOID HeapWalkContext;
    GROUP_AFFINITY PrimaryGroupAffinity;
    ULONG Rcu[2];
} TEB, *PTEB;

#ifdef __cplusplus
}
#endif
