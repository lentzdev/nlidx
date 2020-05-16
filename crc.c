/*============================================================ Rev. 17 Jan 1993
  Routines for table driven CRC-16 & CRC-32, including building tables
  Refer to CRC.DOC for information and documentation.
  This file in portable C is 100% interchangable with the CRC.ASM (80x86 ASM).
  -----------------------------------------------------------------------------

               Information collected and edited by Arjen G. Lentz
             Sourcecode in C and 80x86 ASM written by Arjen G. Lentz
                  COPYRIGHT (C) 1992-1993; ALL RIGHTS RESERVED


  CONTACT ADDRESS

  LENTZ SOFTWARE-DEVELOPMENT    Arjen Lentz @
  Langegracht 7B                AINEX-BBS +31-33-633916
  3811 BT  Amersfoort           FidoNet 2:283/512
  The Netherlands               arjen_lentz@f512.n283.z2.fidonet.org


  DISCLAIMER

  This information is provided "as is" and comes with no warranties of any
  kind, either expressed or implied. It's intended to be used by programmers
  and developers. In no event shall the author be liable to you or anyone
  else for any damages, including any lost profits, lost savings or other
  incidental or consequential damages arising out of the use or inability
  to use this information.


  LICENCE

  This package may be freely distributed provided the files remain together,
  in their original unmodified form.
  All files, executables and sourcecode remain the copyrighted property of
  Arjen G. Lentz and LENTZ SOFTWARE-DEVELOPMENT.
  Licence for any use granted, provided this notice & CRC.DOC are included.
  For executable applications, credit should be given in the appropriate
  place in the program and documentation.
  These notices must be retained in any copies of any part of this
  documentation and/or software.

  Any use of, or operation on (including copying/distributing) any of
  the above mentioned files implies full and unconditional acceptance of
  this license and disclaimer.

=============================================================================*/

#include "crc.h"



/* ------------------------------------------------------------------------- */
void crc16init (word FAR *crctab, word poly)
{
        register int  i, j;
        register word crc;

        for (i = 0; i <= 255; i++) {
            crc = i;
            for (j = 8; j > 0; j--) {
                if (crc & 1) crc = (crc >> 1) ^ poly;
                else         crc >>= 1;
            }
            crctab[i] = crc;
        }
}/*crc16init()*/


/* ------------------------------------------------------------------------- */
word crc16block (word FAR *crctab, word crc, byte FAR *buf, word len)
{
        while (len--)
              crc = crc16upd(crctab,crc,*buf++);

        return (crc);
}/*crc16block()*/



/* ------------------------------------------------------------------------- */
void crc16rinit (word FAR *crctab, word poly)
{
        register int  i, j;
        register word crc;

        for (i = 0; i <= 255; i++) {
            crc = i << 8;
            for (j = 8; j > 0; j--) {
                if (crc & 0x8000) crc = (crc << 1) ^ poly;
                else              crc <<= 1;
            }
            crctab[i] = crc;
        }
}/*crc16rinit()*/


/* ------------------------------------------------------------------------- */
word crc16rblock (word FAR *crctab, word crc, byte FAR *buf, word len)
{
        while (len--)
              crc = crc16rupd(crctab,crc,*buf++);

        return (crc);
}/*crc16rblock()*/



/* ------------------------------------------------------------------------- */
void crc32init (dword FAR *crctab, dword poly)
{
        register int   i, j;
        register dword crc;

        for (i = 0; i <= 255; i++) {
            crc = i;
            for (j = 8; j > 0; j--) {
                if (crc & 1) crc = (crc >> 1) ^ poly;
                else         crc >>= 1;
            }
            crctab[i] = crc;
        }
}/*crc32init()*/


/* ------------------------------------------------------------------------- */
dword crc32block (dword FAR *crctab, dword crc, byte FAR *buf, word len)
{
        while (len--)
              crc = crc32upd(crctab,crc,*buf++);

        return (crc);
}/*crc32block()*/



/* end of crc.c ------------------------------------------------------------ */
