/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

        .global exception_handler_wrapper

        .p2align 4
exception_handler_wrapper:
        /*
         * Record the state of some registers on entry to the
         * exception handler so that we can check that service_runtime
         * sets the correct values.  We cannot check this exactly from C.
         */
        movw ip, #:lower16:exception_handler_sp
        movt ip, #:upper16:exception_handler_sp
        bic ip, ip, #0xc0000000
        str sp, [ip]
        /*
         * Second bundle:
         */
        b exception_handler
