#pragma once

#include "phnt_ntdef.h"

// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/api/ntldr/ldr_data_table_entry.htm
typedef struct _LDR_DATA_TABLE_ENTRY
{
    // 0x00 0x00 (3.10 and higher)
    LIST_ENTRY InLoadOrderLinks;
    // 0x08 0x10 (3.10 and higher)
    LIST_ENTRY InMemoryOrderLinks;
    // 0x10 0x20 (3.10 and higher)
    union
    {
        // 0x10 0x20 (3.10 and higher)
        LIST_ENTRY InInitializationOrderLinks;
        // 0x10 0x20 (6.2 and higher)
        LIST_ENTRY InProgressLinks;
    };
    VOID *DllBase;                      //0x30
    VOID *EntryPoint;                   //0x38
    ULONG SizeOfImage;                  //0x40
    struct _UNICODE_STRING FullDllName; //0x48
    // 0x2C 0x58 (all)
    struct _UNICODE_STRING BaseDllName; //0x58
    // 0x34 0x68 (all)
    union
    {
        // 3.10 to 6.1
        ULONG Flags;
        // 6.2 and higher
        UCHAR FlagGroup[4];
        // 6.2 and higher
        struct
        {
            // 6.2 and higher
            // 0x00000001
            ULONG PackagedBinary : 1;

            // 3.51 to 6.1 LDRP_STATIC_LINK
            // 6.2 and higher LDRP_MARKED_FOR_REMOVAL
            // 0x00000002
            ULONG MarkedForRemoval : 1;

            // 3.51 and higher LDRP_IMAGE_DLL
            // 6.2 and higher
            // 0x00000004
            ULONG ImageDll : 1;

            //
            // 0x00000008
            ULONG LoadNotificationsSent : 1;
            ULONG TelemetryEntryProcessed : 1;
            ULONG ProcessStaticImport : 1;
            ULONG InLegacyLists : 1;
            ULONG InIndexes : 1;
            ULONG ShimDll : 1;
            ULONG InExceptionTable : 1;
            ULONG ReservedFlags1 : 2;
            ULONG LoadInProgress : 1;
            // 3.51 to 6.1 LDRP_UNLOAD_IN_PROGRESS
            // 6.2 to 6.3 ReservedFlags2
            // 10.0 and higher
            // 0x00002000
            ULONG LoadConfigProcessed : 1;
            // 3.51 and higher LDRP_ENTRY_PROCESSED
            // 6.2 and higher
            // 0x00004000
            ULONG EntryProcessed : 1;
            ULONG ProtectDelayLoad : 1;
            ULONG ReservedFlags3 : 2;
            ULONG DontCallForThreads : 1;
            // 3.51 and higher LDRP_PROCESS_ATTACH_CALLED
            // 6.2 and higher
            // 0x00080000
            ULONG ProcessAttachCalled : 1;
            ULONG ProcessAttachFailed : 1;
            // 5.1 and higher LDRP_IMAGE_NOT_AT_BASE
            // 6.2 and higher
            // 0x00200000
            ULONG CorDeferredValidate : 1;
            ULONG CorImage : 1;
            ULONG DontRelocate : 1;
            ULONG CorILOnly : 1;
            ULONG ChpeImage : 1;
            ULONG ChpeEmulatorImage : 1;
            ULONG ReservedFlags5 : 1;
            // 5.1 and higher use macro LDRP_REDIRECTED 0x10000000
            // 6.2 and higher 0x10000000
            ULONG Redirected : 1;
            ULONG ReservedFlags6 : 2;
            ULONG CompatDatabaseProcessed : 1;
        };
    };
    // 0x38 0x6C (all)
    union
    {
        // 3.10 to 6.1 (3.10 and higher)
        USHORT LoadCount;
        // 6.2 and higher
        USHORT ObsoleteLoadCount;
    };
    // 0x3A 0x6E (all)
    USHORT TlsIndex;
    // 0x3C 0x70 (all)
    LIST_ENTRY HashLinks;
    // 0x44 0x80 (4.0 and higher)
    ULONG TimeDateStamp;
    // 0x48 0x88 (5.1 and higher)
    struct _ACTIVATION_CONTEXT *EntryPointActivationContext;
    // 0x4C 0x90 (5.1 from Windows XP SP2 and higher)
    union
    {
        // 5.1 from Windows XP SP2 to 6.2
        PVOID PatchInformation;
        // 6.3 only
        PVOID Spare;
        // 10.0 and higher
        VOID *Lock;
    };
    union
    {
        struct
        {
            // 0x50 0x98 (6.0 to 6.1)
            LIST_ENTRY ForwarderLinks;
            // 0x58 0xA8 (6.0 to 6.1)
            LIST_ENTRY ServiceTagLinks;
            // 0x60 0xB8 (6.0 to 6.1)
            LIST_ENTRY StaticLinks;
            // 0x68 0xC8 (6.1 only)
            PVOID ContextInformation;
            // 0x6C 0xD0 (6.1 only)
            ULONG_PTR OriginalBase;
            // 0x70 0xD8 (6.1 only)
            LARGE_INTEGER LoadTime;
        } nt_6_0;
        struct
        {
            // 0x50 0x98 (6.2 and higher)
            struct _LDR_DDAG_NODE *DdagNode;
            LIST_ENTRY NodeModuleLink;
            struct _LDRP_LOAD_CONTEXT *LoadContext;
            VOID *ParentDllBase;
            VOID *SwitchBackContext;
            struct _RTL_BALANCED_NODE BaseAddressIndexNode;
            // 0x74 0xE0 (6.2 and higher)
            struct _RTL_BALANCED_NODE MappingInfoIndexNode;
            // 0x80 0xF8 (6.2 and higher)
            ULONGLONG OriginalBase;
            // 0x88 0x0100 (6.2 and higher)
            union _LARGE_INTEGER LoadTime;
        };
    };

    // 0x90 0x0108 (6.2 and higher)
    ULONG BaseNameHashValue;
    // 0x94 0x010C (6.2 and higher)
    enum _LDR_DLL_LOAD_REASON LoadReason;
    // 0x98 0x0110 (6.3 and higher)
    ULONG ImplicitPathOptions;
    // 0x9C 0x0114 (10.0 and higher)
    ULONG ReferenceCount;
    // 0xA0 0x0118 (10.0 1607 and higher)
    ULONG DependentLoadFlags; //0x118
    // 0xA4 0x011C (10.0 1703 and higher)
    UCHAR SigningLevel;
    ULONG CheckSum; //0x120
    // since 22H1
    VOID *ActivePatchImageBase;              //0x128
    enum _LDR_HOT_PATCH_STATE HotPatchState; //0x130
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;
