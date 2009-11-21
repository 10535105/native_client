/*
 * Copyright 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * NaCl Service Runtime.  Transferrable shared memory objects.
 */

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_platform.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/desc/linux/nacl_desc_sysv_shm.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#include "native_client/src/trusted/service_runtime/internal_errno.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/sel_util.h"


/*
 * This file contains the implementation of the NaClDescSysvShm
 * subclass of NaClDesc.
 *
 * NaClDescSysvShm is the subclass that wraps SysV shared memory descriptors.
 */

int NaClDescSysvShmImportCtor(struct NaClDescSysvShm  *self,
                              int                     id,
                              nacl_off64_t            size) {
  struct NaClDesc *basep = (struct NaClDesc *) self;

  /*
   * off_t is signed, but size_t are not; historically size_t is for
   * sizeof and similar, and off_t is also used for stat structure
   * st_size member.  This runtime test detects large object sizes
   * that are silently converted to negative values.  Additionally,
   * the size must be a multiple of 4K.
   */
  basep->vtbl = (struct NaClDescVtbl *) NULL;
  if (size < 0 || (size_t) size != NaClRoundPage((size_t) size)) {
    return 0;
  }

  if (!NaClDescCtor(basep)) {
    return 0;
  }
  self->id = id;
  self->size = size;
  basep->vtbl = &kNaClDescSysvShmVtbl;
  return 1;
}

/*
 * Creates a NaClDesc containing a new shared memory region.
 */
int NaClDescSysvShmCtor(struct NaClDescSysvShm  *self,
                        nacl_off64_t            size) {
  int id;
  int retval;

  /* We only allow multiples of 64K for NaCl-created region sizes. */
  if ((uintptr_t) size != NaClRoundAllocPage((uintptr_t) size)) {
    return 0;
  }
  /* Create the region. */
  id = shmget(IPC_PRIVATE,
              (int) size,
              IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
  if (-1 == id) {
    return 0;
  }
  /* Construct the descriptor. */
  retval = NaClDescSysvShmImportCtor(self, id, size);
  /* If the constructor failed, mark the region for freeing. */
  if (0 == retval) {
    shmctl(id, IPC_RMID, NULL);
    return 0;
  }
  /* Return success. */
  return 1;
}

void NaClDescSysvShmDtor(struct NaClDesc *vself) {
  struct NaClDescSysvShm  *self = (struct NaClDescSysvShm *) vself;

  /* NACL_INVALID_HANDLE is also an invalid id for shmat. */
  self->id = NACL_INVALID_HANDLE;
  vself->vtbl = (struct NaClDescVtbl *) NULL;
  NaClDescDtor(vself);
}

uintptr_t NaClDescSysvShmMap(struct NaClDesc         *vself,
                             struct NaClDescEffector *effp,
                             void                    *start_addr,
                             size_t                  len,
                             int                     prot,
                             int                     flags,
                             nacl_off64_t            offset) {
  struct NaClDescSysvShm  *self = (struct NaClDescSysvShm *) vself;

  int           nacl_flags;
  void          *result;

  /*
   * shm must have NACL_ABI_MAP_SHARED in flags, and all calls through
   * this API must supply a start_addr, so NACL_ABI_MAP_FIXED is
   * assumed.
   */
  UNREFERENCED_PARAMETER(flags);
  UNREFERENCED_PARAMETER(effp);
  /*
   * shmat can only map the shared memory region starting at its beginning.
   */
  if (0 != offset || self->size != len) {
    NaClLog(LOG_INFO,
            "NaClDescSysvShmMap: Mapping at non-zero offset or length"
            " mismatch\n");
    return -NACL_ABI_EINVAL;
  }
  /*
   * prot must be not be PROT_NONE nor contain other than PROT_{READ|WRITE}
   */
  if (NACL_ABI_PROT_NONE == prot) {
    NaClLog(LOG_INFO, "NaClDescSysvShmMap: PROT_NONE not supported\n");
    return -NACL_ABI_EINVAL;
  }
  if (0 == (NACL_ABI_PROT_READ & prot)) {
    NaClLog(LOG_INFO, "NaClDescSysvShmMap: PROT_READ not set\n");
    return -NACL_ABI_EINVAL;
  }
  if (0 != (~(NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE) & prot)) {
    NaClLog(LOG_INFO,
            "NaClDescSysvShmMap: prot has other bits than PROT_{READ|WRITE}\n");
    return -NACL_ABI_EINVAL;
  }
  /*
   * Map from NACL_ABI_ prot and flags bits to shmat flags.
   */
  nacl_flags = SHM_REMAP;
  if (NACL_ABI_PROT_READ == prot) {
    nacl_flags |= SHM_RDONLY;
  }

  /*
   * Attach (map) the shared memory region.
   */
  result = shmat(self->id, (void *) start_addr, nacl_flags);
  if (NACL_MAP_FAILED == result) {
    NaClLog(LOG_FATAL, "NaClDescSysvMap: shmat failed %d\n", errno);
  }
  if (result != start_addr) {
    NaClLog(LOG_FATAL,
            ("NaClDescSysvShmMap: NACL_MAP_FIXED but"
             " got 0x%08"PRIxPTR" instead of 0x%08"PRIxPTR"\n"),
            (uintptr_t) result, (uintptr_t) start_addr);
  }

  /*
   * If the id does not specify a region that is a multiple of 64K,
   * we need to mmap a region after the id mapping.
   */
  if (!NaClIsAllocPageMultiple((uintptr_t) len)) {
    int tail_prot = PROT_READ;
    void* tail_addr = (void*) ((uintptr_t) start_addr + len);
    size_t tail_size = NaClRoundAllocPage((uintptr_t) len) - (uintptr_t) len;

    if (0 != (NACL_ABI_PROT_READ & prot)) {
      tail_prot |= PROT_WRITE;
    }

    if (MAP_FAILED == mmap(tail_addr,
                           tail_size,
                           tail_prot,
                           MAP_PRIVATE | MAP_ANONYMOUS,
                           -1,
                           0)) {
      NaClLog(LOG_FATAL, ("NaClDescSysvShmMap: Couldn't map the tail page"));
    }
  }

  return (uintptr_t) start_addr;
}

int NaClDescSysvShmUnmapCommon(struct NaClDesc         *vself,
                              struct NaClDescEffector  *effp,
                              void                     *start_addr,
                              size_t                   len,
                              int                      safe_mode) {
  uintptr_t addr;
  uintptr_t end_addr;

  UNREFERENCED_PARAMETER(vself);
  /*
   * Note: we cannot detach from the shared memory region for risk of
   * creating a security hole.  However, from Bennet:
   * "0 == safe_mode should actually unmap. This is used by UnmapUnsafe the
   * callers of which are acknowledging that it creates a squatting timing
   * hole. This mode is currently only used in NaClApp dtor cleanup on linux,
   * so it's not critical -- We don't use that code since the service runtime
   * _exit when the NaCl module exits, but we may in the future need this. A
   * TODO/WARNING would suffice. Since w/ sysv shm there's no way to unmap,
   * we'd have to do it by side-effect: mmap anonymous memory on top, then
   * munmap that."
   *
   * TODO(sehr,bsy): in unsafe mode mmap then unmap.
   */
  if (0 == safe_mode) {
    return 0;
  }

  for (addr = (uintptr_t) start_addr, end_addr = addr + len;
       addr < end_addr;
       addr += NACL_MAP_PAGESIZE) {
    /* there's still a race condition */
    if (NaClIsNegErrno((*effp->vtbl->MapAnonymousMemory)(effp,
                                                         addr,
                                                         NACL_MAP_PAGESIZE,
                                                         PROT_NONE))) {
      NaClLog(LOG_ERROR, "NaClDescSysvShmUnmapCommon: could not fill hole\n");
      return -NACL_ABI_E_MOVE_ADDRESS_SPACE;
    }
  }
  return 0;
}

int NaClDescSysvShmUnmapUnsafe(struct NaClDesc         *vself,
                               struct NaClDescEffector *effp,
                               void                    *start_addr,
                               size_t                  len) {
  return NaClDescSysvShmUnmapCommon(vself, effp, start_addr, len, 0);
}

int NaClDescSysvShmUnmap(struct NaClDesc         *vself,
                        struct NaClDescEffector *effp,
                        void                    *start_addr,
                        size_t                  len) {
  return NaClDescSysvShmUnmapCommon(vself, effp, start_addr, len, 1);
}

int NaClDescSysvShmFstat(struct NaClDesc         *vself,
                        struct NaClDescEffector *effp,
                        struct nacl_abi_stat    *stbp) {
  struct NaClDescSysvShm  *self = (struct NaClDescSysvShm *) vself;

  UNREFERENCED_PARAMETER(effp);

  if (self->size > INT32_MAX) {
    return -NACL_ABI_EOVERFLOW;
  }

  stbp->nacl_abi_st_dev = 0;
  stbp->nacl_abi_st_ino = 0x6c43614e;
  stbp->nacl_abi_st_mode = (NACL_ABI_S_IFSHM |
                            NACL_ABI_S_IRUSR |
                            NACL_ABI_S_IWUSR);
  stbp->nacl_abi_st_nlink = 1;
  stbp->nacl_abi_st_uid = -1;
  stbp->nacl_abi_st_gid = -1;
  stbp->nacl_abi_st_rdev = 0;
  /* TODO(sehr): this should really use shmctl instead. */
  stbp->nacl_abi_st_size = (nacl_abi_off_t) self->size;
  stbp->nacl_abi_st_blksize = 0;
  stbp->nacl_abi_st_blocks = 0;
  stbp->nacl_abi_st_atime = 0;
  stbp->nacl_abi_st_mtime = 0;
  stbp->nacl_abi_st_ctime = 0;

  return 0;
}

int NaClDescSysvShmClose(struct NaClDesc         *vself,
                        struct NaClDescEffector *effp) {
  UNREFERENCED_PARAMETER(effp);

  NaClDescUnref(vself);
  return 0;
}

int NaClDescSysvShmExternalizeSize(struct NaClDesc *vself,
                                   size_t          *nbytes,
                                   size_t          *nhandles) {
  struct NaClDescSysvShm  *self = (struct NaClDescSysvShm *) vself;

  *nbytes = sizeof self->id + sizeof(nacl_off64_t);
  *nhandles = 0;

  return 0;
}

int NaClDescSysvShmExternalize(struct NaClDesc           *vself,
                               struct NaClDescXferState  *xfer) {
  struct NaClDescSysvShm  *self = (struct NaClDescSysvShm *) vself;
  nacl_off64_t size64 = self->size;

  memcpy(xfer->next_byte, &self->id, sizeof self->id);
  xfer->next_byte += sizeof self->id;
  memcpy(xfer->next_byte, &size64, sizeof size64);
  xfer->next_byte += sizeof size64;
  return 0;
}

struct NaClDescVtbl const kNaClDescSysvShmVtbl = {
  NaClDescSysvShmDtor,
  NaClDescSysvShmMap,
  NaClDescSysvShmUnmapUnsafe,
  NaClDescSysvShmUnmap,
  NaClDescReadNotImplemented,
  NaClDescWriteNotImplemented,
  NaClDescSeekNotImplemented,
  NaClDescIoctlNotImplemented,
  NaClDescSysvShmFstat,
  NaClDescSysvShmClose,
  NaClDescGetdentsNotImplemented,
  NACL_DESC_SYSV_SHM,
  NaClDescSysvShmExternalizeSize,
  NaClDescSysvShmExternalize,
  NaClDescLockNotImplemented,
  NaClDescTryLockNotImplemented,
  NaClDescUnlockNotImplemented,
  NaClDescWaitNotImplemented,
  NaClDescTimedWaitAbsNotImplemented,
  NaClDescSignalNotImplemented,
  NaClDescBroadcastNotImplemented,
  NaClDescSendMsgNotImplemented,
  NaClDescRecvMsgNotImplemented,
  NaClDescConnectAddrNotImplemented,
  NaClDescAcceptConnNotImplemented,
  NaClDescPostNotImplemented,
  NaClDescSemWaitNotImplemented,
  NaClDescGetValueNotImplemented,
};

int NaClDescSysvShmInternalize(struct NaClDesc           **baseptr,
                               struct NaClDescXferState  *xfer) {
  int                   rv;
  struct NaClDescSysvShm *ndisp;
  int                   id;
  nacl_off64_t          size;

  rv = -NACL_ABI_EIO;
  ndisp = NULL;

  if (xfer->next_byte + sizeof ndisp->id + sizeof ndisp->size >
      xfer->byte_buffer_end) {
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }

  ndisp = malloc(sizeof *ndisp);
  if (NULL == ndisp) {
    rv = -NACL_ABI_ENOMEM;
    goto cleanup;
  }

  memcpy(&id, xfer->next_byte, sizeof id);
  xfer->next_byte += sizeof id;
  memcpy(&size, xfer->next_byte, sizeof size);
  xfer->next_byte += sizeof size;

  if (!NaClDescSysvShmImportCtor(ndisp, id, size)) {
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }

  *baseptr = (struct NaClDesc *) ndisp;
  rv = 0;

cleanup:
  if (rv < 0) {
    free(ndisp);
  }
  return rv;
}
