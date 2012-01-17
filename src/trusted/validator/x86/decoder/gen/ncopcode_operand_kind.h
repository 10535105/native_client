/* native_client/src/trusted/validator/x86/decoder/gen/ncopcode_operand_kind.h
 * THIS FILE IS AUTO_GENERATED DO NOT EDIT.
 *
 * This file was auto-generated by enum_gen.py
 * from file ncopcode_operand_kind.enum
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_GEN_NCOPCODE_OPERAND_KIND_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_GEN_NCOPCODE_OPERAND_KIND_H__

#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN
typedef enum NaClOpKind {
  Unknown_Operand = 0,
  A_Operand = 1,
  Aw_Operand = 2,
  Av_Operand = 3,
  Ao_Operand = 4,
  E_Operand = 5,
  Eb_Operand = 6,
  Ew_Operand = 7,
  Ev_Operand = 8,
  Eo_Operand = 9,
  Edq_Operand = 10,
  G_Operand = 11,
  Gb_Operand = 12,
  Gw_Operand = 13,
  Gv_Operand = 14,
  Go_Operand = 15,
  Gdq_Operand = 16,
  Seg_G_Operand = 17,
  G_OpcodeBase = 18,
  I_Operand = 19,
  Ib_Operand = 20,
  Iw_Operand = 21,
  Iv_Operand = 22,
  Io_Operand = 23,
  I2_Operand = 24,
  J_Operand = 25,
  Jb_Operand = 26,
  Jw_Operand = 27,
  Jv_Operand = 28,
  M_Operand = 29,
  Mb_Operand = 30,
  Mw_Operand = 31,
  Mv_Operand = 32,
  Mo_Operand = 33,
  Mdq_Operand = 34,
  Mpw_Operand = 35,
  Mpv_Operand = 36,
  Mpo_Operand = 37,
  Mmx_E_Operand = 38,
  Mmx_N_Operand = 39,
  Mmx_G_Operand = 40,
  Mmx_Gd_Operand = 41,
  Xmm_E_Operand = 42,
  Xmm_Eo_Operand = 43,
  Xmm_G_Operand = 44,
  Xmm_Go_Operand = 45,
  C_Operand = 46,
  D_Operand = 47,
  O_Operand = 48,
  Ob_Operand = 49,
  Ow_Operand = 50,
  Ov_Operand = 51,
  Oo_Operand = 52,
  S_Operand = 53,
  St_Operand = 54,
  RegUnknown = 55,
  RegAL = 56,
  RegBL = 57,
  RegCL = 58,
  RegDL = 59,
  RegAH = 60,
  RegBH = 61,
  RegCH = 62,
  RegDH = 63,
  RegDIL = 64,
  RegSIL = 65,
  RegBPL = 66,
  RegSPL = 67,
  RegR8B = 68,
  RegR9B = 69,
  RegR10B = 70,
  RegR11B = 71,
  RegR12B = 72,
  RegR13B = 73,
  RegR14B = 74,
  RegR15B = 75,
  RegAX = 76,
  RegBX = 77,
  RegCX = 78,
  RegDX = 79,
  RegSI = 80,
  RegDI = 81,
  RegBP = 82,
  RegSP = 83,
  RegR8W = 84,
  RegR9W = 85,
  RegR10W = 86,
  RegR11W = 87,
  RegR12W = 88,
  RegR13W = 89,
  RegR14W = 90,
  RegR15W = 91,
  RegEAX = 92,
  RegEBX = 93,
  RegECX = 94,
  RegEDX = 95,
  RegESI = 96,
  RegEDI = 97,
  RegEBP = 98,
  RegESP = 99,
  RegR8D = 100,
  RegR9D = 101,
  RegR10D = 102,
  RegR11D = 103,
  RegR12D = 104,
  RegR13D = 105,
  RegR14D = 106,
  RegR15D = 107,
  RegCS = 108,
  RegDS = 109,
  RegSS = 110,
  RegES = 111,
  RegFS = 112,
  RegGS = 113,
  RegCR0 = 114,
  RegCR1 = 115,
  RegCR2 = 116,
  RegCR3 = 117,
  RegCR4 = 118,
  RegCR5 = 119,
  RegCR6 = 120,
  RegCR7 = 121,
  RegCR8 = 122,
  RegCR9 = 123,
  RegCR10 = 124,
  RegCR11 = 125,
  RegCR12 = 126,
  RegCR13 = 127,
  RegCR14 = 128,
  RegCR15 = 129,
  RegDR0 = 130,
  RegDR1 = 131,
  RegDR2 = 132,
  RegDR3 = 133,
  RegDR4 = 134,
  RegDR5 = 135,
  RegDR6 = 136,
  RegDR7 = 137,
  RegDR8 = 138,
  RegDR9 = 139,
  RegDR10 = 140,
  RegDR11 = 141,
  RegDR12 = 142,
  RegDR13 = 143,
  RegDR14 = 144,
  RegDR15 = 145,
  RegEFLAGS = 146,
  RegRFLAGS = 147,
  RegEIP = 148,
  RegRIP = 149,
  RegRAX = 150,
  RegRBX = 151,
  RegRCX = 152,
  RegRDX = 153,
  RegRSI = 154,
  RegRDI = 155,
  RegRBP = 156,
  RegRSP = 157,
  RegR8 = 158,
  RegR9 = 159,
  RegR10 = 160,
  RegR11 = 161,
  RegR12 = 162,
  RegR13 = 163,
  RegR14 = 164,
  RegR15 = 165,
  RegREIP = 166,
  RegREAX = 167,
  RegREBX = 168,
  RegRECX = 169,
  RegREDX = 170,
  RegRESP = 171,
  RegREBP = 172,
  RegRESI = 173,
  RegREDI = 174,
  RegREAXa = 175,
  RegDS_ESI = 176,
  RegDS_EDI = 177,
  RegES_EDI = 178,
  RegDS_EBX = 179,
  RegST0 = 180,
  RegST1 = 181,
  RegST2 = 182,
  RegST3 = 183,
  RegST4 = 184,
  RegST5 = 185,
  RegST6 = 186,
  RegST7 = 187,
  RegMMX0 = 188,
  RegMMX1 = 189,
  RegMMX2 = 190,
  RegMMX3 = 191,
  RegMMX4 = 192,
  RegMMX5 = 193,
  RegMMX6 = 194,
  RegMMX7 = 195,
  RegXMM0 = 196,
  RegXMM1 = 197,
  RegXMM2 = 198,
  RegXMM3 = 199,
  RegXMM4 = 200,
  RegXMM5 = 201,
  RegXMM6 = 202,
  RegXMM7 = 203,
  RegXMM8 = 204,
  RegXMM9 = 205,
  RegXMM10 = 206,
  RegXMM11 = 207,
  RegXMM12 = 208,
  RegXMM13 = 209,
  RegXMM14 = 210,
  RegXMM15 = 211,
  RegGP7 = 212,
  Const_1 = 213,
  NaClOpKindEnumSize = 214, /* special size marker */
} NaClOpKind;

/* Returns the name of an NaClOpKind constant. */
extern const char* NaClOpKindName(NaClOpKind name);

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_GEN_NCOPCODE_OPERAND_KIND_H__ */
