#include "LdrEntry.h"
#include "BaseAddressIndex.h"
#include "win-polyfill-version.h"
#include "win-polyfill-string.h"

#include <stddef.h>

static DWORD WindowsVersion;
static WORD LdrDataTableEntrySize;
static PLIST_ENTRY LdrpHashTable;

void LdrEntryLibInit()
{
	WindowsVersion = GetSystemVersionCompact();
}

static NTSTATUS RtlFreeDependencies(_In_ PLDR_DATA_TABLE_ENTRY_WIN10 LdrEntry) {
	_LDR_DDAG_NODE* DependentDdgeNode = nullptr;
	PLDR_DATA_TABLE_ENTRY_WIN10 ModuleEntry = nullptr;
	PLDRP_CSLIST_DEPENDENT head = LdrEntry->DdagNode->Dependencies, entry = head;
	HANDLE heap = NtCurrentPeb()->ProcessHeap;
	BOOL IsWin8 = WindowsVersion >= SYSTEM_NT_6_2_RTM && WindowsVersion < SYSTEM_WIN10_RTM;
	if (!LdrEntry->DdagNode->Dependencies)return STATUS_SUCCESS;

	//find all dependencies and free
	do {
		DependentDdgeNode = entry->DependentDdagNode;
		if (DependentDdgeNode->Modules.Flink->Flink != &DependentDdgeNode->Modules) __fastfail(FAST_FAIL_CORRUPT_LIST_ENTRY);
		ModuleEntry = decltype(ModuleEntry)((size_t)DependentDdgeNode->Modules.Flink - offsetof(_LDR_DATA_TABLE_ENTRY_WIN8, NodeModuleLink));
		if (ModuleEntry->DdagNode != DependentDdgeNode) __fastfail(FAST_FAIL_CORRUPT_LIST_ENTRY);
		if (!DependentDdgeNode->IncomingDependencies) __fastfail(FAST_FAIL_CORRUPT_LIST_ENTRY);
		PLDRP_CSLIST_INCOMMING _last = DependentDdgeNode->IncomingDependencies, _entry = _last;
		_LDR_DDAG_NODE* CurrentDdagNode;
		ULONG State = 0;
		PVOID Cookies;

		//Acquire LoaderLock
		do {
			if (!NT_SUCCESS(LdrLockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY, &State, &Cookies))) __fastfail(FAST_FAIL_FATAL_APP_EXIT);
		} while (State != LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED);

		do {
			CurrentDdagNode = (decltype(CurrentDdagNode))((size_t)_entry->IncommingDdagNode & ~1);
			if (CurrentDdagNode == LdrEntry->DdagNode) {
				//node is head
				if (_entry == DependentDdgeNode->IncomingDependencies) {
					//only one node in list
					if (_entry->NextIncommingEntry == (PSINGLE_LIST_ENTRY)DependentDdgeNode->IncomingDependencies) {
						DependentDdgeNode->IncomingDependencies = nullptr;
					}
					else {
						//find the last node in the list
						PSINGLE_LIST_ENTRY i = _entry->NextIncommingEntry;
						while (i->Next != (PSINGLE_LIST_ENTRY)_entry)i = i->Next;
						i->Next = _entry->NextIncommingEntry;
						DependentDdgeNode->IncomingDependencies = (PLDRP_CSLIST_INCOMMING)_entry->NextIncommingEntry;
					}
				}
				//node is not head
				else {
					_last->NextIncommingEntry = _entry->NextIncommingEntry;
				}
				break;
			}

			//save the last entry
			if (_last != _entry)_last = (decltype(_last))_last->NextIncommingEntry;
			_entry = (decltype(_entry))_entry->NextIncommingEntry;
		} while (_entry != _last);
		//free LoaderLock
		LdrUnlockLoaderLock(0, Cookies);
		entry = (decltype(entry))entry->NextDependentEntry;

		//free it
		if (IsWin8) {
			//Update win8 dep count
			_LDR_DDAG_NODE_WIN8* win8_node = (decltype(win8_node))ModuleEntry->DdagNode;
			if (!win8_node->DependencyCount)__fastfail(FAST_FAIL_CORRUPT_LIST_ENTRY);
			--win8_node->DependencyCount;
			if (!ModuleEntry->DdagNode->LoadCount && win8_node->ReferenceCount == 1 && !win8_node->DependencyCount) {
				win8_node->LoadCount = 1;
				LdrUnloadDll(ModuleEntry->DllBase);
			}
		}
		else {
			LdrUnloadDll(ModuleEntry->DllBase);
		}
		RtlFreeHeap(heap, 0, LdrEntry->DdagNode->Dependencies);

		//lookup next dependent.
		LdrEntry->DdagNode->Dependencies = (PLDRP_CSLIST_DEPENDENT)(entry == head ? nullptr : entry);
	} while (entry != head);

	return STATUS_SUCCESS;
}

PLDR_DATA_TABLE_ENTRY NTAPI RtlAllocateDataTableEntry(_In_ PVOID BaseAddress) {
	PLDR_DATA_TABLE_ENTRY LdrEntry = nullptr;
	PIMAGE_NT_HEADERS NtHeader;
	HANDLE heap = NtCurrentPeb()->ProcessHeap;

	/* Make sure the header is valid */
	if (NtHeader = RtlImageNtHeader(BaseAddress)) {
		/* Allocate an entry */
		LdrEntry = (PLDR_DATA_TABLE_ENTRY)RtlAllocateHeap(heap, HEAP_ZERO_MEMORY, LdrDataTableEntrySize);
	}

	/* Return the entry */
	return LdrEntry;
}

BOOL NTAPI RtlInitializeLdrDataTableEntry(
	_Out_ PLDR_DATA_TABLE_ENTRY LdrEntry,
	_In_ DWORD dwFlags,
	_In_ PVOID BaseAddress,
	_In_ UNICODE_STRING& DllBaseName,
	_In_ UNICODE_STRING& DllFullName) {
	RtlZeroMemory(LdrEntry, LdrDataTableEntrySize);
	PIMAGE_NT_HEADERS headers = RtlImageNtHeader(BaseAddress);
	if (!headers)return FALSE;
	HANDLE heap = NtCurrentPeb()->ProcessHeap;
	bool FlagsProcessed = false;

	bool CorImage = false, CorIL = false;
	auto& com = headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
	if (com.Size && com.VirtualAddress) {
		CorImage = true;

		auto cor = PIMAGE_COR20_HEADER(LPBYTE(BaseAddress) + com.VirtualAddress);
		if (cor->Flags & ReplacesCorHdrNumericDefines::COMIMAGE_FLAGS_ILONLY) {
			CorIL = true;
		}
	}

	if (WindowsVersion >= SYSTEM_WIN11_21H2) {
		auto entry = (PLDR_DATA_TABLE_ENTRY_WIN11)LdrEntry;
		entry->CheckSum = headers->OptionalHeader.CheckSum;
	}

	if (WindowsVersion >= SYSTEM_WIN10_RTM) {
		auto entry = (PLDR_DATA_TABLE_ENTRY_WIN10)LdrEntry;
		entry->ReferenceCount = 1;
	}

	if (WindowsVersion >= SYSTEM_NT_6_2_RTM) {
		auto entry = (PLDR_DATA_TABLE_ENTRY_WIN8)LdrEntry;
		BOOL IsWin8 = WindowsVersion < SYSTEM_WIN10_RTM;
		NtQuerySystemTime(&entry->LoadTime);
		entry->OriginalBase = headers->OptionalHeader.ImageBase;
		entry->BaseNameHashValue = LdrHashEntryRaw(DllBaseName);
		entry->LoadReason = LoadReasonDynamicLoad;
		if (!NT_SUCCESS(RtlInsertModuleBaseAddressIndexNode(LdrEntry, BaseAddress)))return FALSE;
		if (!(entry->DdagNode = (decltype(entry->DdagNode))
			RtlAllocateHeap(heap, HEAP_ZERO_MEMORY, IsWin8 ? sizeof(_LDR_DDAG_NODE_WIN8) : sizeof(_LDR_DDAG_NODE))))return FALSE;

		entry->NodeModuleLink.Flink = &entry->DdagNode->Modules;
		entry->NodeModuleLink.Blink = &entry->DdagNode->Modules;
		entry->DdagNode->Modules.Flink = &entry->NodeModuleLink;
		entry->DdagNode->Modules.Blink = &entry->NodeModuleLink;
		entry->DdagNode->State = LdrModulesReadyToRun;
		entry->DdagNode->LoadCount = 1;
		if (IsWin8) ((_LDR_DDAG_NODE_WIN8*)(entry->DdagNode))->ReferenceCount = 1;
		entry->ImageDll = entry->LoadNotificationsSent = entry->EntryProcessed =
			entry->InLegacyLists = entry->InIndexes = true;
		entry->ProcessAttachCalled = false;
		entry->InExceptionTable = false;
		entry->CorImage = CorImage;
		entry->CorILOnly = CorIL;

		FlagsProcessed = true;
	}

	if (WindowsVersion > SYSTEM_NT_6_1_RTM) {
		if (LdrDataTableEntrySize == sizeof(LDR_DATA_TABLE_ENTRY_WIN7)) {
			auto entry = (PLDR_DATA_TABLE_ENTRY_WIN7)LdrEntry;
			entry->OriginalBase = headers->OptionalHeader.ImageBase;
			NtQuerySystemTime(&entry->LoadTime);
		}
	}

	if (WindowsVersion > SYSTEM_NT_6_0_RTM) {
		if (LdrDataTableEntrySize == sizeof(LDR_DATA_TABLE_ENTRY_VISTA) ||
			LdrDataTableEntrySize == sizeof(LDR_DATA_TABLE_ENTRY_WIN7)) {
			auto entry = (PLDR_DATA_TABLE_ENTRY_VISTA)LdrEntry;
			InitializeListHead(&entry->ForwarderLinks);
			InitializeListHead(&entry->StaticLinks);
			InitializeListHead(&entry->ServiceTagLinks);
		}
	}
	if (WindowsVersion > SYSTEM_NT_5_0_RTM) {
		LdrEntry->DllBase = BaseAddress;
		LdrEntry->SizeOfImage = headers->OptionalHeader.SizeOfImage;
		LdrEntry->TimeDateStamp = headers->FileHeader.TimeDateStamp;
		LdrEntry->BaseDllName = DllBaseName;
		LdrEntry->FullDllName = DllFullName;
		LdrEntry->EntryPoint = (PLDR_INIT_ROUTINE)((size_t)BaseAddress + headers->OptionalHeader.AddressOfEntryPoint);
		LdrEntry->LoadCount = 1;
		if (!FlagsProcessed) {
			LdrEntry->Flags = LDRP_IMAGE_DLL | LDRP_ENTRY_INSERTED | LDRP_ENTRY_PROCESSED;

			if (CorImage)LdrEntry->Flags |= LDRP_COR_IMAGE;
		}
		InitializeListHead(&LdrEntry->HashLinks);
		return TRUE;
	}
	return FALSE;
}

BOOL NTAPI RtlFreeLdrDataTableEntry(_In_ PLDR_DATA_TABLE_ENTRY LdrEntry) {
	HANDLE heap = NtCurrentPeb()->ProcessHeap;
	if (WindowsVersion >= SYSTEM_NT_6_2_RTM) {
		auto entry = (PLDR_DATA_TABLE_ENTRY_WIN10)LdrEntry;
		RtlFreeDependencies(entry);
		RtlFreeHeap(heap, 0, entry->DdagNode);
		RtlRemoveModuleBaseAddressIndexNode(LdrEntry);
	}

	if (WindowsVersion >= SYSTEM_NT_6_0_RTM) {
		if (LdrDataTableEntrySize == sizeof(LDR_DATA_TABLE_ENTRY_VISTA) ||
			LdrDataTableEntrySize == sizeof(LDR_DATA_TABLE_ENTRY_WIN7)) {
			PLDR_DATA_TABLE_ENTRY_VISTA entry = (decltype(entry))LdrEntry;
			PLIST_ENTRY head = &entry->ForwarderLinks, next = head->Flink;
			while (head != next) {
				PLDR_DATA_TABLE_ENTRY dep = *(decltype(&dep))((size_t*)next + 2);
				LdrUnloadDll(dep->DllBase);
				next = next->Flink;
				RtlFreeHeap(heap, 0, next->Blink);
			}
		}
	}

	if (WindowsVersion >= SYSTEM_NT_5_0_RTM) {
		RtlFreeHeap(heap, 0, LdrEntry->BaseDllName.Buffer);
		RtlFreeHeap(heap, 0, LdrEntry->FullDllName.Buffer);
		RemoveEntryList(&LdrEntry->InLoadOrderLinks);
		RemoveEntryList(&LdrEntry->InMemoryOrderLinks);
		RemoveEntryList(&LdrEntry->InInitializationOrderLinks);
		RemoveEntryList(&LdrEntry->HashLinks);
		RtlFreeHeap(heap, 0, LdrEntry);
		return TRUE;
	}
	return FALSE;
}

VOID NTAPI RtlInsertMemoryTableEntry(_In_ PLDR_DATA_TABLE_ENTRY LdrEntry) {
	PPEB_LDR_DATA PebData = NtCurrentPeb()->Ldr;
	ULONG i;

	/* Insert into hash table */
	i = LdrHashEntry(LdrEntry->BaseDllName);
	InsertTailList(&LdrpHashTable[i], &LdrEntry->HashLinks);

	/* Insert into other lists */
	InsertTailList(&PebData->InLoadOrderModuleList, &LdrEntry->InLoadOrderLinks);
	InsertTailList(&PebData->InMemoryOrderModuleList, &LdrEntry->InMemoryOrderLinks);
	InsertTailList(&PebData->InInitializationOrderModuleList, &LdrEntry->InInitializationOrderLinks);
}

PLDR_DATA_TABLE_ENTRY NTAPI RtlFindLdrTableEntryByHandle(_In_ PVOID BaseAddress) {
	PLIST_ENTRY ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList, ListEntry = ListHead->Flink;
	PLDR_DATA_TABLE_ENTRY CurEntry;
	while (ListEntry != ListHead) {
		CurEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
		ListEntry = ListEntry->Flink;
		if (CurEntry->DllBase == BaseAddress) {
			return CurEntry;
		}
	}
	return nullptr;
}

PLDR_DATA_TABLE_ENTRY NTAPI RtlFindLdrTableEntryByBaseName(_In_z_ PCWSTR BaseName) {
	PLIST_ENTRY ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList, ListEntry = ListHead->Flink;
	PLDR_DATA_TABLE_ENTRY CurEntry;
	while (ListEntry != ListHead) {
		CurEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
		ListEntry = ListEntry->Flink;
		auto BaseNameUnicode = MakeNtString(BaseName);
		if (RtlCompareUnicodeString(&CurEntry->BaseDllName, &BaseNameUnicode, TRUE) == 0)
		{
			return CurEntry;
		}
	}
	return nullptr;
}

ULONG NTAPI LdrHashEntryRaw(_In_ UNICODE_STRING& DllBaseName) {
	ULONG result = 0;

	if (WindowsVersion < SYSTEM_NT_6_0_RTM)
	{
		result = RtlUpcaseUnicodeChar(DllBaseName.Buffer[0]) - 'A';
	}
	else if (WindowsVersion < SYSTEM_NT_6_1_RTM)
	{
		result = RtlUpcaseUnicodeChar(DllBaseName.Buffer[0]) - 1;
	}
	else if (WindowsVersion < SYSTEM_NT_6_3_RTM)
	{
		for (USHORT i = 0; i < (DllBaseName.Length / sizeof(wchar_t)); ++i)
			result += 0x1003F * RtlUpcaseUnicodeChar(DllBaseName.Buffer[i]);
	}
	else
	{
		RtlHashUnicodeString(&DllBaseName, TRUE, HASH_STRING_ALGORITHM_DEFAULT, &result);
	}
	return result;
}
