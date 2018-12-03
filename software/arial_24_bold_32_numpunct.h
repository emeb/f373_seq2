#ifndef arial_24_bold_32_numpunctH
#define arial_24_bold_32_numpunctH

#ifndef SMfgTypes
#define SMfgTypes

/*======= binar input =======*/
#define b2b(b7,b6,b5,b4,b3,b2,b1,b0) ((unsigned char)((b7)*128u + (b6)*64u + (b5)*32u + (b4)*16u + (b3)*8u + (b2)*4u + (b1)*2u + (b0)))

/*============================================================================*/
/* You have to manualy set correct types TCDATA and TCLISTP                   */
/* for your platform and compiler.                                            */
/*                                                                            */
/* Keil C51 example:                                                          */
/* Character data (TCDATA) are stored in code memory,                         */
/* array of pointers to characters (TCLISTP) is stored in code memory         */
/* and pointers are pointing into code memory.                                */
/*                                                                            */
/* typedef unsigned char code TCDATA;                                         */
/* typedef TCDATA * code TCLISTP;                                             */
/*============================================================================*/

typedef const unsigned char TCDATA;
typedef const TCDATA* TCLISTP;

#endif

/*======= Character pointers table =======*/
extern TCLISTP arial_24_bold_32_numpunct[256];

/*======= Characters data =======*/
TCDATA arial_24_bold_32_numpunct_ssp[17];
TCDATA arial_24_bold_32_numpunct_spls[69];
TCDATA arial_24_bold_32_numpunct_scmm[21];
TCDATA arial_24_bold_32_numpunct_smin[41];
TCDATA arial_24_bold_32_numpunct_sssp[21];
TCDATA arial_24_bold_32_numpunct_n0[65];
TCDATA arial_24_bold_32_numpunct_n1[53];
TCDATA arial_24_bold_32_numpunct_n2[65];
TCDATA arial_24_bold_32_numpunct_n3[65];
TCDATA arial_24_bold_32_numpunct_n4[69];
TCDATA arial_24_bold_32_numpunct_n5[69];
TCDATA arial_24_bold_32_numpunct_n6[65];
TCDATA arial_24_bold_32_numpunct_n7[65];
TCDATA arial_24_bold_32_numpunct_n8[65];
TCDATA arial_24_bold_32_numpunct_n9[65];
TCDATA arial_24_bold_32_numpunct_scln[21];
TCDATA arial_24_bold_32_numpunct_blank[33];

#endif
