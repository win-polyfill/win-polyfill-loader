#pragma once

#include <phnt.h>
#include <stdint.h>

namespace
{
    template <typename Char>
    size_t StringLength(_In_z_ const Char *_szString, size_t _cchMaxLength = -1)
    {
        if (!_szString)
            return 0;

        size_t _cchString = 0;
        for (; _cchMaxLength && *_szString; --_cchMaxLength, ++_szString)
        {
            ++_cchString;
        }

        return _cchString;
    }

    static UNICODE_STRING __fastcall MakeNtString(
        _In_z_ const wchar_t *_szString,
        size_t count)
    {
        const auto _cbString = count * sizeof(_szString[0]);
        UNICODE_STRING _Result = {
            (USHORT)min(UINT16_MAX, _cbString),
            (USHORT)min(UINT16_MAX, _cbString + sizeof(_szString[0])),
            const_cast<PWSTR>(_szString)};
        return _Result;
    }

    static UNICODE_STRING __fastcall MakeNtString(_In_z_ const wchar_t *_szString)
    {
        return MakeNtString(_szString, StringLength(_szString, UINT16_MAX / 2));
    }

    static ANSI_STRING __fastcall MakeNtString(_In_z_ const char *_szString, size_t count)
    {
        const auto _cbString = count * sizeof(_szString[0]);

        ANSI_STRING _Result = {
            (USHORT)min(UINT16_MAX, _cbString),
            (USHORT)min(UINT16_MAX, _cbString + sizeof(_szString[0])),
            const_cast<PSTR>(_szString)};
        return _Result;
    }

    static ANSI_STRING __fastcall MakeNtString(_In_z_ const char *_szString)
    {
        return MakeNtString(_szString, StringLength(_szString, UINT16_MAX));
    }

    LONG NTAPI RtlCompareUnicodeStrings(
        _In_reads_(String1Length) PCWCH String1,
        _In_ SIZE_T String1Length,
        _In_reads_(String2Length) PCWCH String2,
        _In_ SIZE_T String2Length,
        _In_ BOOLEAN CaseInSensitive)
    {
        auto a = MakeNtString(String1, String1Length);
        auto b = MakeNtString(String2, String2Length);
        return RtlCompareUnicodeString(&a, &b, CaseInSensitive);
    }

} // namespace
