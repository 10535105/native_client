#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import linecache
import os
import sys


# Import decode_dump for small testing.
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
TESTS_DIR = os.path.dirname(SCRIPT_DIR)
NACL_DIR = os.path.dirname(TESTS_DIR)
CRASH_DUMP_DIR = os.path.join(NACL_DIR, 'src', 'untrusted', 'crash_dump')
sys.path.insert(0, CRASH_DUMP_DIR)
import decode_dump


def CheckStackTrace(core, main_nexe, nmf_filename, addr2line, nacl_sdk_lib):
  """Check that a core dump yields an expected stack trace.

  Args:
    core: the core.json file emitted on crash.
    main_nexe: the main nexe in question.
    nmf_filename: the manifest for main_nexe.
    addr2line: location of addr2line for the relevant toolchain.
    nacl_sdk_lib: location of the system dylibs for this nexe.
  """
  decoder = decode_dump.CoreDecoder(
      main_nexe=main_nexe,
      nmf_filename=nmf_filename,
      addr2line=addr2line,
      toolchain_libs=nacl_sdk_lib)
  info = decoder.LoadAndDecode(core)
  trace = decoder.StackTrace(info)
  expected = [
      ('layer5', '*(volatile int *) x = y;'),
      ('layer4', 'layer5(x, 1);'),
      ('CallMe', 'func(arg);'),
      ('layer3', 'CallMe(layer4, a + b + c);'),
      ('layer2', 'layer3(i, j, 7);'),
      ('layer1', 'layer2(junk[0], t + 1);'),
  ]
  for i in range(len(expected)):
    scope = trace[i]
    lineno = int(scope['lineno'])
    line = linecache.getline(scope['filename'], lineno).strip()
    actual = (scope['function'], line)
    assert expected[i] == actual


if __name__ == '__main__':
  if len(sys.argv) != 6:
    sys.stderr.write(
        'USAGE: %s <core.json> <nexe> <nmf> <addr2line> <nacl_sdk_lib>\n' %
        sys.argv[0])
    sys.exit(1)
  CheckStackTrace(*sys.argv[1:])