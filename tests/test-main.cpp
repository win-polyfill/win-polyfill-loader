#include <sdkddkver.h>

#include "ntpebteb.h"

#include <windows.h>

int main()
{
  auto teb = NtCurrentTeb();
  return 0;
}
