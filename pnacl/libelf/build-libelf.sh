#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
##########################################################################
#
# Build and install libelf (part of elfutils).
# Installs libelf.a and associated headers.
#
# The elfutils sources were pulled from:
#     https://fedorahosted.org/releases/e/l/elfutils/0.153/
#
# This script expects the following variables to be set in the environment:
#
#   DEST_DIR   (absolute path)
#   CC
#   AR
#   RANLIB
#   EXTRA_HEADERS (true/false)
#
# The destination directory will be used with the following layout:
#   source/  - Untar'd source
#   build/   - Build directory
#   install/ - Install directory
#

set -u
set -e
set -x

# Directory where this script resides.
pushd "$(dirname "$0")"
readonly ROOT_DIR="$(pwd)"
popd

# Where the elfutil source will be untar'd
readonly SOURCE_DIR="${DEST_DIR}/source"
readonly BUILD_DIR="${DEST_DIR}/build"
readonly INSTALL_DIR="${DEST_DIR}/install"
readonly LABEL="elfutils-0.153"
readonly THIRD_PARTY_ELFUTILS="${ROOT_DIR}/../../../third_party/elfutils"

CFLAGS="${CFLAGS:-}"
# Extra headers for the pnacl/darwin build
if ${EXTRA_HEADERS}; then
  CFLAGS+=" -I${ROOT_DIR}/include -DHAVE_UNDEF"
fi

readonly BUILD_ENV=(CC="${CC} ${CFLAGS}"
                    AR="${AR}"
                    RANLIB="${RANLIB}"
                    PATH="${PATH}")

Clean() {
  rm -rf "${DEST_DIR}"
}

UntarAndPatch() {
  mkdir -p "${DEST_DIR}"
  tar -jxvof "${THIRD_PARTY_ELFUTILS}"/${LABEL}.tar.bz2 -C "${DEST_DIR}"
  mv "${DEST_DIR}"/${LABEL} "${SOURCE_DIR}"
  pushd "${SOURCE_DIR}"
  patch -p1 < "${THIRD_PARTY_ELFUTILS}"/elfutils-portability.patch
  patch -p1 < "${ROOT_DIR}"/elfutils-pnacl.patch
  patch -p1 < "${ROOT_DIR}"/elfutils-mem.patch
  popd
}

RunConfigure() {
  mkdir -p "${BUILD_DIR}"
  pushd "${BUILD_DIR}"
  env -i \
    "${BUILD_ENV[@]}" \
    "${SOURCE_DIR}"/configure "$@"
  popd
}

RunMake() {
  pushd "${BUILD_DIR}"/libelf
  env -i \
    "${BUILD_ENV[@]}" \
    make -j8 "${BUILD_ENV[@]}" "$@"
  popd
}

BuildLibElf() {
  Clean
  UntarAndPatch
  RunConfigure \
      --prefix="${INSTALL_DIR}" \
      --host=pnacl-unknown-nacl \
      --disable-werror
  RunMake libelf.a
  RunMake install-includeHEADERS
  RunMake install-libLIBRARIES
  # Install elf.h also
  cp -f "${SOURCE_DIR}"/libelf/elf.h "${INSTALL_DIR}"/include

  # elf.h needs features.h
  if ${EXTRA_HEADERS} ; then
    cp "${ROOT_DIR}"/include/features.h "${INSTALL_DIR}"/include
  fi
}

BuildLibElf