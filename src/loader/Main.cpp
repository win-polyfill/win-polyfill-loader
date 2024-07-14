#include "LdrEntry.h"
#include "BaseAddressIndex.h"

EXTERN_C BOOL WINAPI
_DllMainCRTStartup(HINSTANCE const hInstance, DWORD const dwReason, LPVOID const reserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		PLDR_DATA_TABLE_ENTRY pLdrEntry = RtlAllocateDataTableEntry(nullptr);
		DWORD dwFlags = 0;
		PVOID BaseAddress = 0;
		UNICODE_STRING DllBaseName = {};
		UNICODE_STRING DllFullName = {};
		auto pNtdllLdrEntry = RtlFindLdrTableEntryByBaseName(L"ntdll.DLL");
		RtlInitializeLdrDataTableEntry(pLdrEntry, dwFlags, BaseAddress, DllBaseName, DllFullName);
		RtlInsertMemoryTableEntry(pLdrEntry);
		RtlFreeLdrDataTableEntry(pLdrEntry);
		BaseAddressIndexInit(pNtdllLdrEntry);
		break;
	}
	case DLL_THREAD_ATTACH:
	{
		break;
	}
	case DLL_THREAD_DETACH:
	{
		break;
	}
	case DLL_PROCESS_DETACH:
	{
		break;
	}
	default:
		break;
	}
	return true;
}
