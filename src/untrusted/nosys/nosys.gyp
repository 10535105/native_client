# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


{
  'includes': [
    '../../../build/common.gypi',
  ],
  'variables': {
    'common_sources': [
      'access.c',
      'chdir.c',
      'chmod.c',
      'chown.c',
      'endpwent.c',
      'environ.c',
      'errno.c',
      '__errno.c',
      'execve.c',
      '_execve.c',
      'fcntl.c',
      'fchdir.c',
      'fchmod.c',
      'fchown.c',
      'fork.c',
      'ftruncate.c',
      'get_current_dir_name.c',
      'getcwd.c',
      'getegid.c',
      'geteuid.c',
      'getgid.c',
      'getlogin.c',
      'getrusage.c',
      'getppid.c',
      'getpwent.c',
      'getpwnam.c',
      'getpwnam_r.c',
      'getpwuid.c',
      'getpwuid_r.c',
      'getuid.c',
      'getwd.c',
      'ioctl.c',
      'issetugid.c',
      'isatty.c',
      'kill.c',
      'lchown.c',
      'link.c',
      'llseek.c',
      'lstat.c',
      'mkdir.c',
      'pclose.c',
      'pipe.c',
      'popen.c',
      'pselect.c',
      'pthread_cleanup_push.c',
      'pthread_cleanup_pop.c',
      'readlink.c',
      'remove.c',
      'rename.c',
      'rmdir.c',
      'select.c',
      'setegid.c',
      'seteuid.c',
      'setgid.c',
      'setpwent.c',
      'settimeofday.c',
      'setuid.c',
      'signal.c',
      'sigprocmask.c',
      'symlink.c',
      'system.c',
      'times.c',
      'tmpfile.c',
      'truncate.c',
      'ttyname.c',
      'ttyname_r.c',
      'umask.c',
      'unlink.c',
      'utime.c',
      'utimes.c',
      'vfork.c',
      'wait.c',
      'waitpid.c',
    ],
  },
  'targets' : [
    {
      'target_name': 'nosys_lib',
      'type': 'none',
      'variables': {
        'nlib_target': 'libnosys.a',
        'build_glibc': 0,
        'build_newlib': 1,
        'sources': ['<@(common_sources)']
      },
      'dependencies': [
        '<(DEPTH)/native_client/tools.gyp:prep_toolchain',
      ],
    },
  ],
}
