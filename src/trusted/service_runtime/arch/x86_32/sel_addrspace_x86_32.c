/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#if NACL_LINUX
#include <errno.h>
#include <sys/mman.h>
#endif

#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/sel_addrspace.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


NaClErrorCode NaClAllocateSpace(void **mem, size_t addrsp_size) {
  int result;

  CHECK(NULL != mem);

  NaClAddrSpaceBeforeAlloc(addrsp_size);

#if NACL_LINUX
  /*
   * On 32 bit Linux, a 1 gigabyte block of address space may be reserved at
   * the zero-end of the address space during process creation, to address
   * sandbox layout requirements on ARM and performance issues on Intel ATOM.
   * Look for this prereserved block and if found, pass its address to the
   * page allocation function.
   */
  if (NaClFindPrereservedSandboxMemory(mem, addrsp_size)) {
    void *tmp_mem = (void *) NACL_TRAMPOLINE_START;
    CHECK(*mem == 0);
    addrsp_size -= NACL_TRAMPOLINE_START;
    result = NaCl_page_alloc_at_addr(&tmp_mem, addrsp_size);
  } else {
    /* Zero-based sandbox not prereserved. Attempt to allocate anyway. */
    result = NaCl_page_alloc(mem, addrsp_size);
  }
#elif NACL_WINDOWS
  /*
   * On 32 bit Windows, a 1 gigabyte block of address space is reserved before
   * starting up this process to make sure we can create the sandbox. Look for
   * this prereserved block and if found, pass its address to the page
   * allocation function.
   */
  if (0 == NaClFindPrereservedSandboxMemory(mem, addrsp_size)) {
    result = NaCl_page_alloc_at_addr(mem, addrsp_size);
  } else {
    result = NaCl_page_alloc(mem, addrsp_size);
  }
#else
  result = NaCl_page_alloc(mem, addrsp_size);
#endif

  if (0 != result) {
    NaClLog(2,
        "NaClAllocateSpace: NaCl_page_alloc 0x%08"NACL_PRIxPTR
        " failed\n",
        (uintptr_t) *mem);
    return LOAD_NO_MEMORY;
  }
  NaClLog(4, "NaClAllocateSpace: %"NACL_PRIxPTR", %"NACL_PRIxS"\n",
          (uintptr_t) *mem,
          addrsp_size);

  return LOAD_OK;
}


NaClErrorCode NaClMprotectGuards(struct NaClApp *nap) {
  uintptr_t start_addr;
  int       err;

  start_addr = nap->mem_start;

  if (0 == start_addr) {
#if !NACL_LINUX
    NaClLog(LOG_FATAL, ("NaClMprotectGuards: zero-based sandbox is"
                        " supported on Linux only.\n"));
#endif
  } else {
    err = NaCl_mprotect((void *) start_addr,
                        NACL_SYSCALL_START_ADDR,
                        PROT_NONE);
    if (err != 0) {
      NaClLog(LOG_ERROR, ("NaClMemoryProtection:"
                          " NaCl_mprotect(0x%08"NACL_PRIxPTR","
                          " 0x%08x, 0x%x) failed,"
                          " error %d (NULL pointer guard page)\n"),
              start_addr, NACL_SYSCALL_START_ADDR, PROT_NONE,
              err);
      return LOAD_MPROTECT_FAIL;
    }
  }

  return LOAD_OK;
}
