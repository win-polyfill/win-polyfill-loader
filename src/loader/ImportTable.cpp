#define PHNT_NO_INLINE_INIT_STRING
#include "ImportTable.h"

NTSTATUS MemoryResolveImportTable(
	_In_ LPBYTE base,
	_In_ PIMAGE_NT_HEADERS lpNtHeaders)
{
	NTSTATUS status = STATUS_SUCCESS;
	DWORD count = 0;

	do {
		__try {
			PIMAGE_DATA_DIRECTORY dir = &lpNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
			PIMAGE_IMPORT_DESCRIPTOR importDesc = nullptr;

			if (dir && dir->Size) {
				importDesc = PIMAGE_IMPORT_DESCRIPTOR(lpNtHeaders->OptionalHeader.ImageBase + dir->VirtualAddress);
			}

			if (importDesc) {
				for (; importDesc->Name; ++importDesc) {
					LPCSTR DllNameASCII = (LPCSTR)(base + importDesc->Name);
					uintptr_t* thunkRef;
					PVOID *funcRef;
					PVOID handle = nullptr;
					ANSI_STRING AnsiString;
					UNICODE_STRING DllNameUnicode;
					RtlInitAnsiString(&AnsiString, DllNameASCII);
					status = RtlAnsiStringToUnicodeString(&DllNameUnicode, &AnsiString, TRUE);
					if (!NT_SUCCESS(status))
						break;
					status = LdrGetDllHandle(nullptr, nullptr, &DllNameUnicode, &handle);
					RtlFreeUnicodeString(&DllNameUnicode);
					if (!NT_SUCCESS(status))
						break;
					if (!handle) {
						status = STATUS_DLL_NOT_FOUND;
						break;
					}

					thunkRef = (uintptr_t*)(base + (importDesc->OriginalFirstThunk ? importDesc->OriginalFirstThunk : importDesc->FirstThunk));
					funcRef = (PVOID*)(base + importDesc->FirstThunk);
					while (*thunkRef) {
						ANSI_STRING ProcedureNameANSI;
						PANSI_STRING ProcedureName = nullptr;
						ULONG ProcedureNumber = 0;
						bool IsOrdinal = IMAGE_SNAP_BY_ORDINAL(*thunkRef);
						if (IsOrdinal) {
							ProcedureNumber = IMAGE_ORDINAL(*thunkRef);
						} else {
							ProcedureName = &ProcedureNameANSI;
						}

						status = LdrGetProcedureAddress(
							handle, ProcedureName, ProcedureNumber, funcRef);
						if (!NT_SUCCESS(status)) {
							break;
						}
						if (!*funcRef) {
							status = STATUS_ENTRYPOINT_NOT_FOUND;
							break;
						}
						++thunkRef;
						++funcRef;
					}

					if (!NT_SUCCESS(status))break;
				}

			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			status = GetExceptionCode();
		}
	} while (false);

	return status;
}
