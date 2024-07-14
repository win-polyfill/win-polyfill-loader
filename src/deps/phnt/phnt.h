/*
 * NT Header annotations
 *
 * This file is part of System Informer.
 */

#ifndef _PHNT_H
#define _PHNT_H

// This header file provides access to NT APIs.

// Definitions are annotated to indicate their source. If a definition is not annotated, it has been
// retrieved from an official Microsoft source (NT headers, DDK headers, winnt.h).

// * "winbase" indicates that a definition has been reconstructed from a Win32-ized NT definition in
//   winbase.h.
// * "rev" indicates that a definition has been reverse-engineered.
// * "dbg" indicates that a definition has been obtained from a debug message or assertion in a
//   checked build of the kernel or file.

// Reliability:
// 1. No annotation.
// 2. dbg.
// 3. symbols, private. Types may be incorrect.
// 4. winbase. Names and types may be incorrect.
// 5. rev.

#include <phnt_ntdef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)
#include <ntnls.h>
#endif

#include <ntkeapi.h>
#include <ntldr.h>
#include <ntexapi.h>

#include <ntbcd.h>
#include <ntmmapi.h>
#include <ntobapi.h>
#include <ntpsapi.h>

#if (PHNT_MODE != PHNT_MODE_KERNEL)
#include <cfg.h>
#include <ntdbg.h>
#include <ntioapi.h>
#include <ntlpcapi.h>
#include <ntpfapi.h>
#include <ntpnpapi.h>
#include <ntpoapi.h>
#include <ntregapi.h>
#include <ntrtl.h>
#include <ntimage.h>
#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)

#include <ntseapi.h>
#include <nttmapi.h>
#include <nttp.h>
#include <ntxcapi.h>

#include <ntwow64.h>

#include <ntlsa.h>
#include <ntsam.h>

#include <ntmisc.h>
#include <ntwmi.h>

#include <ntzwapi.h>

#endif

#ifdef __cplusplus
}
#endif

#endif
