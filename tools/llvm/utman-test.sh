#!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
#@                 Untrusted Toolchain Test Helper
#@-------------------------------------------------------------------
#@ It MUST be run from the native_client/ directory.
#@
#@ The env variables: UTMAN_CONCURRENCY, UTMAN_BUILDBOT, UTMAN_DEBUG
#@ control behavior of this script
#@

######################################################################
# Config
######################################################################

set -o nounset
set -o errexit

# The script is located in "native_client/tools/llvm".
# Set pwd to native_client/
cd "$(dirname "$0")"/../..
if [[ $(basename "$(pwd)") != "native_client" ]] ; then
  echo "ERROR: cannot find native_client/ directory"
  exit -1
fi
readonly NACL_ROOT="$(pwd)"

source tools/llvm/common-tools.sh
SetScriptPath "${NACL_ROOT}/tools/llvm/utman-test.sh"
# For different levels of make parallelism change this in your env
readonly UTMAN_CONCURRENCY=${UTMAN_CONCURRENCY:-8}

######################################################################
######################################################################
#
#   < TESTING >
#
######################################################################
######################################################################

# TODO(robertm): figure out what to do about concurrency in debug mode.
# Perhaps it is fine just tweaking that via UTMAN_CONCURRENCY.
if ${UTMAN_DEBUG} || ${UTMAN_BUILDBOT}; then
  readonly SCONS_ARGS=(MODE=nacl,opt-host
                       bitcode=1
                       sdl=none
                       --verbose
                       -j${UTMAN_CONCURRENCY})

  readonly SCONS_ARGS_SEL_LDR=(MODE=opt-host
                               bitcode=1
                               sdl=none
                               --verbose
                               -j${UTMAN_CONCURRENCY})
else
  readonly SCONS_ARGS=(MODE=nacl,opt-host
                       bitcode=1
                       naclsdk_validate=0
                       sdl=none
                       sysinfo=0
                       -j${UTMAN_CONCURRENCY})

  readonly SCONS_ARGS_SEL_LDR=(MODE=opt-host
                               bitcode=1
                               naclsdk_validate=0
                               sdl=none
                               sysinfo=0
                               -j${UTMAN_CONCURRENCY})
fi

#@ show-tests            - see what tests can be run
show-tests() {
  StepBanner "SHOWING TESTS"
  cat $(find tests -name nacl.scons) | grep -o 'run_[A-Za-z_-]*' | sort | uniq
}

#+ scons-determine-tests  - returns:
#+    (a) "true smoke_tests [-k]" if all smoke tests should be built and run.
#+ or (b) "false $@" if not all tests should be built because specific tests
#+        are already identified in $@. The test must be the first element
#+        of $@, but we don't check that here.
scons-determine-tests() {
  if [ $# -eq 0 ] || ([ $# -eq 1 ] && [ "$1" == "-k" ]); then
    echo "true smoke_tests $@" # $@ should only tack on the -k flag or nothing
  else
    echo "false $@"
  fi
}

scons-build-sel_ldr() {
  local  platform=$1
  ./scons platform=${platform} ${SCONS_ARGS_SEL_LDR[@]} sel_ldr
}

scons-build-sel_universal() {
  local  platform=$1
  ./scons platform=${platform} ${SCONS_ARGS_SEL_LDR[@]} sel_universal
}

scons-clean-pnacl-build-dir () {
  rm -rf scons-out/nacl-$1-pnacl
}

scons-clean-pnacl-pic-build-dir () {
  rm -rf scons-out/nacl-$1-pnacl-pic
}

scons-pnacl-build () {
  local platform=$1
  shift
  ./scons ${SCONS_ARGS[@]} \
          platform=${platform} \
          "$@"
}

run-scons-tests() {
  local platform=$1
  local should_build_all=$2
  local testname=$3
  shift 3
  # The rest of the arguments should be flags!

  # See if we should build all the tests.
  if ${should_build_all}; then
    scons-pnacl-build ${platform} $@
  fi

  # Then run the listed tests.
  scons-pnacl-build ${platform} ${testname} $@
}

test-scons-common () {
  local platform=$1
  shift
  scons-clean-pnacl-build-dir ${platform}

  test_setup=$(scons-determine-tests "$@")
  run-scons-tests ${platform} ${test_setup}
}

test-scons-pic-common () {
  local platform=$1
  shift
  scons-clean-pnacl-pic-build-dir ${platform}

  test_setup=$(scons-determine-tests "$@")
  run-scons-tests ${platform} ${test_setup} nacl_pic=1
}

#@ test-arm              - run arm tests via pnacl toolchain
#@ test-arm <test>       - run a single arm test via pnacl toolchain
test-arm() {
  test-scons-common arm "$@"
}

#@ test-x86-32           - run x86-32 tests via pnacl toolchain
#@ test-x86-32 <test>    - run a single x86-32 test via pnacl toolchain
test-x86-32() {
  test-scons-common x86-32 "$@"
}

#@ test-x86-64           - run all x86-64 tests via pnacl toolchain
#@ test-x86-64 <test>    - run a single x86-64 test via pnacl toolchain
test-x86-64() {
  test-scons-common x86-64 "$@"
}

#@ test-arm-pic           - run all arm pic tests via pnacl toolchain
#@ test-arm-pic <test>    - run a single arm pic test via pnacl toolchain
test-arm-pic() {
  test-scons-pic-common arm "$@"
}

#@ test-x86-32-pic        - run all x86-32 pic tests via pnacl toolchain
#@ test-x86-32-pic <test> - run a single x86-32 pic test via pnacl toolchain
test-x86-32-pic() {
  test-scons-pic-common x86-32 "$@"
}

#@ test-x86-64-pic        - run all x86-64 pic tests via pnacl toolchain
#@ test-x86-64-pic <test> - run a single x86-64 pic test via pnacl toolchain
test-x86-64-pic() {
  test-scons-pic-common x86-64 "$@"
}

#@ test-all              - run arm, x86-32, and x86-64 tests. (all should pass)
#@ test-all <test>       - run a single test on all architectures.
test-all() {
  if [ $# -eq 1 ] && [ "$1" == "-k" ]; then
    echo "Using -k on test-all is not a good idea."
    exit -1
  fi

  test-arm "$@"
  test-arm-pic "$@"
  test-x86-64 "$@"
  test-x86-64-pic "$@"
  test-x86-32 "$@"
  test-x86-32-pic "$@"
}


#@ test-spec <official-spec-dir> <setup> [ref|train] [<benchmarks>]*
#@                       - run spec tests
#@
test-spec() {
  if [[ $# -lt 2 ]]; then
    echo "not enough arguments for test-spec"
    exit 1
  fi;
  official=$(GetAbsolutePath $1)
  setup=$2
  shift 2
  spushd tests/spec2k
  ./run_all.sh CleanBenchmarks "$@"
  ./run_all.sh PopulateFromSpecHarness ${official} "$@"
  ./run_all.sh BuildAndRunBenchmarks ${setup} "$@"
  spopd
}

#@ CollectTimingInfo <directory> <timing_result_file> <tagtype...>
#@  CD's into the directory in a subshell and collects all the
#@  relevant timed run info
#@  tagtype just gets printed out.
CollectTimingInfo() {
  wd=$1
  result_file=$2
  setup=$3
  (cd ${wd};
   mkdir -p $(dirname ${result_file})
   echo "##################################################" >>${result_file}
   date +"# Completed at %F %H:%M:%S %A ${result_file}" >> ${result_file}
   echo "# " ${wd}
   echo "#" $(uname -a) >> ${result_file}
   echo "# SETUP: ${setup}" >>${result_file}
   echo "##################################################" >>${result_file}
   echo "# COMPILE " >> ${result_file}
   for ff in $(find . -name "*.compile_time"); do
     cat ${ff} >> ${result_file}
   done
   echo "# RUN " >> ${result_file}
   for ff in $(find . -name "*.run_time"); do
     cat ${ff} >> ${result_file}
   done
   cat ${result_file}
  )
}


#@ timed-test-spec <result-file> <official-spec-dir> <setup> ... - run spec and
#@  measure time / size data. Data is emitted to stdout, but also collected
#@  in <result-file>. <result-file> is not cleared across runs (but temp files
#@  are cleared on each run).
#@  Note that the VERIFY variable effects the timing!
timed-test-spec() {
  if ${BUILD_PLATFORM_MAC} ; then
    echo "Timed-test-spec is not currently supported on MacOS"
    exit -1
  fi
  if [ "$#" -lt "3" ]; then
    echo "timed-test-spec {result-file} {spec2krefdir} {setupfunc}" \
         "[ref|train] [benchmark]*"
    exit 1
  fi
  result_file=$1
  official=$(GetAbsolutePath $2)
  setup=$3
  shift 3
  spushd tests/spec2k
  ./run_all.sh CleanBenchmarks "$@"
  ./run_all.sh PopulateFromSpecHarness ${official} "$@"
  ./run_all.sh TimedBuildAndRunBenchmarks ${setup} "$@"
  CollectTimingInfo $(pwd) ${result_file} ${setup}
  spopd
}


#@ test-bot-base         - tests that must pass on the bots to validate a TC
test-bot-base() {
  # TODO(robertm): this is missing adhoc tests.
  test-all
}


#@ help                  - Usage information.
help() {
  Usage
}

#@ help-full             - Usage information including internal functions.
help-full() {
  Usage2
}

######################################################################
######################################################################
#
#                               < MAIN >
#
######################################################################
######################################################################

[ $# = 0 ] && set -- help  # Avoid reference to undefined $1.
if [ "$(type -t $1)" != "function" ]; then
  #Usage
  echo "ERROR: unknown function '$1'." >&2
  echo "For help, try:"
  echo "    $0 help"
  exit 1
fi

"$@"
