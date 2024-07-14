#pragma once

#include "phnt.h"

//
// Determine whether to use MmpTls(1) or LdrpTls(0)
//
#ifndef MMPP_USE_TLS
#define MMPP_USE_TLS 1
#endif

// offsetof()
#include <cstddef>

//memory module base support
#include "MemoryModule.h"

//import table support
#include "ImportTable.h"

//LDR_DATA_TABLE_ENTRY
#include "LdrEntry.h"

//rtl inverted function table for exception handling
#include "InvertedFunctionTable.h"

//base address index
#include "BaseAddressIndex.h"

//tls support
#include "MmpTls.h"

//DotNet support
#include "MmpDotNet.h"

//MemoryModulePP api interface
#include "Loader.h"

//utils
#include "Utils.h"

//global data
#include "MmpGlobalData.h"
