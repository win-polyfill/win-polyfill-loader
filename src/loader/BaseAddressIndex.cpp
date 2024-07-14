#include "BaseAddressIndex.h"
#include "LdrEntry.h"
#include "win-polyfill-string.h"
#include "win-polyfill-version.h"


static PRTL_RB_TREE LdrpModuleBaseAddressIndex;
static DWORD WindowsVersion;

NTSTATUS NTAPI RtlFindMemoryBlockFromModuleSection(
	_In_ HMODULE ModuleHandle,
	_In_ LPCSTR SectionName,
	_Inout_ PSEARCH_CONTEXT SearchContext)
{

	NTSTATUS status = STATUS_SUCCESS;

	__try
	{

		//
		// checks if no search pattern and length are provided
		//

		if (!SearchContext->SearchPattern || !SearchContext->PatternSize)
		{
			SearchContext->Result = nullptr;
			SearchContext->MemoryBlockSize = 0;

			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		if (SearchContext->Result)
		{
			++SearchContext->Result;
			--SearchContext->MemoryBlockSize;
		}
		else
		{

			//
			// if it is the first search, find the length and start address of the specified section
			//

			PIMAGE_NT_HEADERS headers = RtlImageNtHeader(ModuleHandle);
			PIMAGE_SECTION_HEADER section = nullptr;

			if (headers)
			{
				section = IMAGE_FIRST_SECTION(headers);
				for (WORD i = 0; i < headers->FileHeader.NumberOfSections; ++i)
				{
					auto stringA = MakeNtString(SectionName);
					auto stringB = MakeNtString((LPCSTR)section->Name);
					if (RtlCompareString(&stringA, &stringB, TRUE) == 0)
					{
						SearchContext->Result =
							(LPBYTE)ModuleHandle + section->VirtualAddress;
						SearchContext->MemoryBlockSize = section->Misc.VirtualSize;
						break;
					}

					++section;
				}

				if (!SearchContext->Result || !SearchContext->MemoryBlockSize ||
					SearchContext->MemoryBlockSize < SearchContext->PatternSize)
				{
					SearchContext->Result = nullptr;
					SearchContext->MemoryBlockSize = 0;
					status = STATUS_NOT_FOUND;
					__leave;
				}
			}
			else
			{
				status = STATUS_INVALID_PARAMETER_1;
				__leave;
			}
		}

		//
		// perform a linear search on the pattern
		//

		LPBYTE end = SearchContext->Result + SearchContext->MemoryBlockSize -
					 SearchContext->PatternSize;
		while (SearchContext->Result <= end)
		{
			if (RtlCompareMemory(
					SearchContext->SearchPattern,
					SearchContext->Result,
					SearchContext->PatternSize) == SearchContext->PatternSize)
			{
				__leave;
			}

			++SearchContext->Result;
			--SearchContext->MemoryBlockSize;
		}

		//
		// if the search fails, clear the output parameters
		//

		SearchContext->Result = nullptr;
		SearchContext->MemoryBlockSize = 0;
		status = STATUS_NOT_FOUND;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		status = GetExceptionCode();
	}

	return status;
}

void FindLdrpModuleBaseAddressIndex(PLDR_DATA_TABLE_ENTRY_WIN10 nt10)
{
	PRTL_BALANCED_NODE node = nullptr;
	if (!nt10 || WindowsVersion < SYSTEM_NT_6_2_RTM)
		return;
	node = &nt10->BaseAddressIndexNode;
	while (node->ParentValue & (~7))
		node = decltype(node)(node->ParentValue & (~7));

	if (!node->Red)
	{
		BYTE count = 0;
		PRTL_RB_TREE tmp = nullptr;
		SEARCH_CONTEXT SearchContext{};
		SearchContext.SearchPattern = (LPBYTE)&node;
		SearchContext.PatternSize = sizeof(size_t);
		while (NT_SUCCESS(RtlFindMemoryBlockFromModuleSection(
			(HMODULE)nt10->DllBase, ".data", &SearchContext)))
		{
			if (count++)
				return;
			tmp = (decltype(tmp))SearchContext.Result;
		}
		if (count && tmp && tmp->Root && tmp->Min)
		{
			LdrpModuleBaseAddressIndex = tmp;
		}
	}
}

void BaseAddressIndexInit(PLDR_DATA_TABLE_ENTRY pNtdllEntry)
{
	WindowsVersion = GetSystemVersionCompact();
	FindLdrpModuleBaseAddressIndex((PLDR_DATA_TABLE_ENTRY_WIN10)pNtdllEntry);
	// TODO: _RtlRbRemoveNode _RtlRbInsertNodeEx from pNtdllEntry
}

NTSTATUS NTAPI RtlInsertModuleBaseAddressIndexNode(
	_In_ PLDR_DATA_TABLE_ENTRY DataTableEntry,
	_In_ PVOID BaseAddress) {
	if (!LdrpModuleBaseAddressIndex)return STATUS_UNSUCCESSFUL;

	PLDR_DATA_TABLE_ENTRY_WIN8 LdrNode = CONTAINING_RECORD(LdrpModuleBaseAddressIndex->Root, LDR_DATA_TABLE_ENTRY_WIN8, BaseAddressIndexNode);
	bool bRight = false;

	while (true) {
		if (BaseAddress < LdrNode->DllBase) {
			if (!LdrNode->BaseAddressIndexNode.Left)break;
			LdrNode = CONTAINING_RECORD(LdrNode->BaseAddressIndexNode.Left, LDR_DATA_TABLE_ENTRY_WIN8, BaseAddressIndexNode);
		}
		else if (BaseAddress > LdrNode->DllBase) {
			if (!LdrNode->BaseAddressIndexNode.Right) {
				bRight = true;
				break;
			}
			LdrNode = CONTAINING_RECORD(LdrNode->BaseAddressIndexNode.Right, LDR_DATA_TABLE_ENTRY_WIN8, BaseAddressIndexNode);
		}
		else {
			LdrNode->DdagNode->LoadCount++;
			if (WindowsVersion >= SYSTEM_WIN10_RTM) {
				PLDR_DATA_TABLE_ENTRY_WIN10(LdrNode)->ReferenceCount++;
			}
			return STATUS_SUCCESS;
		}
	}

	RtlRbInsertNodeEx(LdrpModuleBaseAddressIndex, &LdrNode->BaseAddressIndexNode, bRight, &PLDR_DATA_TABLE_ENTRY_WIN8(DataTableEntry)->BaseAddressIndexNode);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlRemoveModuleBaseAddressIndexNode(_In_ PLDR_DATA_TABLE_ENTRY DataTableEntry) {
	RtlRbRemoveNode(LdrpModuleBaseAddressIndex, &PLDR_DATA_TABLE_ENTRY_WIN8(DataTableEntry)->BaseAddressIndexNode);
	return STATUS_SUCCESS;
}
