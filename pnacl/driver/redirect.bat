@echo off
:: Copyright (c) 2011 The Native Client Authors. All rights reserved.
:: Use of this source code is governed by a BSD-style license that can be
:: found in the LICENSE file.

:: This file is installed for each driver file so that the driver can
:: be run from outside Cygwin. This file will be installed as bin/pnacl-*.bat
:: inside the PNaCl directory. For example, if you invoke:
::
::  C:\> path\to\pnacl\bin\pnacl-gcc hello.c -o hello.pexe
::
:: inside the Windows command prompt, then Windows runs pnacl-gcc.bat.
:: This script will in turn run:
:: "python path\to\pnacl\bin\pnacl-gcc hello.c -o hello.pexe"

setlocal

:: Stop incessant CYGWIN complains about "MS-DOS style path"
set CYGWIN=nodosfilewarning %CYGWIN%

:: TODO(pdox): For the toolchain to be hermetic and relocatable,
:: we may need to arrange to include Python in our distribution.
:: For now, use the Python included in the repository checkout.
set PATH=%~dp0..\..\..\..\third_party\python_26;%PATH%

:: Run the driver
:: For now, assume "python" is in the PATH.
python -OO "%~dp0\%~n0" %*

:end