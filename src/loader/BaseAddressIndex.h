#pragma once
#include "phnt.h"

typedef struct _SEARCH_CONTEXT
{

    IN LPBYTE SearchPattern;
    IN SIZE_T PatternSize;

    OUT LPBYTE Result;
    SIZE_T MemoryBlockSize;

} SEARCH_CONTEXT, *PSEARCH_CONTEXT;

NTSTATUS NTAPI RtlFindMemoryBlockFromModuleSection(
    _In_ HMODULE ModuleHandle,
    _In_ LPCSTR SectionName,
    _Inout_ PSEARCH_CONTEXT SearchContext);
void BaseAddressIndexInit(PLDR_DATA_TABLE_ENTRY pNtdllEntry);

NTSTATUS NTAPI RtlInsertModuleBaseAddressIndexNode(
    _In_ PLDR_DATA_TABLE_ENTRY DataTableEntry,
    _In_ PVOID BaseAddress);

NTSTATUS NTAPI
RtlRemoveModuleBaseAddressIndexNode(_In_ PLDR_DATA_TABLE_ENTRY DataTableEntry);
