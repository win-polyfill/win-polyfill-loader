#pragma once

#include "phnt_ntdef.h"

// ==CURDIR==
// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/pebteb/curdir.htm?tx=179
// The CURDIR structure (formally _CURDIR) is a small structure that is known
// only within the RTL_USER_PROCESS_PARAMETERS. It there models its process’s
// notion of a current directory.
//
// The CURDIR is 0x0C and 0x18 bytes in 32-bit and 64-bit Windows, respectively.
//
// The Handle member records that Windows ordinarily keeps an open file handle
// to a process’s current directory. This is a useful optimisation for programs
// that are aware of having a current directory and may use relative pathnames
// when working with files. It can be problematic for programs that have no such
// awareness or do not provide easy means of changing the current directory. For
// instance, open a Command Prompt, create a directory, make it the current
// directory, run a simple program such as Notepad, change the Command Prompt to
// some other current directory and then try to remove the created directory:
//
// md path\test
// cd path\test
// notepad
// cd ..
// rd test
//
// You can expect to be told something like
// The process cannot access the file because it is being used by another process.
// In this case, Notepad has the created directory open as Notepad’s current directory. In real-world use of Windows it can be far from obvious what can be this “another process.”
typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    HANDLE Handle;
} CURDIR, *PCURDIR;

#define RTL_USER_PROC_CURDIR_CLOSE 0x00000002
#define RTL_USER_PROC_CURDIR_INHERIT 0x00000003

// ==RTL_DRIVE_LETTER_CURDIR==
// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/pebteb/rtl_drive_letter_curdir.htm
// The RTL_DRIVE_LETTER_CURDIR structure (formally _RTL_DRIVE_LETTER_CURDIR) is
// a small structure that is presently thought to be defined in all Windows
// versions but not used in any.
//
// From the name, the intended purpose is for keeping a current directory for
// each drive (letter). This is consistent with the two known references to this
// structure in definitions of other structures.
//
// The better known, e.g., from public symbol files for the kernel, is that the
// RTL_USER_PROCESS_PARAMETERS has an RTL_DRIVE_LETTER_CURDIR array as a member
// named CurrentDirectores. Yes, this misspelling is Microsoft’s and there are
// 32 in the array, not 26. Nesting even one RTL_DRIVE_LETTER_CURDIR in the
// RTL_USER_PROCESS_PARAMETERS means that the former’s definition must be
// available for the latter’s—and the latter is in turn well known for being
// pointed to from the PEB. Public symbols for the kernel, starting with Windows
// 8, confirm that the RTL_DRIVE_LETTER_CURDIR is defined in a header named
// pebteb.h. But no code is (yet) known that uses this array—not in the kernel
// or NTDLL or even further afield, in any version.
//
// Much less well known is that a structure named RTL_PERTHREAD_CURDIR has a
// member named CurrentDirectories which is defined as pointing to an
// RTL_DRIVE_LETTER_CURDIR (or, surely, to an array of them). All known versions
// of 32-bit Windows have code that prepares an RTL_PERTHREAD_CURDIR but no code
// is yet known that initialises the CurrentDirectories pointer to anything but
// NULL. All known versions of Windows, both 32-bit and 64-bit, have code that
// will read an RTL_PERTHREAD_CURDIR (from the SubSystemTib pointer in the
// NT_TIB at the start of a TEB), but no code is yet known that reads the
// CurrentDirectories.
//
// Thus is the RTL_DRIVE_LETTER_CURDIR thought to be defined in all Windows
// versions but not used in any. That a structure is still defined without
// having ever been used is certainly not impossible, but the waste of it
// suggests that more research is required.
//
//
// Layout
//
// The RTL_DRIVE_LETTER_CURDIR is 0x10 and 0x18 bytes in 32-bit and 64-bit
// Windows, respectively, in all known versions. The structure’s name and the
// names and definitions of its members are known from public symbol files for
// the kernel, starting from Windows 2000 SP3. That the definitions apply also
// to earlier versions is a guess based on nothing more than the apparent
// reservation of the right amount of space in the RTL_USER_PROCESS_PARAMETERS
// all the way back to version 3.10.
typedef struct _RTL_DRIVE_LETTER_CURDIR
{
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    STRING DosPath;
    // What any of these members mean—or were ever meant to mean—is not known, no
    // code having yet been found that accesses the structure whether in the
    // CurrentDirectores array or through the CurrentDirectories pointer.
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

#define RTL_MAX_DRIVE_LETTERS 32
#define RTL_DRIVE_LETTER_VALID (USHORT)0x0001

// ==RTL_USER_PROCESS_PARAMETERS==
// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/pebteb/rtl_user_process_parameters.htm?tx=179
// The RTL_USER_PROCESS_PARAMETERS structure (formally
// _RTL_USER_PROCESS_PARAMETERS) is the low-level packaging of the numerous
// arguments and parameters that can be specified to such Win32 API functions as
// CreateProcess.
//
// By the phrase “low-level packaging” I mean very deliberately that the
// structure is not specifically a packaging for the transition to and from
// kernel mode. Ask almost anyone in the last decade or more for a potted
// description of the RTL_USER_PROCESS_PARAMETERS, and you can expect to hear
// that these are parameters that are passed from user mode to kernel mode for
// handling the NtCreateUserProcess and ZwCreateUserProcess functions, and that
// the kernel creates a copy in the created process’s address space where it is
// then available in user mode through the created process’s PEB. This, however,
// is not the history. Even now, it is not the full story. The clue is in the
// name, specifically in the RTL prefix.
//
// For the first decade and more of Windows history, the
// RTL_USER_PROCESS_PARAMETERS structure does not cross between user and kernel
// modes as anything other than an uninterpreted blob that the creating process
// copies into the created process’s address space. The structure exists only as
// input to the RtlCreateUserProcess function. This Run Time Library (RTL)
// function is exported by name from NTDLL in all Windows versions, right from
// the beginning with version 3.10. It also exists in all versions of the
// kernel, but only as an internal routine which the kernel uses for starting
// the first user-mode process (and which is in a section that the kernel
// discards soon afterwards).
//
//
// Variability
//
// Presumably because of its role as input to an API function, albeit an
// undocumented one, the RTL_USER_PROCESS_PARAMETERS structure is strikingly
// stable. Indeed, while the RtlCreateUserProcess function was the only means of
// creating a user-mode process, its input structure did not change at all.
// Since then, the structure’s only variability is that new members are
// appended:
//
// | Version      | Size (x86) | Size (x64) |
// |--------------|------------|------------|
// | 3.10 to 5.2  | 0x0290     | 0x03F0     |
// | 6.0          | 0x0294     | 0x03F8     |
// | 6.1          | 0x0298     | 0x0400     |
// | 6.2 to 6.3   | 0x02A0     | 0x0410     |
// | 10.0 to 1803 | 0x02A4     | 0x0410     |
// | 1809         | 0x02AC     | 0x0420     |
// | 1903         | 0x02BC     | 0x0440     |
// | 2004         | 0x02C0     | 0x0440     |
//
//
// Layout
//
// These sizes, and the offsets, names and types of members in the table below,
// are from Microsoft’s public symbol files for the kernel starting with Windows
// 2000 SP3.
//
// For versions that predate the availability of type information in symbol
// files, continuity with the known definition for later versions is confirmed
// by inspection of the implementations and uses of RtlCreateUserProcess. For a
// handful of these early versions, the continuity can be established more
// readily. For instance, the import libraries GDISRVL.LIB and SHELL32.LIB that
// Microsoft supplied with the Device Driver Kit (DDK) for Windows NT 3.51 and
// 4.0, respectively, have an early form of type information, including for
// RTL_USER_PROCESS_PARAMETERS.
typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    // 0x00 0x00 (all)
    ULONG MaximumLength;
    // 0x04 0x04 (all)
    ULONG Length;
    // 0x08 0x08 (all)
    ULONG Flags;
    // 0x0C 0x0C (all)
    ULONG DebugFlags;
    // 0x10 0x10 (all)
    HANDLE ConsoleHandle;
    // 0x14 0x18 (all)
    ULONG ConsoleFlags;
    // 0x18 0x20 (all)
    HANDLE StandardInput;
    // 0x1C 0x28 (all)
    HANDLE StandardOutput;
    // 0x20 0x30 (all)
    HANDLE StandardError;
    // 0x24 0x38 (all)
    CURDIR CurrentDirectory;
    // 0x30 0x50 (all)
    UNICODE_STRING DllPath;
    // 0x38 0x60 (all)
    UNICODE_STRING ImagePathName;
    // 0x40 0x70 (all)
    UNICODE_STRING CommandLine;
    // 0x48 0x80 (all)
    PVOID Environment;
    // 0x4C 0x88 (all)
    ULONG StartingX;
    // 0x50 0x8C (all)
    ULONG StartingY;
    // 0x54 0x90 (all)
    ULONG CountX;
    // 0x58 0x94 (all)
    ULONG CountY;
    // 0x5C 0x98 (all)
    ULONG CountCharsX;
    // 0x60 0x9C (all)
    ULONG CountCharsY;
    // 0x64 0xA0 (all)
    ULONG FillAttribute;
    // 0x68 0xA4 (all)
    ULONG WindowFlags;
    // 0x6C 0xA8 (all)
    ULONG ShowWindowFlags;
    // 0x70 0xB0 (all)
    UNICODE_STRING WindowTitle;
    // 0x78 0xC0 (all)
    UNICODE_STRING DesktopInfo;
    // 0x80 0xD0 (all)
    UNICODE_STRING ShellInfo;
    // 0x88 0xE0 (all)
    UNICODE_STRING RuntimeData;
    // 0x90 0xF0 (all)
    RTL_DRIVE_LETTER_CURDIR CurrentDirectores[RTL_MAX_DRIVE_LETTERS];

    // 0x0290 0x03F0 (6.0 and higher)
    ULONG_PTR EnvironmentSize;
    // 0x0294 0x03F8 (6.1 and higher)
    ULONG_PTR EnvironmentVersion;
    // 0x0298 0x0400 (6.2 and higher)
    PVOID PackageDependencyData;
    // 0x029C 0x0408 (6.2 and higher)
    ULONG ProcessGroupId;
    // 0x02A0 0x040C (10.0 and higher)
    ULONG LoaderThreads;
    // 0x02A4 0x0410 (1809 and higher)
    UNICODE_STRING RedirectionDllName;
    // 0x02AC 0x0420 (1903 and higher)
    UNICODE_STRING HeapPartitionName;
    // 0x02B4 0x0430 (1903 and higher)
    ULONGLONG *DefaultThreadpoolCpuSetMasks;
    // 0x02B8 0x0438 (1903 and higher)
    ULONG DefaultThreadpoolCpuSetMaskCount;
    // 0x02BC 0x043C (2004 and higher)
    ULONG DefaultThreadpoolThreadMaximum;
    // 0x02C0 0x0440 (Win 11 and higher)
    ULONG HeapMemoryTypeMask;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;
