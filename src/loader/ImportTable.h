#pragma once

#include "phnt.h"

NTSTATUS MemoryResolveImportTable(
	_In_ LPBYTE base,
	_In_ PIMAGE_NT_HEADERS lpNtHeaders
);
