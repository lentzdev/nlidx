/* ========================================================================= */
/* Some basic definitions						     */
/* COPYRIGHT by A.G. Lentz & LENTZ SOFTWARE-DEVELOPMENT; ALL RIGHTS RESERVED */
/* ------------------------------------------------------------------------- */
#ifndef __2TYPES_DEF_
#define __2TYPES_DEF_


typedef unsigned char  byte;
typedef unsigned short word;					/*AGL:10mar93*/
typedef unsigned long  dword;

enum boolean { false, true };
#ifndef __cplusplus
typedef int boolean;
#endif

typedef long FILE_OFS;				/* Offset in a disk file     */
#ifdef __cplusplus
const FILE_OFS OFS_NONE = -1L;			/* Unused file offset ptr    */
#else
#define OFS_NONE (-1L)
#endif


#endif/*__2TYPES_DEF_*/


/* end of 2types.h */
