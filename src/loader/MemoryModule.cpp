#include "MemoryModule.h"

#if defined(_M_ARM64)
#define HOST_MACHINE IMAGE_FILE_MACHINE_ARM64
#elif defined(_M_ARM)
#define HOST_MACHINE IMAGE_FILE_MACHINE_ARM
#elif defined(_WIN64)
#define HOST_MACHINE IMAGE_FILE_MACHINE_AMD64
#else
#define HOST_MACHINE IMAGE_FILE_MACHINE_I386
#endif

#define GET_HEADER_DICTIONARY(headers, idx)  &headers->OptionalHeader.DataDirectory[idx]

#define AlignValueUp(value, alignment) ((size_t(value) + size_t(alignment) - 1) & ~(size_t(alignment) - 1))

#define OffsetPointer(data, offset) LPVOID(LPBYTE(data) + ptrdiff_t(offset))

// Protection flags for memory pages (Executable, Readable, Writeable)
static const int ProtectionFlags[2][2][2] = {
	{
		// not executable
		{PAGE_NOACCESS, PAGE_WRITECOPY},
		{PAGE_READONLY, PAGE_READWRITE},
	}, {
		// executable
		{PAGE_EXECUTE, PAGE_EXECUTE_WRITECOPY},
		{PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE},
	},
};

NTSTATUS MemorySetSectionProtection(
	_In_ LPBYTE base,
	_In_ PIMAGE_NT_HEADERS lpNtHeaders) {
	NTSTATUS status = STATUS_SUCCESS;
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(lpNtHeaders);

	for (DWORD i = 0; i < lpNtHeaders->FileHeader.NumberOfSections; ++i, ++section) {
		LPVOID address = LPBYTE(base) + section->VirtualAddress;
		SIZE_T size = AlignValueUp(section->Misc.VirtualSize, lpNtHeaders->OptionalHeader.SectionAlignment);

		BOOL executable = (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0,
			readable = (section->Characteristics & IMAGE_SCN_MEM_READ) != 0,
			writeable = (section->Characteristics & IMAGE_SCN_MEM_WRITE) != 0;
		DWORD protect = ProtectionFlags[executable][readable][writeable], oldProtect;

		if (section->Characteristics & IMAGE_SCN_MEM_NOT_CACHED) protect |= PAGE_NOCACHE;

		status = NtProtectVirtualMemory(NtCurrentProcess(), &address, &size, protect, &oldProtect);
		if (!NT_SUCCESS(status))break;
	}

	return status;
}

NTSTATUS MemoryLoadLibrary(
	_Out_ PVOID* MemoryModuleHandle,
	_In_ LPCVOID data,
	_In_ DWORD size) {

	PIMAGE_DOS_HEADER dos_header = nullptr;
	PIMAGE_NT_HEADERS old_header = nullptr;
	BOOLEAN CorImage = FALSE;
	NTSTATUS status = STATUS_SUCCESS;

	//
	// Check parameters
	//
	__try {

		*MemoryModuleHandle = nullptr;

		//
		// Check dos magic
		//
		dos_header = (PIMAGE_DOS_HEADER)data;
		if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
			status = STATUS_INVALID_IMAGE_FORMAT;
			__leave;
		}

		//
		// Check nt headers
		//
		old_header = (PIMAGE_NT_HEADERS)((size_t)data + dos_header->e_lfanew);
		if (old_header->Signature != IMAGE_NT_SIGNATURE ||
			old_header->OptionalHeader.SectionAlignment & 1) {
			status = STATUS_INVALID_IMAGE_FORMAT;
			__leave;
		}

		//
		// Match machine type
		//
		if (old_header->FileHeader.Machine != HOST_MACHINE) {
			status = STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
			__leave;
		}

		//
		// Only dll image support
		//
		if (!(old_header->FileHeader.Characteristics & IMAGE_FILE_DLL)) {
			status = STATUS_NOT_SUPPORTED;
			__leave;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		status = GetExceptionCode();
	}
	if (!NT_SUCCESS(status) || status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH)return status;

	//
	// Reserve the address range of image
	//
	LPBYTE base = nullptr;
	if ((old_header->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE) == 0) {
		base = (LPBYTE)VirtualAlloc(
			LPVOID(old_header->OptionalHeader.ImageBase),
			old_header->OptionalHeader.SizeOfImage,
			MEM_RESERVE,
			PAGE_EXECUTE_READWRITE
		);
	}
	if (!base) {
		base = (LPBYTE)VirtualAlloc(
			nullptr,
			old_header->OptionalHeader.SizeOfImage,
			MEM_RESERVE,
			PAGE_EXECUTE_READWRITE
		);
		if (!base) status = STATUS_NO_MEMORY;
	}

	if (!NT_SUCCESS(status)) {
		return status;
	}

	//
	// Copy headers
	//
	PIMAGE_DOS_HEADER new_dos_header = (PIMAGE_DOS_HEADER)base;
	PIMAGE_NT_HEADERS new_header = (PIMAGE_NT_HEADERS)(base + dos_header->e_lfanew);
	RtlCopyMemory(
		new_dos_header,
		dos_header,
		old_header->OptionalHeader.SizeOfHeaders
	);
	new_header->OptionalHeader.ImageBase = (size_t)base;

	do {
		//
		// Allocate and copy sections
		//
		PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(new_header);
		for (DWORD i = 0; i < new_header->FileHeader.NumberOfSections; ++i, ++section) {

			DWORD size = AlignValueUp(
				section->Misc.VirtualSize,
				new_header->OptionalHeader.SectionAlignment
			);
			if (size < section->SizeOfRawData) {
				status = STATUS_INVALID_IMAGE_FORMAT;
				break;
			}

			LPVOID dest = VirtualAlloc(
				(LPSTR)new_header->OptionalHeader.ImageBase + section->VirtualAddress,
				size,
				MEM_COMMIT,
				PAGE_READWRITE
			);
			if (!dest) {
				status = STATUS_NO_MEMORY;
				break;
			}

			if (section->SizeOfRawData) {
				RtlCopyMemory(
					dest,
					LPBYTE(data) + section->PointerToRawData,
					section->SizeOfRawData
				);
			}

		}
		if (!NT_SUCCESS(status))break;

		//
		// Rebase image
		//
		auto locationDelta = new_header->OptionalHeader.ImageBase - old_header->OptionalHeader.ImageBase;
		if (locationDelta) {
			typedef struct _REBASE_INFO {
				USHORT Offset : 12;
				USHORT Type : 4;
			}REBASE_INFO, * PREBASE_INFO;
			typedef struct _IMAGE_BASE_RELOCATION_HEADER {
				DWORD VirtualAddress;
				DWORD SizeOfBlock;
				REBASE_INFO TypeOffset[ANYSIZE_ARRAY];

				DWORD TypeOffsetCount()const {
					return (this->SizeOfBlock - 8) / sizeof(_REBASE_INFO);
				}
			}IMAGE_BASE_RELOCATION_HEADER, * PIMAGE_BASE_RELOCATION_HEADER;

			PIMAGE_DATA_DIRECTORY dir = GET_HEADER_DICTIONARY(new_header, IMAGE_DIRECTORY_ENTRY_BASERELOC);
			PIMAGE_BASE_RELOCATION_HEADER relocation = (PIMAGE_BASE_RELOCATION_HEADER)(LPBYTE(base) + dir->VirtualAddress);

			if (dir->Size && dir->VirtualAddress) {
				while ((LPBYTE(relocation) < LPBYTE(base) + dir->VirtualAddress + dir->Size) && relocation->VirtualAddress > 0) {
					auto relInfo = (_REBASE_INFO*)&relocation->TypeOffset;
					for (DWORD i = 0; i < relocation->TypeOffsetCount(); ++i, ++relInfo) {
						switch (relInfo->Type) {
						case IMAGE_REL_BASED_HIGHLOW: *(DWORD*)(base + relocation->VirtualAddress + relInfo->Offset) += (DWORD)locationDelta; break;
#ifdef _WIN64
						case IMAGE_REL_BASED_DIR64: *(ULONGLONG*)(base + relocation->VirtualAddress + relInfo->Offset) += (ULONGLONG)locationDelta; break;
#endif
						case IMAGE_REL_BASED_ABSOLUTE:
						default: break;
						}
					}

					// advance to next relocation block
					//relocation->VirtualAddress += module->headers_align;
					relocation = decltype(relocation)(OffsetPointer(relocation, relocation->SizeOfBlock));
				}
			}

		}
		if (!NT_SUCCESS(status))break;

		*MemoryModuleHandle = base;
		return status;
	} while (false);

	return status;
}
