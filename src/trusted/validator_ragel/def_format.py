# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

################################################################################
# File format:
#     three columns separated by commas.  Each line describes one instruction.
#     Notation for argument types and sizes and for opcodes is based on
#     AMD64 Architecture Programmer's Manual.
################################################################################
# First column: instruction description.
#   Includes name of the instruction and arguments.
#
#   Arguments consist of four parts:
#      1. Read/write attribute (optional).
#      2. Argument type.
#      3. Argument size.
#      4. Implicit argument mark (optional).
#
#      Read/write attribute:
#       ': Instruction does not use this argument (lea or nop).
#       =: Instruction reads from this argument.
#       !: Instruction writes to this argument.
#       &: Instruction reads this argument and writes the result to it.
#        By default one- and two-operand instructions are assumed to read all
#        operands and store result to the last one, while instructions with
#        three or more operands are assumed to read all operands except last one
#        which is used to store the result of the execution.
#      Possible argument types:
#       a: Accumulator: %al/%ax/%eax/%rax/%es (depending on size).
#       c: Counter register: %cl/%cx/%ecx/%rcx/%cs (depending on size).
#       d: Data register: %dl/%dx/%edx/%rdx/%ss (depending on size).
#       b: Base register: %bl/%bx/%ebx/%rbx/%ds (depending on size).
#       f: Stack pointer: %spl/%sp/%esp/%rsp/%fs (depending on size).
#       g: Base pointer: %bpl/%bp/%ebp/%rbp/%gs (depending on size).
#       i: Second immediate value encoded in the instruction.
#       o: I/O port in %dx (used in "in"/"out" instructions).
#       p: Accumulator pair (%dx:%ax/%edx:%eax/%rdx:%rax depending on size).
#       r: Register in opcode (low 3 bits plus rex.B).
#       t: Top of the x87 stack (%st).
#       x: A memory operand addressed by the %ds:(%[er]bx). See "xlat".
#       B: General purpose register specified by the VEX/XOP.vvvv field.
#       C: Control register specified by the ModRM.reg field.
#       D: Debug register specified by the ModRM.reg field.
#       E: General purpose register or memory operand specified by the r/m
#          field of the ModRM byte.  For memory operands, the ModRM byte may
#          be followed by a SIB byte to specify one of the indexed
#          register-indirect addressing forms.
#       G: General purpose register specified by the reg field of ModRM.
#       H: YMM or XMM register specified by the VEX/XOP.vvvv field.
#       I: Immediate value encoded in the instruction.
#       J: The instruction encoding includes a relative offset that is added to
#          the rIP.
#       L: YMM or XMM register specified using the most-significant 4 bits of an
#          8-bit immediate value.  In legacy or compatibility mode the most
#          significant bit is ignored.
#       M: A memory operand specified by the {mod, r/m} field of the ModRM byte.
#          ModRM.mod != 11b.
#       N: 64-bit MMX register specified by the ModRM.r/m field. The ModRM.mod
#          field must be 11b.
#       O: The offset of an operand is encoded in the instruction. There is no
#          ModRM byte in the instruction encoding. Indexed register-indirect
#          addressing using the SIB byte is not supported.
#       P: 64-bit MMX register specified by the ModRM.reg field.
#       Q: 64-bit MMX-register or memory operand specified by the {mod, r/m}
#          field of the ModRM byte.  For memory operands, the ModRM byte may
#          be followed by a SIB byte to specify one of the indexed
#          register-indirect addressing forms.
#       R: General purpose register specified by the ModRM.r/m field.
#          The ModRM.mod field must be 11b.
#       S: Segment register specified by the ModRM.reg field.
#       U: YMM/XMM register specified by the ModRM.r/m field.
#          The ModRM.mod field must be 11b.
#       V: YMM/XMM register specified by the ModRM.reg field.
#       W: YMM/XMM register or memory operand specified by the {mod, r/m} field
#          of the ModRM byte.   For memory operands, the ModRM byte may be
#          followed by a SIB byte to specify one of the indexed
#          register-indirect addressing forms.
#       X: A memory operand addressed by the %ds:%[er]si registers. Used in
#          string instructions.
#       Y: A memory operand addressed by the %es:%[er]di registers. Used in
#          string instructions.
#      Possible sizes:
#       (no size provided):
#             A byte, word, doubleword, or quadword (in 64-bit mode),
#             depending on the effective operand size.
#       2:    Two bits (see VPERMIL2Px instruction).
#       7:    x87 register %st(N).
#       b:    A byte, irrespective of the effective operand size.
#       d:    A doubleword (32-bit), irrespective of the effective operand size.
#       do:   A double octword (256 bits), irrespective of the effective operand
#             size.
#       dq:   A double quadword (128 bits), irrespective of the effective
#             operand size.
#       fq:   A quadra quadword (256 bits), irrespective of the effective
#             operand size.
#       o:    An octword (128 bits), irrespective of the effective operand size.
#       p:    A 32-bit or 48-bit far pointer, depending on the effective operand
#             size.
#       pb:   A Vector with byte-wide (8-bit) elements (packed byte).
#       pd:   A double-precision (64-bit) floating-point vector operand (packed
#             double-precision).
#       pdw:  Vector composed of 32-bit doublewords.
#       pdwx: Vector composed of 32-bit doublewords. L bit selects 256bit YMM
#             registers.
#       pdx:  A double-precision (64-bit) floating-point vector operand (packed
#             double-precision).  L bit selects 256bit YMM registers.
#       ph:   A half-precision (16-bit) floating-point vector operand (packed
#             half-precision).
#       phx:  A half-precision (16-bit) floating-point vector operand (packed
#             half-precision).  L bit selects 256bit YMM registers.
#       pi:   Vector composed of 16-bit integers (packed integer).
#       pj:   Vector composed of 32-bit integers (packed double integer).
#       pjx:  Vector composed of 32-bit integers (packed double integer).
#             L bit selects 256bit YMM registers.
#       pk:   Vector composed of 8-bit integers (packed half-word integer).
#       pkx:  Vector composed of 8-bit integers (packed half-word integer).
#             L bit selects 256bit YMM registers.
#       pq:   Vector composed of 64-bit integers (packed quadword integer).
#       pqw:  Vector composed of 64-bit quadwords (packed quadword).
#       pqwx: Vector composed of 64-bit quadwords (packed quadword).  L bit
#             selects 256bit YMM registers.
#       pqx:  Vector composed of 64-bit integers (packed quadword integer).
#             L bit selects 256bit YMM registers.
#       ps:   A single-precision floating-point vector operand (packed
#             single-precision).
#       psx:  A single-precision floating-point vector operand (packed
#             single-precision).  L bit selects 256bit YMM registers.
#       pw:   Vector composed of 16-bit words (packed word).
#       q:    A quadword (64-bit), irrespective of the effective operand size.
#       r:    Register size (32bit in 32bit mode, 64bit in 64bit mode).
#       s:    Segment register (if register operand).
#       s:    A 6-byte or 10-byte pseudo-descriptor (if memory operand).
#       sb:   A scalar 10-byte packed BCD value (scalar BCD).
#       sd:   A scalar double-precision floating-point operand (scalar double).
#       se:   A 14-byte or 28-byte x87 environment.
#       si:   A scalar doubleword (32-bit) integer operand (scalar integer).
#       sq:   A scalar quadword (64-bit) integer operand (scalar integer).
#       sr:   A 94-byte or 108-byte x87 state.
#       ss:   A scalar single-precision floating-point operand (scalar single).
#       st:   A scalar 80bit-precision floating-point operand (scalar tenbytes).
#       sw:   A scalar word (16-bit) integer operand (scalar integer).
#       sx:   A 512-byte extended x87/MMX/XMM state.
#       v:    A word, doubleword, or quadword (in 64-bit mode), depending on
#             the effective operand size.
#       w:    A word, irrespective of the effective operand size.
#       x:    Instruction supports both vector sizes (128 bits or 256 bits).
#             Size is encoded using the VEX/XOP.L field. (L=0: 128 bits;
#             L=1: 256 bits). Usually this symbol is appended to ps or pd, but
#             sometimes it is used alone. For gen_dfa psx, pdx and x
#             are the same.
#       y:    A doubleword or quadword depending on effective operand size.
#       z:    A word if the effective operand size is 16 bits, or a doubleword
#             if the effective operand size is 32 or 64 bits.
#      Implicit argument mark:
#       *: This argument is implicit. It's not shown in the diassembly listing.
################################################################################
# Second column: instruction opcodes.
#   Includes all opcode bytes.  If first opcode bytes is 0x66/data16,
#   0xf2/repnz, or 0xf3/rep/repz then they can be moved before other prefixes
#   (and will be moved before REX prefix if it's allowed).  Note: data16, repnz,
#   and rep/repz opcodes will set appropriate flags while 0x66, 0xf2, and 0xf3
#   will not.
#   If part of the opcode is stored in ModRM byte then opcode should include the
#   usual "/0", "/1", ..., "/7" "bytes".  Use /s "byte" for segment register
#   operand: it'll make only registers 0..5 valid (there are only six segment
#   registers in total).  Use "/m" or "/r" "byte" to distinguish instructions
#   which have different names for memory and register operands (such as
#   "movlps" or "movhlps").
################################################################################
# Third column: additional instruction notes.
#   Different kind of notes for the instruction: non-typical prefixes (for
#   example "lock" prefix or "rep" prefix), CPUID checks, etc.
#
#     Possible prefixes:
#       branch_hint: branch hint prefixes are allowed (0x2E, 0x3E)
#       condrep: prefixes "repnz" and "repz" are allowed for the instruction
#       lock: prefix "lock" is allowed for the instruction
#       rep: prefix "rep" is allowed for the instruction (it's alias of "repz")
#       no_memory_access: command does not access memory in detectable way: lea,
#         nop, prefetch* instructions...
#       norex: "rex" prefix can not be used with this instruction (various "nop"
#         instructions use this flag)
#       norexw: "rex.W" can not be used with this instruction (usually used when
#         instruction with "rex.W" have a different name: e.g. "movd"/"movq")
#
#     Instruction enabling/disabling:
#       ia32: ia32-only instruction
#       amd64: amd64-only instruction
#       nacl-forbidden: instruction is not supported in NaCl sandbox
#       nacl-ia32-forbidden: instruction is not supported in ia32 NaCl sandbox
#       nacl-amd64-forbidden: instruction is not supported in amd64 NaCl sandbox
#
#     Special marks:
#       nacl-amd64-zero-extends: instruction can be used to zero-extend register
#         in amd64 mode
#       nacl-amd64-modifiable: instruction can be modified in amd64 mode
#       att-show-name-suffix-{b,l,ll,t,s,q,x,y,w}: instruction is shown with the
#         given suffix by objdump in AT&T mode
#       att-show-memory-suffix-{b,l,ll,t,s,q,x,y,w}: instruction is shown in the
#         given suffix by objdump in AT&T mode, but only if it accesses memory
#       spurious-rex.w: instruction ignores rex.w bit (most instructions don't
#         ignore it, that's why it's the default)
#
#     Possible CPUID values:
#       CPUFeature_3DNOW:     Fn8000_0001_EDX[3DNOW]
#       CPUFeature_3DPRFTCH:  Fn8000_0001_ECX[3DNowPrefetch] ||
#                             Fn8000_0001_EDX[LM] ||
#                             Fn8000_0001_EDX[3DNow]
#       CPUFeature_AES:       Fn0000_0001_ECX[AES]
#       CPUFeature_AESAVX:    Fn0000_0001_ECX[AES] && Fn0000_0001_ECX[AVX]
#       CPUFeature_ALTMOVCR8: Fn8000_0001_ECX[ALTMOVCR8] || Fn8000_0001_EDX[LM]
#       CPUFeature_AVX:       Fn0000_0001_ECX[AVX]
#       CPUFeature_BMI1:      Fn0000_0007_EBX_x0[BMI1]
#       CPUFeature_CLFLUSH:   Fn0000_0001_EDX[CLFSH]
#       CPUFeature_CLMUL:     Fn0000_0001_ECX[PCLMULQDQ]
#       CPUFeature_CLMULAVX:  Fn0000_0001_ECX[PCLMULQDQ] &&
#                             Fn0000_0001_ECX[AVX]
#       CPUFeature_CMOV:      Fn0000_0001_EDX[CMOV] || Fn8000_0001_EDX[CMOV]
#       CPUFeature_CMOVx87:  (Fn0000_0001_EDX[CMOV] || Fn8000_0001_EDX[CMOV]) &&
#                             Fn0000_0001_EDX[x87]
#       CPUFeature_CX16:      Fn0000_0001_ECX[CMPXCHG16B]
#       CPUFeature_CX8:       Fn0000_0001_EDX[CMPXCHG8B] ||
#                             Fn8000_0001_EDX[CMPXCHG8B]
#       CPUFeature_E3DNOW:    Fn8000_0001_EDX[E3DNOW]
#       CPUFeature_EMMX:      Fn8000_0001_EDX[EMMX]
#       CPUFeature_EMMXSSE:   Fn8000_0001_EDX[EMMX] ||
#                             Fn0000_0001_EDX[SSE]
#       CPUFeature_F16C:      Fn0000_0001_ECX[F16C]
#       CPUFeature_FMA:       Fn0000_0001_ECX[FMA]
#       CPUFeature_FMA4:      Fn8000_0001_ECX[FMA4]
#       CPUFeature_FXSR:      Fn0000_0001_ECX[XSAVE]
#       CPUFeature_LAHF:      Fn8000_0001_ECX[LahfSahf]
#       CPUFeature_LWP:       Fn8000_0001_ECX[LWP]
#       CPUFeature_LZCNT:     Fn8000_0001_EXC[ABM]
#       CPUFeature_MMX:       Fn0000_0001_EDX[MMX]
#       CPUFeature_MON:       Fn0000_0001_ECX[Monitor]
#       CPUFeature_MOVBE:     Fn0000_0001_ECX[MOVBE]
#       CPUFeature_MSR:       Fn0000_0001_EDX[MSR]
#       CPUFeature_POPCNT:    Fn0000_0001_ECX[POPCNT]
#       CPUFeature_SEP:       Fn0000_0001_EDX[SYSENTER]
#       CPUFeature_SSE:       Fn0000_0001_EDX[SSE]
#       CPUFeature_SSE2:      Fn0000_0001_EDX[SSE2]
#       CPUFeature_SSE3:      Fn0000_00001_ECX[SSE3]
#       CPUFeature_SSE41:     Fn0000_0001_ECX[SSE41]
#       CPUFeature_SSE42:     Fn0000_0001_ECX[SSE42]
#       CPUFeature_SSE4A:     Fn8000_0001_ECX[SSE4A]
#       CPUFeature_SSSE3:     Fn0000_00001_ECX[SSSE3]
#       CPUFeature_SKINIT:   (Fn8000_0001_ECX[SVM] && EFER.SVME) ||
#                             Fn8000_0001_ECX[SKINIT]
#       CPUFeature_SVM:       Fn8000_0001_ECX[SVM] && EFER.SVME
#       CPUFeature_SYSCALL:   Fn8000_0001_EDX[SYSCALL]
#       CPUFeature_TBM:       Fn8000_0001_ECX[TBM]
#       CPUFeature_TSC:       Fn0000_0001_EDX[RDTSC] || Fn8000_0001_EDX[RDTSC]
#       CPUFeature_TSCP:      Fn8000_0001_EDX[RDTSCP]
#       CPUFeature_TZCNT:     Fn0000_0007_EBX_x0[BMI1]
#       CPUFeature_x87:       Fn0000_0001_EDX[x87]
#       CPUFeature_XOP:       Fn8000_0001_ECX[XOP]
################################################################################


# Technically, columns are separated with mere ',' followed by spaces for
# readability, but there are quoted instruction names that include commas
# not followed by spaces (see nops.def).
# For simplicity I choose to rely on this coincidence and use split-based parser
# instead of proper recursive descent one.
# If by accident somebody put ', ' in quoted instruction name, it will fail
# loudly, because closing quote then will fall into second or third column and
# will cause parse error.
# TODO(shcherbina): use for column separator something that is never encountered
# in columns, like semicolon?
COLUMN_SEPARATOR = ', '


SUPPORTED_ATTRIBUTES = [
    # Parsing attributes.
    'branch_hint',
    'condrep',
    'lock',
    'no_memory_access',
    'norex',
    'norexw',
    'rep',

    # CPUID attributes.
    'CPUFeature_3DNOW',
    'CPUFeature_3DPRFTCH',
    'CPUFeature_AES',
    'CPUFeature_AESAVX',
    'CPUFeature_ALTMOVCR8',
    'CPUFeature_AVX',
    'CPUFeature_BMI1',
    'CPUFeature_CLFLUSH',
    'CPUFeature_CLMUL',
    'CPUFeature_CLMULAVX',
    'CPUFeature_CMOV',
    'CPUFeature_CMOVx87',
    'CPUFeature_CX16',
    'CPUFeature_CX8',
    'CPUFeature_E3DNOW',
    'CPUFeature_EMMX',
    'CPUFeature_EMMXSSE',
    'CPUFeature_F16C',
    'CPUFeature_FMA',
    'CPUFeature_FMA4',
    'CPUFeature_FXSR',
    'CPUFeature_LAHF',
    'CPUFeature_LWP',
    'CPUFeature_LZCNT',
    'CPUFeature_MMX',
    'CPUFeature_MON',
    'CPUFeature_MOVBE',
    'CPUFeature_MSR',
    'CPUFeature_POPCNT',
    'CPUFeature_SEP',
    'CPUFeature_SFENCE',
    'CPUFeature_SKINIT',
    'CPUFeature_SSE',
    'CPUFeature_SSE2',
    'CPUFeature_SSE3',
    'CPUFeature_SSE41',
    'CPUFeature_SSE42',
    'CPUFeature_SSE4A',
    'CPUFeature_SSSE3',
    'CPUFeature_SVM',
    'CPUFeature_SYSCALL',
    'CPUFeature_TBM',
    'CPUFeature_TSC',
    'CPUFeature_TSCP',
    'CPUFeature_TZCNT',
    'CPUFeature_x87',
    'CPUFeature_XOP',

    # Attributes for enabling/disabling based on architecture and validity.
    'ia32',
    'amd64',
    'nacl-ia32-forbidden',
    'nacl-amd64-forbidden',
    'nacl-forbidden',
    'nacl-amd64-zero-extends',
    'nacl-amd64-modifiable',

    # AT&T Decoder attributes.
    'att-show-memory-suffix-b',
    'att-show-memory-suffix-l',
    'att-show-memory-suffix-ll',
    'att-show-memory-suffix-t',
    'att-show-memory-suffix-s',
    'att-show-memory-suffix-q',
    'att-show-memory-suffix-x',
    'att-show-memory-suffix-y',
    'att-show-memory-suffix-w',
    'att-show-name-suffix-b',
    'att-show-name-suffix-l',
    'att-show-name-suffix-ll',
    'att-show-name-suffix-t',
    'att-show-name-suffix-s',
    'att-show-name-suffix-q',
    'att-show-name-suffix-x',
    'att-show-name-suffix-y',
    'att-show-name-suffix-w',

    # Spurious REX.W bits (instructions 'in', 'out', 'nop', etc).
    'spurious-rex.w'
]


class OperandReadWriteMode(object):
  UNUSED = '\''
  READ = '='
  WRITE = '!'
  READ_WRITE = '&'
