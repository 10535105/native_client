{
  'includes': [
    'common.gypi',
  ],
  'targets': [
    {
      'target_name': 'pull_in_all',
      'type': 'none',
      'dependencies': [
        '../src/shared/gio/gio.gyp:*',
        '../src/shared/imc/imc.gyp:*',
        '../src/shared/platform/platform.gyp:*',
        '../src/shared/srpc/srpc.gyp:*',
        '../src/shared/utils/utils.gyp:*',
        '../src/trusted/debug_stub/debug_stub.gyp:*',
        '../src/trusted/desc/desc.gyp:*',
        '../src/trusted/gdb_rsp/gdb_rsp.gyp:*',
        '../src/trusted/nacl_base/nacl_base.gyp:*',
        '../src/trusted/nonnacl_util/nonnacl_util.gyp:*',
        '../src/trusted/perf_counter/perf_counter.gyp:*',
        '../src/trusted/platform_qualify/platform_qualify.gyp:*',
        '../src/trusted/sel_universal/sel_universal.gyp:*',
        '../src/trusted/service_runtime/service_runtime.gyp:*',
        '../src/untrusted/irt/irt.gyp:irt_core_nexe',
        '../src/untrusted/irt_stub/irt_stub.gyp:*',
        '../src/untrusted/nacl/nacl.gyp:*',
        '../src/untrusted/nosys/nosys.gyp:*',
        '../src/untrusted/pthread/pthread.gyp:*',
      ],
      'conditions': [
        # No tests on ARM yet.
        ['target_arch!="arm"', {
          'dependencies': [
            '../tests.gyp:*',
          ],
        }],
        ['target_arch=="arm"', {
          'dependencies': [
            '../src/trusted/validator_arm/validator_arm.gyp:*',
          ],
        }, {
          'dependencies': [
            '../src/trusted/validator_x86/validator_x86.gyp:*',
          ],
        }],
      ],
    },
  ],
}
