#include "Utils.h"

#ifndef _WIN64
#undef RtlCompareMemory
#endif


static WORD CalcCheckSum(DWORD StartValue, LPVOID BaseAddress, DWORD WordCount) {
	LPWORD Ptr = (LPWORD)BaseAddress;
	DWORD Sum = StartValue;
	for (DWORD i = 0; i < WordCount; i++) {
		Sum += *Ptr;
		if (HIWORD(Sum) != 0) Sum = LOWORD(Sum) + HIWORD(Sum);
		Ptr++;
	}
	return (WORD)(LOWORD(Sum) + HIWORD(Sum));
}

static BOOLEAN CheckSumBufferedFile(LPVOID BaseAddress, DWORD BufferLength, DWORD CheckSum) {
	DWORD CalcSum = CalcCheckSum(0, BaseAddress, (BufferLength + 1) / sizeof(WORD)), HdrSum = CheckSum;

	if (LOWORD(CalcSum) >= LOWORD(HdrSum)) CalcSum -= LOWORD(HdrSum);
	else CalcSum = ((LOWORD(CalcSum) - LOWORD(HdrSum)) & 0xFFFF) - 1;
	if (LOWORD(CalcSum) >= HIWORD(HdrSum)) CalcSum -= HIWORD(HdrSum);
	else CalcSum = ((LOWORD(CalcSum) - HIWORD(HdrSum)) & 0xFFFF) - 1;

	CalcSum += BufferLength;
	return HdrSum == CalcSum;
}

BOOLEAN NTAPI RtlIsValidImageBuffer(
	_In_ PVOID Buffer,
	_Out_opt_ size_t* Size) {

	BOOLEAN result = FALSE;
	__try {

		if (Size)*Size = 0;

		union {
			PIMAGE_NT_HEADERS32 nt32;
			PIMAGE_NT_HEADERS64 nt64;
			PIMAGE_NT_HEADERS nt;
		}headers;
		headers.nt = RtlImageNtHeader(Buffer);
		PIMAGE_SECTION_HEADER sections = nullptr;
		size_t SizeofImage = 0;
		DWORD CheckSum = 0;

		if (!headers.nt) return FALSE;

		switch (headers.nt->OptionalHeader.Magic) {
		case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
			sections = PIMAGE_SECTION_HEADER((char*)&headers.nt32->OptionalHeader + headers.nt32->FileHeader.SizeOfOptionalHeader);
			SizeofImage = headers.nt32->OptionalHeader.SizeOfHeaders;
			ProbeForRead(sections, headers.nt32->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
			for (WORD i = 0; i < headers.nt32->FileHeader.NumberOfSections; ++i, ++sections)
				SizeofImage += sections->SizeOfRawData;

			//Signature size
			SizeofImage += headers.nt32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size;

			CheckSum = headers.nt32->OptionalHeader.CheckSum;

			break;
		case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
			sections = PIMAGE_SECTION_HEADER((char*)&headers.nt64->OptionalHeader + headers.nt64->FileHeader.SizeOfOptionalHeader);
			SizeofImage = headers.nt64->OptionalHeader.SizeOfHeaders;
			ProbeForRead(sections, headers.nt64->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
			for (WORD i = 0; i < headers.nt64->FileHeader.NumberOfSections; ++i, ++sections)
				SizeofImage += sections->SizeOfRawData;
			SizeofImage += headers.nt64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size;

			CheckSum = headers.nt64->OptionalHeader.CheckSum;

			break;
		default:
			return FALSE;
		}

		ProbeForRead(Buffer, SizeofImage);
		if (Size)*Size = SizeofImage;

		if (!CheckSum)return TRUE;

		result = CheckSumBufferedFile(Buffer, (DWORD)SizeofImage, CheckSum);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		SetLastError(RtlNtStatusToDosError(GetExceptionCode()));
	}
	return result;
}

BOOLEAN NTAPI VirtualAccessCheckNoException(LPCVOID pBuffer, size_t size, ACCESS_MASK protect) {
	if (size) {
		MEMORY_BASIC_INFORMATION mbi{};
		SIZE_T len = 0;
		if (!NT_SUCCESS(NtQueryVirtualMemory(NtCurrentProcess(), const_cast<PVOID>(pBuffer), MemoryBasicInformation, &mbi, sizeof(mbi), &len)) ||
			!(mbi.Protect & protect)) {
			return FALSE;
		}
	}
	return TRUE;
}

BOOLEAN NTAPI VirtualAccessCheck(LPCVOID pBuffer, size_t size, ACCESS_MASK protect) {
	if (!VirtualAccessCheckNoException(pBuffer, size, protect)) {
		RtlRaiseStatus(STATUS_ACCESS_VIOLATION);
		return FALSE;
	}
	return TRUE;
}

BOOL NTAPI RtlVerifyVersion(
	_In_ DWORD MajorVersion,
	_In_ DWORD MinorVersion,
	_In_ DWORD BuildNumber,
	_In_ BYTE Flags
) {
	if (MmpGlobalDataPtr->NtVersions.MajorVersion == MajorVersion &&
		((Flags & RTL_VERIFY_FLAGS_MINOR_VERSION) ? MmpGlobalDataPtr->NtVersions.MinorVersion == MinorVersion : true) &&
		((Flags & RTL_VERIFY_FLAGS_BUILD_NUMBERS) ? MmpGlobalDataPtr->NtVersions.BuildNumber == BuildNumber : true))return TRUE;
	return FALSE;
}

BOOL NTAPI RtlIsWindowsVersionOrGreater(
	_In_ DWORD MajorVersion,
	_In_ DWORD MinorVersion,
	_In_ DWORD BuildNumber
) {
	if (MmpGlobalDataPtr->NtVersions.MajorVersion == MajorVersion) {
		if (MmpGlobalDataPtr->NtVersions.MinorVersion == MinorVersion) return MmpGlobalDataPtr->NtVersions.BuildNumber >= BuildNumber;
		else return (MmpGlobalDataPtr->NtVersions.MinorVersion > MinorVersion);
	}
	else return MmpGlobalDataPtr->NtVersions.MajorVersion > MajorVersion;
}

BOOL NTAPI RtlIsWindowsVersionInScope(
	_In_ DWORD MinMajorVersion,
	_In_ DWORD MinMinorVersion,
	_In_ DWORD MinBuildNumber,

	_In_ DWORD MaxMajorVersion,
	_In_ DWORD MaxMinorVersion,
	_In_ DWORD MaxBuildNumber
) {
	return RtlIsWindowsVersionOrGreater(MinMajorVersion, MinMinorVersion, MinBuildNumber) &&
		!RtlIsWindowsVersionOrGreater(MaxMajorVersion, MaxMinorVersion, MaxBuildNumber);
}

#ifndef _WIN64
int NTAPI RtlCaptureImageExceptionValues(PVOID BaseAddress, PDWORD SEHandlerTable, PDWORD SEHandlerCount) {
	PIMAGE_LOAD_CONFIG_DIRECTORY pLoadConfigDirectory;
	PIMAGE_COR20_HEADER pCor20;
	ULONG Size;

	//check if no seh
	if (RtlImageNtHeader(BaseAddress)->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_SEH) {
		*SEHandlerTable = *SEHandlerCount = -1;
		return 0;
	}

	//get seh table and count
	pLoadConfigDirectory = (decltype(pLoadConfigDirectory))RtlImageDirectoryEntryToData(BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &Size);
	if (pLoadConfigDirectory) {
		if (Size == 0x40 && pLoadConfigDirectory->Size >= 0x48u) {
			if (pLoadConfigDirectory->SEHandlerTable && pLoadConfigDirectory->SEHandlerCount) {
				*SEHandlerTable = pLoadConfigDirectory->SEHandlerTable;
				return *SEHandlerCount = pLoadConfigDirectory->SEHandlerCount;
			}
		}
	}

	//is .net core ?
	pCor20 = (decltype(pCor20))RtlImageDirectoryEntryToData(BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, &Size);
	*SEHandlerTable = *SEHandlerCount = ((pCor20 && pCor20->Flags & 1) ? -1 : 0);
	return 0;
}
#endif
