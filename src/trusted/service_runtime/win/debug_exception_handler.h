/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_WIN_DEBUG_EXCEPTION_HANDLER_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_WIN_DEBUG_EXCEPTION_HANDLER_H_

#include <windows.h>

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

struct NaClApp;

#define DEBUG_EXCEPTION_HANDLER_UNDER_DEBUGGER 0
#define DEBUG_EXCEPTION_HANDLER_SUCCESS 1
#define DEBUG_EXCEPTION_HANDLER_ERROR -1
#define DEBUG_EXCEPTION_HANDLER_NOT_SUPPORTED -2

/*
 * Runs debug loop until process exits or error occurs.
 * In case of error during debugging returns DEBUG_EXCEPTION_HANDLER_ERROR.
 * On Windows 64-bit returns DEBUG_EXCEPTION_HANDLER_NOT_SUPPORTED.
 * Otherwise returns DEBUG_EXCEPTION_HANDLER_SUCCESS and fills exit_code.
 *
 * TODO(mseaborn): Remove this function.  It is provided because
 * Chromium is currently using it, but Chromium will be switched over
 * to using NaClDebugExceptionHandlerRun().
 */
int NaClDebugLoop(HANDLE process_handle, DWORD *exit_code);

/*
 * This runs the debug exception handler in the current thread.  The
 * current thread should already have attached to the target process
 * through the Windows debug API by calling DebugActiveProcess() or by
 * calling CreateProcess() with DEBUG_PROCESS.
 *
 * In info/info_size this function expects to receive an array of
 * bytes that was passed to a NaClAttachDebugExceptionHandlerFunc
 * callback by NaClDebugExceptionHandlerEnsureAttached().
 */
void NaClDebugExceptionHandlerRun(HANDLE process_handle,
                                  void *info, size_t info_size);

/*
 * This requests that a debug exception handler be attached to the
 * current process, and returns whether this succeeded.
 */
int NaClDebugExceptionHandlerEnsureAttached(struct NaClApp *nap);

/*
 * This is an implementation of the
 * NaClAttachDebugExceptionHandlerFunc callback.  It attaches a debug
 * exception handler to the current process by launching sel_ldr with
 * --debug-exception-handler.
 */
int NaClDebugExceptionHandlerStandaloneAttach(void *info, size_t info_size);

/*
 * This implements sel_ldr's --debug-exception-handler option.
 */
int NaClDebugExceptionHandlerStandaloneMain(int argc, char **argv);

EXTERN_C_END

#endif /* NATIVE_CLIENT_SERVICE_RUNTIME_WIN_DEBUG_EXCEPTION_HANDLER_H_ */
