# src/MD5.c  src/byterun.c src/ebcdic.c src/md5c.c

SRC=\
src/av.c \
src/deb.c \
src/doio.c \
src/doop.c \
src/dump.c \
src/fake_dyna.c \
src/globals.c \
src/gv.c \
src/hv.c \
src/mg.c \
src/op.c \
src/perl.c \
src/perlio.c \
src/perly.c \
src/pp.c \
src/pp_ctl.c \
src/pp_hot.c \
src/pp_sys.c \
src/regcomp.c \
src/regexec.c \
src/run.c \
src/scope.c \
src/sv.c \
src/taint.c \
src/toke.c \
src/universal.c \
src/unix_perlmain.c \
src/util.c \
src/MD5.c \
src/md5c.c

IT=perlbmk
#CFLAGS_EXTRA = -DSPEC_CPU2000_LINUX -D_IOCTL_  -DHAS_DBL_DIG

CFLAGS_EXTRA = -DSPEC_CPU2000_LINUX -DHAS_DBL_DIG -DSPEC_CPU2000_NEED_TIME_H \
-DSPEC_CPU2000_NEED_BOOL

EXTRA_CLEAN= benums  cpu2000_mhonarc.rc lib dictionary

include ../Makefile.common
