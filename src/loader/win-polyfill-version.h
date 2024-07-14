#pragma once

#include "phnt.h"
namespace
{
  constexpr DWORD MakeSystemVersionCompact(
    DWORD Major,
    DWORD Minor,
    USHORT BuildNumber,
    USHORT CSDVersion)
  {
    return ((Major << 24) | (Minor << 16) | BuildNumber) + (CSDVersion >> 8);
  }

  // https://github.com/win-polyfill/win-polyfill-pebteb/wiki/Windows--NT-Versions
  constexpr DWORD SYSTEM_NT_4_0_RTM = MakeSystemVersionCompact(4, 0, 1381, 0x0000);
  constexpr DWORD SYSTEM_NT_4_0_SP1 = MakeSystemVersionCompact(4, 0, 1381, 0x0100);
  constexpr DWORD SYSTEM_NT_4_0_SP2 = MakeSystemVersionCompact(4, 0, 1381, 0x0200);
  constexpr DWORD SYSTEM_NT_4_0_SP3 = MakeSystemVersionCompact(4, 0, 1381, 0x0300);
  constexpr DWORD SYSTEM_NT_4_0_SP4 = MakeSystemVersionCompact(4, 0, 1381, 0x0400);
  constexpr DWORD SYSTEM_NT_4_0_SP5 = MakeSystemVersionCompact(4, 0, 1381, 0x0500);
  constexpr DWORD SYSTEM_NT_4_0_SP6 = MakeSystemVersionCompact(4, 0, 1381, 0x0600);

  constexpr DWORD SYSTEM_NT_5_0_RTM = MakeSystemVersionCompact(5, 0, 2195, 0x0000);
  constexpr DWORD SYSTEM_NT_5_0_SP1 = MakeSystemVersionCompact(5, 0, 2195, 0x0100);
  constexpr DWORD SYSTEM_NT_5_0_SP2 = MakeSystemVersionCompact(5, 0, 2195, 0x0200);
  constexpr DWORD SYSTEM_NT_5_0_SP3 = MakeSystemVersionCompact(5, 0, 2195, 0x0300);
  constexpr DWORD SYSTEM_NT_5_0_SP4 = MakeSystemVersionCompact(5, 0, 2195, 0x0400);

  constexpr DWORD SYSTEM_NT_5_1_RTM = MakeSystemVersionCompact(5, 1, 2600, 0x0000);
  constexpr DWORD SYSTEM_NT_5_1_SP1 = MakeSystemVersionCompact(5, 1, 2600, 0x0100);
  constexpr DWORD SYSTEM_NT_5_1_SP2 = MakeSystemVersionCompact(5, 1, 2600, 0x0200);
  constexpr DWORD SYSTEM_NT_5_1_SP3 = MakeSystemVersionCompact(5, 1, 2600, 0x0300);

  constexpr DWORD SYSTEM_NT_5_2_RTM = MakeSystemVersionCompact(5, 2, 3790, 0x0000);
  constexpr DWORD SYSTEM_NT_5_2_SP1 = MakeSystemVersionCompact(5, 2, 3790, 0x0100);
  constexpr DWORD SYSTEM_NT_5_2_SP2 = MakeSystemVersionCompact(5, 2, 3790, 0x0200);

  constexpr DWORD SYSTEM_NT_6_0_RTM = MakeSystemVersionCompact(6, 0, 6000, 0x0000);
  constexpr DWORD SYSTEM_NT_6_0_SP1 = MakeSystemVersionCompact(6, 0, 6001, 0x0100);
  constexpr DWORD SYSTEM_NT_6_0_SP2 = MakeSystemVersionCompact(6, 0, 6002, 0x0200);

  constexpr DWORD SYSTEM_NT_6_1_RTM = MakeSystemVersionCompact(6, 1, 7600, 0x0000);
  constexpr DWORD SYSTEM_NT_6_1_SP1 = MakeSystemVersionCompact(6, 1, 7601, 0x0100);

  constexpr DWORD SYSTEM_NT_6_2_RTM = MakeSystemVersionCompact(6, 2, 9200, 0x0000);
  constexpr DWORD SYSTEM_NT_6_3_RTM = MakeSystemVersionCompact(6, 3, 9600, 0x0000);

  constexpr DWORD SYSTEM_WIN10_RTM = MakeSystemVersionCompact(10, 0, 0, 0x0000);
  // NTDDI_WIN10 RTM
  constexpr DWORD SYSTEM_WIN10_1507 = MakeSystemVersionCompact(10, 0, 10240, 0x0000);
  constexpr DWORD SYSTEM_WIN10_1511 = MakeSystemVersionCompact(10, 0, 10586, 0x0000);
  constexpr DWORD SYSTEM_WIN10_1607 = MakeSystemVersionCompact(10, 0, 14393, 0x0000);
  constexpr DWORD SYSTEM_WIN10_1703 = MakeSystemVersionCompact(10, 0, 15063, 0x0000);
  constexpr DWORD SYSTEM_WIN10_1709 = MakeSystemVersionCompact(10, 0, 16299, 0x0000);
  constexpr DWORD SYSTEM_WIN10_1803 = MakeSystemVersionCompact(10, 0, 17134, 0x0000);
  constexpr DWORD SYSTEM_WIN10_1809 = MakeSystemVersionCompact(10, 0, 17763, 0x0000);
  constexpr DWORD SYSTEM_WIN10_1903 =
    MakeSystemVersionCompact(10, 0, 18362, 0x0000); // NTDDI_WIN10_19H1
  constexpr DWORD SYSTEM_WIN10_1909 = MakeSystemVersionCompact(10, 0, 18363, 0x0000);
  constexpr DWORD SYSTEM_WIN10_2004 = MakeSystemVersionCompact(10, 0, 19041, 0x0000);
  constexpr DWORD SYSTEM_WIN10_20H2 = MakeSystemVersionCompact(10, 0, 19042, 0x0000);
  constexpr DWORD SYSTEM_WIN10_21H1 = MakeSystemVersionCompact(10, 0, 19043, 0x0000);
  constexpr DWORD SYSTEM_WIN10_21H2 = MakeSystemVersionCompact(10, 0, 19044, 0x0000);
  constexpr DWORD SYSTEM_WIN10_22H2 = MakeSystemVersionCompact(10, 0, 19045, 0x0000);
  constexpr DWORD SYSTEM_WIN11_21H2 = MakeSystemVersionCompact(10, 0, 22000, 0x0000);
  constexpr DWORD SYSTEM_WIN11_22H2 = MakeSystemVersionCompact(10, 0, 22621, 0x0000);
  constexpr DWORD SYSTEM_WIN11_23H2 = MakeSystemVersionCompact(10, 0, 22631, 0x0000);
  constexpr DWORD SYSTEM_WIN11_24H2 = MakeSystemVersionCompact(10, 0, 26100, 0x0000);

  __forceinline DWORD GetSystemVersionCompact()
  {
    const auto _pPeb = NtCurrentPeb();
    auto version = MakeSystemVersionCompact(
      _pPeb->OSMajorVersion,
      _pPeb->OSMinorVersion,
      _pPeb->OSBuildNumber,
      _pPeb->OSCSDVersion >> 8);
    return version;
  }

} // namespace
