/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/osx/mach_exception_handler.h"

#include <mach/mach.h>
#include <mach/thread_status.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "gen/native_client/src/trusted/service_runtime/nacl_exc.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_app.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_switch_to_app.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"

/* Only handle x86_32 for now. */
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32


/*
 * MIG generated message pump from /usr/include/mach/exc.defs
 * Tweaked to place in an isolated namespace.
 */
boolean_t nacl_exc_server(
    mach_msg_header_t *InHeadP,
    mach_msg_header_t *OutHeadP);


/* Exception types to intercept. */
#define NACL_MACH_EXCEPTION_MASK EXC_MASK_BAD_ACCESS
/*
 * TODO(bradnelson): eventually consider these too:
 *     EXC_MASK_BAD_INSTRUCTION
 *     EXC_MASK_ARITHMETIC
 *     EXC_MASK_BREAKPOINT
 */

/* TODO(bradnelson): merge this into a common location. */
struct ExceptionFrame {
  uint32_t return_addr;
  uint32_t prog_ctr;
  uint32_t stack_ptr;
};

struct MachExceptionParameters {
  mach_msg_type_number_t count;
  exception_mask_t masks[EXC_TYPES_COUNT];
  mach_port_t ports[EXC_TYPES_COUNT];
  exception_behavior_t behaviors[EXC_TYPES_COUNT];
  thread_state_flavor_t flavors[EXC_TYPES_COUNT];
};

struct MachExceptionHandlerData {
  struct MachExceptionParameters old_params;
  mach_port_t exception_port;
};


static int HandleException(mach_port_t thread_port,
                           exception_type_t exception) {
  mach_msg_type_number_t size;
  x86_thread_state_t regs;
  kern_return_t result;
  uint16_t trusted_cs = NaClGetGlobalCs();
  uint16_t trusted_ds = NaClGetGlobalDs();
  uint32_t nacl_thread_index;
  struct NaClApp *nap;
  struct NaClAppThread *natp;
  volatile struct ExceptionFrame *frame;
  uintptr_t frame_addr_user;
  uintptr_t frame_addr_sys;

  /* Ignore all but bad accesses for now. */
  if (exception != EXC_BAD_ACCESS) {
    return 0;
  }

  /* Capture the register state of the 'excepting' thread. */
  size = sizeof(regs) / sizeof(natural_t);
  result = thread_get_state(thread_port, x86_THREAD_STATE,
                            (void *) &regs, &size);
  if (result != KERN_SUCCESS) {
    return 0;
  }

  /*
   * If trusted_cs is 0 (which is not a usable segment selector), the
   * sandbox has not been initialised yet, so there can be no untrusted
   * code running.
   */
  if (trusted_cs == 0) {
    return 0;
  }

  /*
   * If the current code segment is the trusted one, we aren't in the
   * sandbox.
   * TODO(bradnelson): This makes the potentially false assumption that cs is
   *     the last thing to change when switching into untrusted code. We need
   *     tests to vet this.
   */
  if (regs.uts.ts32.__cs == trusted_cs) {
    return 0;
  }

  /*
   * We can get the thread index from the segment selector used for TLS
   * from %gs >> 3.
   * TODO(bradnelson): Migrate that knowledge to a single shared location.
   */
  nacl_thread_index = regs.uts.ts32.__gs >> 3;
  natp = nacl_thread[nacl_thread_index];
  nap = natp->nap;

  /* Don't handle it if the exception flag is set. */
  if (natp->user.exception_flag) {
    return 0;
  }
  /* Set the flag. */
  natp->user.exception_flag = 1;

  /* Get location of exception stack frame. */
  if (natp->user.exception_stack) {
    frame_addr_user = natp->user.exception_stack;
  } else {
    /* If not set default to user stack. */
    frame_addr_user = regs.uts.ts32.__esp;
  }

  /* Align stack frame properly. */
  frame_addr_user -= sizeof(struct ExceptionFrame) - NACL_STACK_PAD_BELOW_ALIGN;
  frame_addr_user &= ~NACL_STACK_ALIGN_MASK;
  frame_addr_user -= NACL_STACK_PAD_BELOW_ALIGN;

  /* Convert from user to system space. */
  frame_addr_sys = NaClUserToSysAddrRange(
      nap, frame_addr_user, sizeof(struct ExceptionFrame));
  if (frame_addr_sys == kNaClBadAddress) {
    return 0;
  }
  frame = (struct ExceptionFrame *) frame_addr_sys;

  /* Write out call stack frame for handler invocation. */
  frame->return_addr = 0;
  frame->prog_ctr = regs.uts.ts32.__eip;
  frame->stack_ptr = regs.uts.ts32.__esp;

  /* Set up thread context to resume at handler. */
  natp->user.new_prog_ctr = nap->exception_handler;
  natp->user.stack_ptr.ptr_32.ptr = frame_addr_user;

  /*
   * Put registers in right place to land at NaClSwitchNoSSEViaECX
   * This is required because:
   *   - For an unknown reason thread_set_state resets %cs to the default
   *     value, even when set to something else, in current XNU versions.
   *   - An examination of the XNU sources indicates
   *     that setting the code which state the thread state resets
   *     %cs, %ds, %es, %ss to their default values in some early versions.
   *     (For instance: xnu-792.6.22/osfmk/i386/pcb.c:616)
   * This precludes going directly to the untrusted handler.
   * Instead we call a variant of NaClSwitchNoSSE which takes a pointer
   * to the thread user context in %ecx.
   */
  regs.uts.ts32.__eip = (uint32_t) &NaClSwitchNoSSEViaECX;
  regs.uts.ts32.__cs = trusted_cs;
  regs.uts.ts32.__ecx = (uint32_t) &natp->user;
  regs.uts.ts32.__ds = trusted_ds;
  regs.uts.ts32.__es = trusted_ds;  /* just for good measure */
  regs.uts.ts32.__ss = trusted_ds;  /* just for good measure */
  result = thread_set_state(thread_port, x86_THREAD_STATE,
                            (void *) &regs, size);
  if (result != KERN_SUCCESS) {
    return 0;
  }

  /* Return success, and resume the thread. */
  return 1;
}


kern_return_t nacl_catch_exception_raise(
    mach_port_t exception_port,
    mach_port_t thread,
    mach_port_t task,
    exception_type_t exception,
    exception_data_t code,
    mach_msg_type_number_t code_count) {
  UNREFERENCED_PARAMETER(exception_port);
  UNREFERENCED_PARAMETER(task);
  UNREFERENCED_PARAMETER(code);
  UNREFERENCED_PARAMETER(code_count);

  if (HandleException(thread, exception)) {
    return KERN_SUCCESS;
  }

  /*
   * For now just exit with failure, which cause the failure to go to the
   * system. This will bypass breakpad.
   * TODO(bradnelson): Add exception forwarding once there's also a test.
   */
  return KERN_FAILURE;
}

kern_return_t nacl_catch_exception_raise_state(
    mach_port_t exception_port,
    exception_type_t exception,
    const exception_data_t code,
    mach_msg_type_number_t code_count,
    int *flavor,
    const thread_state_t old_state,
    mach_msg_type_number_t old_state_count,
    thread_state_t new_state,
    mach_msg_type_number_t *new_state_count) {
  /* MIG generated code expects this, but should never be called. */
  UNREFERENCED_PARAMETER(exception_port);
  UNREFERENCED_PARAMETER(exception);
  UNREFERENCED_PARAMETER(code);
  UNREFERENCED_PARAMETER(code_count);
  UNREFERENCED_PARAMETER(flavor);
  UNREFERENCED_PARAMETER(old_state);
  UNREFERENCED_PARAMETER(old_state_count);
  UNREFERENCED_PARAMETER(new_state);
  UNREFERENCED_PARAMETER(new_state_count);
  NaClLog(LOG_FATAL, "nacl_catch_exception_raise_state: "
                     "Unrequested message received.\n");
  return KERN_FAILURE;
}

kern_return_t nacl_catch_exception_raise_state_identity (
        mach_port_t exception_port,
        mach_port_t thread,
        mach_port_t task,
        exception_type_t exception,
        exception_data_t code,
        mach_msg_type_number_t code_count,
        int *flavor,
        thread_state_t old_state,
        mach_msg_type_number_t old_state_count,
        thread_state_t new_state,
        mach_msg_type_number_t *new_state_count) {
  /* MIG generated code expects this, but should never be called. */
  UNREFERENCED_PARAMETER(exception_port);
  UNREFERENCED_PARAMETER(thread);
  UNREFERENCED_PARAMETER(task);
  UNREFERENCED_PARAMETER(exception);
  UNREFERENCED_PARAMETER(code);
  UNREFERENCED_PARAMETER(code_count);
  UNREFERENCED_PARAMETER(flavor);
  UNREFERENCED_PARAMETER(old_state);
  UNREFERENCED_PARAMETER(old_state_count);
  UNREFERENCED_PARAMETER(new_state);
  UNREFERENCED_PARAMETER(new_state_count);
  NaClLog(LOG_FATAL, "nacl_catch_exception_raise_state_identity: "
                     "Unrequested message received.\n");
  return KERN_FAILURE;
}

static void *MachExceptionHandlerThread(void *arg) {
  struct MachExceptionHandlerData *data =
      (struct MachExceptionHandlerData *) arg;
  kern_return_t result;
  /* Have space for a fairly large mach messages. */
  struct {
    mach_msg_header_t header;
    mach_msg_body_t body;
    char padding[1024];
  } request;
  struct {
    mach_msg_header_t header;
    mach_msg_body_t body;
    mig_reply_error_t reply_error;
    mach_msg_trailer_t trailer;
  } reply;

  for (;;) {
    result = mach_msg(&request.header, MACH_RCV_MSG | MACH_RCV_LARGE, 0,
                      sizeof(request), data->exception_port,
                      MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    if (result != MACH_MSG_SUCCESS) {
      goto failure;
    }
    if (!nacl_exc_server(&request.header, &reply.header)) {
      goto failure;
    }
    result = mach_msg(&reply.header, MACH_SEND_MSG,
                      reply.header.msgh_size, 0,
                      MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    if (result != MACH_MSG_SUCCESS) {
      goto failure;
    }
  }

failure:
  free(data);

  return 0;
}

static int InstallHandler(struct MachExceptionHandlerData *data) {
  kern_return_t result;
  mach_port_t current_task = mach_task_self();

  /* Capture old handler info. */
  data->old_params.count = EXC_TYPES_COUNT;
  result = task_get_exception_ports(current_task, NACL_MACH_EXCEPTION_MASK,
                                    data->old_params.masks,
                                    &data->old_params.count,
                                    data->old_params.ports,
                                    data->old_params.behaviors,
                                    data->old_params.flavors);
  if (result != KERN_SUCCESS) {
    return result;
  }

  /* TODO(bradnelson): decide if we should set the exception port per thread. */
  /* Direct all task exceptions to new exception port. */
  result = task_set_exception_ports(current_task, NACL_MACH_EXCEPTION_MASK,
                                    data->exception_port, EXCEPTION_DEFAULT,
                                    THREAD_STATE_NONE);
  if (result != KERN_SUCCESS) {
    return result;
  }

  return KERN_SUCCESS;
}

int NaClInterceptMachExceptions(void) {
  struct MachExceptionHandlerData *data;
  kern_return_t result;
  mach_port_t current_task;
  pthread_attr_t attr;
  int thread_result;
  pthread_t exception_handler_thread;

  current_task = mach_task_self();

  /* Allocate structure to share with exception handler thread. */
  data = (struct MachExceptionHandlerData *) calloc(1, sizeof(*data));
  if (data == NULL) {
    goto failure;
  }
  data->exception_port = MACH_PORT_NULL;

  /* Allocate port to receive exceptions. */
  result = mach_port_allocate(current_task, MACH_PORT_RIGHT_RECEIVE,
                              &data->exception_port);
  if (result != KERN_SUCCESS) {
    goto failure;
  }

  /* Add the right to send. */
  result = mach_port_insert_right(current_task,
                                  data->exception_port, data->exception_port,
                                  MACH_MSG_TYPE_MAKE_SEND);
  if (result != KERN_SUCCESS) {
    goto failure;
  }

  /* Install handler. */
  result = InstallHandler(data);
  if (result != KERN_SUCCESS) {
    goto failure;
  }

  /* Create handler thread. */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  thread_result = pthread_create(&exception_handler_thread, &attr,
                                 &MachExceptionHandlerThread, data);
  pthread_attr_destroy(&attr);
  if (thread_result) {
    goto failure;
  }

  return TRUE;

failure:
  if (data) {
    if (MACH_PORT_NULL != data->exception_port) {
      mach_port_deallocate(current_task, data->exception_port);
    }
    free(data);
  }
  return FALSE;
}

#else  /* NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32 */

int NaClInterceptMachExceptions(void) {
  return FALSE;
}

#endif  /* NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32 */