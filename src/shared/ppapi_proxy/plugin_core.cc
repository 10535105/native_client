/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include "native_client/src/shared/ppapi_proxy/plugin_core.h"
#include <stdio.h>
#include <map>
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/ppapi_proxy/plugin_globals.h"
#include "native_client/src/shared/ppapi_proxy/plugin_resource_tracker.h"
#include "native_client/src/shared/ppapi_proxy/plugin_upcall.h"
#include "native_client/src/shared/ppapi_proxy/utility.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/cpp/common.h"
#include "srpcgen/ppb_rpc.h"

using ppapi_proxy::DebugPrintf;

// All of the methods here are invoked from the plugin's main (UI) thread,
// so no locking is done.

namespace {

__thread bool is_main_thread = false;
bool main_thread_marked = false;

// Increment the reference count for a specified resource.  This only takes
// care of the plugin's reference count - the Resource should obtain the
// browser reference when it stores the browser's Resource id. When the
// Resource's reference count goes to zero, the destructor should make sure
// the browser reference is returned.
void AddRefResource(PP_Resource resource) {
  DebugPrintf("PPB_Core::AddRefResource: resource=%"NACL_PRIu32"\n",
              resource);
  if (ppapi_proxy::PluginResourceTracker::Get()->AddRefResource(resource))
    DebugPrintf("PPB_Core::AddRefResource: nonexistent resource");
}

void ReleaseResource(PP_Resource resource) {
  DebugPrintf("PPB_Core::ReleaseResource: resource=%"NACL_PRIu32"\n",
              resource);
  if (ppapi_proxy::PluginResourceTracker::Get()->UnrefResource(resource))
    DebugPrintf("PPB_Core::ReleaseRefResource: nonexistent resource");
}

void* MemAlloc(size_t num_bytes) {
  DebugPrintf("PPB_Core::MemAlloc: num_bytes=%"NACL_PRIuS"\n",
              num_bytes);
  return malloc(num_bytes);
}

void MemFree(void* ptr) {
  DebugPrintf("PPB_Core::MemFree: ptr=%p\n", ptr);
  free(ptr);
}

PP_TimeTicks GetTime() {
  DebugPrintf("PPB_Core::GetTime\n");
  NaClSrpcChannel* channel = ppapi_proxy::GetMainSrpcChannel();
  double time;
  NaClSrpcError retval = PpbCoreRpcClient::PPB_Core_GetTime(channel, &time);
  if (retval != NACL_SRPC_RESULT_OK) {
    return static_cast<PP_Time>(-1.0);
  } else {
    return static_cast<PP_Time>(time);
  }
}

PP_TimeTicks GetTimeTicks() {
  DebugPrintf("PPB_Core::GetTime\n");
  NaClSrpcChannel* channel = ppapi_proxy::GetMainSrpcChannel();
  double time;
  // TODO(sehr): implement time ticks.
  NaClSrpcError retval = PpbCoreRpcClient::PPB_Core_GetTime(channel, &time);
  if (retval != NACL_SRPC_RESULT_OK) {
    return static_cast<PP_TimeTicks>(-1.0);
  } else {
    return static_cast<PP_TimeTicks>(time);
  }
}

PP_Bool IsMainThread() {
  DebugPrintf("PPB_Core::IsMainThread\n");
  return pp::BoolToPPBool(is_main_thread);
}

void CallOnMainThread(int32_t delay_in_milliseconds,
                      PP_CompletionCallback callback,
                      int32_t result) {
  NACL_UNTESTED();
  CHECK(!IsMainThread());  // TODO(polina): define such behavior.
  DebugPrintf("PPB_Core::CallOnMainThread: "
              "delay=%"NACL_PRIu32" result=%"NACL_PRIu32 "\n",
              delay_in_milliseconds, result);
  ppapi_proxy::PluginUpcallCoreCallOnMainThread(delay_in_milliseconds,
                                                callback,
                                                result);
}

}  // namespace

namespace ppapi_proxy {

const PPB_Core* PluginCore::GetInterface() {
  static const PPB_Core core_interface = {
    AddRefResource,
    ReleaseResource,
    MemAlloc,
    MemFree,
    GetTime,
    GetTimeTicks,
    CallOnMainThread,
    IsMainThread
  };
  return &core_interface;
}

void PluginCore::MarkMainThread() {
  if (main_thread_marked) {
    // A main thread was already designated.  Fail.
    NACL_NOTREACHED();
  } else {
    is_main_thread = true;
    // Setting this once works because the main thread will call this function
    // before calling any pthread_creates.  Hence the result is already
    // published before other threads might attempt to call it.
    main_thread_marked = true;
  }
}


}  // namespace ppapi_proxy
