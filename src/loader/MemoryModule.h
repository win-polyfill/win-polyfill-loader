#pragma once

#include "phnt.h"

NTSTATUS MemoryLoadLibrary(
	_Out_ PVOID* MemoryModuleHandle,
	_In_ LPCVOID data,
	_In_ DWORD size
);

NTSTATUS MemorySetSectionProtection(
	_In_ LPBYTE base,
	_In_ PIMAGE_NT_HEADERS lpNtHeaders
);
