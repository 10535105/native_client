/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <windows.h>

#include "native_client/src/trusted/service_runtime/arch/sel_ldr_arch.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"
#include "native_client/src/trusted/service_runtime/sel_util.h"
#include "native_client/src/trusted/service_runtime/win/debug_exception_handler.h"
#include "native_client/src/trusted/service_runtime/win/thread_handle_map.h"


static int HandleBreakpointException(HANDLE thread_handle);
static int HandleException(HANDLE process_handle, HANDLE thread_handle);

struct ExceptionFrame {
  uint32_t return_addr;
  uint32_t prog_ctr;
  uint32_t stack_ptr;
};


static BOOL GetAddrProtection(HANDLE process_handle, uintptr_t addr,
                              int *protect) {
  MEMORY_BASIC_INFORMATION memory_info;
  if (VirtualQueryEx(process_handle, (void *) addr,
                     &memory_info, sizeof(memory_info))
      != sizeof(memory_info)) {
    return FALSE;
  }
  *protect = memory_info.Protect;
  return TRUE;
}

static BOOL ReadProcessMemoryChecked(HANDLE process_handle,
                                     const void *remote_addr,
                                     void *local_addr, size_t size) {
  size_t bytes_copied;
  return (ReadProcessMemory(process_handle, remote_addr, local_addr,
                            size, &bytes_copied)
          && bytes_copied == size);
}

static BOOL WriteProcessMemoryChecked(HANDLE process_handle, void *remote_addr,
                                      const void *local_addr, size_t size) {
  size_t bytes_copied;
  /*
   * WriteProcessMemory() does not always respect page permissions, so
   * we first manually check permissions to ensure that we are not
   * overwriting untrusted code.  For background, see
   * http://code.google.com/p/nativeclient/issues/detail?id=2572.  The
   * debuggee process should be suspended and unable to change page
   * mappings at the moment, so there should be no TOCTTOU race
   * condition with this check.
   */
  uintptr_t page_start = NaClTruncPage((uintptr_t) remote_addr);
  uintptr_t page_end = NaClRoundPage((uintptr_t) remote_addr + size);
  uintptr_t addr;
  for (addr = page_start; addr < page_end; addr += NACL_PAGESIZE) {
    int protect;
    if (!GetAddrProtection(process_handle, addr, &protect) ||
        protect != PAGE_READWRITE) {
      return FALSE;
    }
  }
  return (WriteProcessMemory(process_handle, remote_addr, local_addr,
                             size, &bytes_copied)
          && bytes_copied == size);
}


#define READ_MEM(process_handle, remote_ptr, local_ptr) \
  (sizeof(*remote_ptr) == sizeof(*local_ptr) && \
   ReadProcessMemoryChecked(process_handle, remote_ptr, local_ptr, \
                            sizeof(*local_ptr)))

#define WRITE_MEM(process_handle, remote_ptr, local_ptr) \
  (sizeof(*remote_ptr) == sizeof(*local_ptr) && \
   WriteProcessMemoryChecked(process_handle, remote_ptr, local_ptr, \
                             sizeof(*local_ptr)))


/*
 * DebugLoop below handles debug events in NaCl program.
 * CREATE_PROCESS, CREATE_THREAD and EXIT_THREAD events are used
 * to store thread handles. These thread handles are needed to get and set
 * thread context.
 *
 * EXCEPTION debug events are classified into 3 types. The BREAKPOINT event
 * that happens at attaching debugger to the program. All other trusted
 * exceptions are not handled. All untrusted exceptions are handled if
 * possible. The exception is called untrusted in both cs and ds are
 * non-standard. Debugger reads exception handler address and stack from NaCl
 * process and transfers control to the handler by changing thread context and
 * writing exception data to the NaCl process. Values of all registers are not
 * changed with the exception of eip and esp. When all data is written and
 * context have changed, debugger finishes exception debug event handling and
 * all threads in NaCl application continue execution. If anything goes wrong,
 * debugger doesn't handle exception and program terminates.
 *
 * Debugger have a special logic to prevent nested exceptions. It sets special
 * flag in NaCl program that must be cleared by exception handler. If exception
 * happens before that, debugger doesn't handle the exception and program
 * terminates.
 */

int NaClDebugLoop(HANDLE process_handle, DWORD *exit_code) {
  DEBUG_EVENT debug_event;
  int process_exited = 0;
  int error = 0;
  DWORD continue_status;
  ThreadHandleMap *map;
  HANDLE thread_handle;
  DWORD exception_code;
  map = CreateThreadHandleMap();
  while (!process_exited) {
    if (!WaitForDebugEvent(&debug_event, INFINITE)) {
      error = 1;
      break;
    }
    continue_status = DBG_CONTINUE;
    switch(debug_event.dwDebugEventCode) {
      case OUTPUT_DEBUG_STRING_EVENT:
      case UNLOAD_DLL_DEBUG_EVENT:
        break;
      case CREATE_PROCESS_DEBUG_EVENT:
        CloseHandle(debug_event.u.CreateProcessInfo.hFile);
        if (!ThreadHandleMapPut(map, debug_event.dwThreadId,
                                debug_event.u.CreateProcessInfo.hThread)) {
          error = 1;
        }
        break;
      case LOAD_DLL_DEBUG_EVENT:
        CloseHandle(debug_event.u.LoadDll.hFile);
        break;
      case CREATE_THREAD_DEBUG_EVENT:
        if (!ThreadHandleMapPut(map, debug_event.dwThreadId,
                                debug_event.u.CreateThread.hThread)) {
          error = 1;
        }
        break;
      case EXIT_THREAD_DEBUG_EVENT:
        ThreadHandleMapDelete(map, debug_event.dwThreadId);
        break;
      case EXIT_PROCESS_DEBUG_EVENT:
        process_exited = 1;
        break;
      case EXCEPTION_DEBUG_EVENT:
        continue_status = DBG_EXCEPTION_NOT_HANDLED;
        thread_handle = ThreadHandleMapGet(map, debug_event.dwThreadId);
        if (thread_handle == INVALID_HANDLE_VALUE) {
          error = 1;
        } else {
          exception_code =
              debug_event.u.Exception.ExceptionRecord.ExceptionCode;
          if (HandleException(process_handle, thread_handle)) {
            continue_status = DBG_CONTINUE;
          } else if (exception_code == EXCEPTION_BREAKPOINT &&
                     HandleBreakpointException(thread_handle)) {
            continue_status = DBG_CONTINUE;
          }
        }
        break;
    }
    if (error) {
      break;
    }
    if (!ContinueDebugEvent(
             debug_event.dwProcessId,
             debug_event.dwThreadId,
             continue_status)) {
      break;
    }
  }
  DestroyThreadHandleMap(map);
  if (error) {
    TerminateProcess(process_handle, -1);
    return DEBUG_EXCEPTION_HANDLER_ERROR;
  }
  GetExitCodeProcess(process_handle, exit_code);
  return DEBUG_EXCEPTION_HANDLER_SUCCESS;
}

/*
 * INT3 is not allowed in NaCl code but breakpoint exception
 * always happens when debugger attaches to the program or
 * starts program under debugging. This is the only exception
 * that must be handled in trusted code.
 */
static BOOL HandleBreakpointException(HANDLE thread_handle) {
  CONTEXT context;
  LDT_ENTRY cs_entry;
  DWORD cs_limit;
  context.ContextFlags = CONTEXT_CONTROL;
  if (!GetThreadContext(thread_handle, &context)) {
    return FALSE;
  }
  GetThreadSelectorEntry(thread_handle, context.SegCs, &cs_entry);
  cs_limit = cs_entry.LimitLow + (((int)cs_entry.HighWord.Bits.LimitHi) << 16);
  /* Segment limits are 4Gb in trusted code */
  return cs_limit == 0xfffff;
}


static BOOL GetThreadIndex(HANDLE process_handle, HANDLE thread_handle,
                           const CONTEXT *regs,
                           uint32_t *nacl_thread_index) {
  LDT_ENTRY cs_entry;
  LDT_ENTRY ds_entry;
  DWORD cs_limit;
  DWORD ds_limit;

  GetThreadSelectorEntry(thread_handle, regs->SegCs, &cs_entry);
  cs_limit = cs_entry.LimitLow + (((int)cs_entry.HighWord.Bits.LimitHi) << 16);
  GetThreadSelectorEntry(thread_handle, regs->SegDs, &ds_entry);
  ds_limit = ds_entry.LimitLow + (((int)ds_entry.HighWord.Bits.LimitHi) << 16);
  /* Segment limits are 4Gb in trusted code */
  if (cs_limit == 0xfffff || ds_limit == 0xfffff) {
    return FALSE;
  }
  *nacl_thread_index = regs->SegGs >> 3;
  return TRUE;
}

static BOOL HandleException(HANDLE process_handle, HANDLE thread_handle) {
  CONTEXT context;
  uint32_t nacl_thread_index;
  uintptr_t addr_space_size;
  uint32_t exception_stack;
  uint32_t exception_frame_end;
  uint32_t exception_flag;
  struct ExceptionFrame new_stack;
  /*
   * natp_remote and nap_remote point into the debuggee process's
   * address space, so cannot be dereferenced directly.  We use
   * pointer types for the convenience of calculating field offsets
   * and sizes.
   */
  struct NaClAppThread *natp_remote;
  struct NaClAppThread appthread_copy;
  struct NaClApp app_copy;

  context.ContextFlags = CONTEXT_SEGMENTS | CONTEXT_INTEGER | CONTEXT_CONTROL;
  if (!GetThreadContext(thread_handle, &context)) {
    return FALSE;
  }

  if (!GetThreadIndex(process_handle, thread_handle, &context,
                      &nacl_thread_index)) {
    /*
     * We can get here if the faulting thread is running trusted code
     * (an expected situation) or if an unexpected error occurred.
     */
    return FALSE;
  }

  /*
   * Note that this assumes that nacl_thread (a global array) is at
   * the same address in the debuggee process as in this debugger
   * process.  This assumes that the debuggee is running the same
   * executable/DLL as this process, and that address space
   * randomisation for a given executable/DLL does not vary across
   * processes (which is true on Windows).
   */
  if (!READ_MEM(process_handle, nacl_thread + nacl_thread_index,
                &natp_remote)) {
    return FALSE;
  }
  if (natp_remote == NULL) {
    /*
     * This means the nacl_thread and nacl_thread_ids arrays do not
     * match up.  TODO(mseaborn): Complain more noisily about such
     * unexpected cases and terminate the NaCl process.
     */
    return FALSE;
  }
  if (!READ_MEM(process_handle, natp_remote, &appthread_copy)) {
    return FALSE;
  }
  if (!READ_MEM(process_handle, appthread_copy.nap, &app_copy)) {
    return FALSE;
  }

  exception_stack = appthread_copy.user.exception_stack;
  exception_flag = appthread_copy.user.exception_flag;
  if (exception_flag) {
    return FALSE;
  }
  exception_flag = 1;
  if (!WRITE_MEM(process_handle, &natp_remote->user.exception_flag,
                 &exception_flag)) {
    return FALSE;
  }

  /*
   * Sanity check: Ensure the entry point is properly aligned.  We do
   * not expect this to fail because alignment was checked by the
   * syscall when the function was registered.
   */
  if (app_copy.exception_handler % NACL_INSTR_BLOCK_SIZE != 0) {
    return FALSE;
  }

  if (exception_stack == 0) {
    exception_stack = context.Esp;
  }
  /*
   * Calculate the position of the exception stack frame, with
   * suitable alignment.
   */
  exception_stack -= sizeof(struct ExceptionFrame) - NACL_STACK_PAD_BELOW_ALIGN;
  exception_stack = exception_stack & ~NACL_STACK_ALIGN_MASK;
  exception_stack -= NACL_STACK_PAD_BELOW_ALIGN;
  /*
   * Ensure that the exception frame would be written within the
   * bounds of untrusted address space.
   */
  exception_frame_end = exception_stack + sizeof(struct ExceptionFrame);
  if (exception_frame_end < exception_stack) {
    /* Unsigned overflow. */
    return FALSE;
  }
  addr_space_size = (uintptr_t) 1U << app_copy.addr_bits;
  if (exception_frame_end > addr_space_size) {
    return FALSE;
  }

  new_stack.return_addr = 0;
  new_stack.prog_ctr = context.Eip;
  new_stack.stack_ptr = context.Esp;
  if (!WRITE_MEM(process_handle,
                 (struct ExceptionFrame *) (app_copy.mem_start
                                            + exception_stack),
                 &new_stack)) {
    return FALSE;
  }

  context.Eip = app_copy.exception_handler;
  context.Esp = exception_stack;
  context.EFlags &= ~NACL_X86_DIRECTION_FLAG;
  return SetThreadContext(thread_handle, &context);
}

int NaClLaunchAndDebugItself(char *program, DWORD *exit_code) {
  PROCESS_INFORMATION process_information;
  STARTUPINFOA startup_info = {sizeof(STARTUPINFOA)};
  int result;
  if (IsDebuggerPresent()) {
    return DEBUG_EXCEPTION_HANDLER_UNDER_DEBUGGER;
  }
  if (0 == CreateProcessA(
               program,
               GetCommandLineA(),
               NULL,
               NULL,
               TRUE,
               DEBUG_PROCESS,
               NULL,
               NULL,
               &startup_info,
               &process_information)) {
    return DEBUG_EXCEPTION_HANDLER_ERROR;
  }
  CloseHandle(process_information.hThread);
  result = NaClDebugLoop(process_information.hProcess, exit_code);
  CloseHandle(process_information.hProcess);
  return result;
}
