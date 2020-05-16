/* Sample of how to calculate the famous CRC of an St.Lous/FTS-0005 nodelist */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys\stat.h>
#include "crc.h"


word crctab[CRC_TABSIZE];


/* ------------------------------------------------------------------------- */
static boolean dos_sharing = false;


void dos_sharecheck (void)    /* SHARE installed? set dos_sharing true/false */
{
#if defined(__MSDOS__)
	union REGS regs;

#if defined(__386__)
	regs.w.ax = 0x1000;				/* DOS Multiplexer   */
	int386(0x2f,&regs,&regs);			/* INT 2Fh sub 1000h */
#else
	regs.x.ax = 0x1000;				/* DOS Multiplexer   */
	int86(0x2f,&regs,&regs);			/* INT 2Fh sub 1000h */
#endif
	dos_sharing = (regs.h.al == 0xff) ? true : false;
#endif
#if defined(__OS2__)
	dos_sharing = true;
#endif
}/*dos_sharecheck()*/


/* ----------------------------------------------------------------------------
			oflags			mode
	r	O_RDONLY			don't care
	w	O_CREAT | O_WRONLY | O_TRUNC	S_IWRITE
	a	O_CREAT | O_WRONLY | O_APPEND	S_IWRITE
	r+	O_RDWR				don't care
	w+	O_RDWR | O_CREAT | O_TRUNC	S_IWRITE | S_IREAD
	a+	O_RDWR | O_CREAT | O_APPEND	S_IWRITE | S_IREAD
---------------------------------------------------------------------------- */
FILE *sfopen (char *name, char *mode, int shareflag)
{
#if defined(__MSDOS__) || defined(__OS2__)
	int   fd, access, flags;
	char  *type = mode, c;
	FILE *fp;

	if ((c = *type++) == 'r') {
	   access = O_RDONLY | O_NOINHERIT;
	   flags = 0;
	}
	else if (c == 'w') {
	   access = O_WRONLY | O_CREAT | O_TRUNC | O_NOINHERIT;
	   flags = S_IWRITE;
	}
	else if (c == 'a') {
	   access = O_RDWR   | O_CREAT | O_APPEND | O_NOINHERIT;
	   flags = S_IWRITE;
	}
	else
	   return (NULL);

	c = *type++;

	if (c == '+' || (*type == '+' && (c == 't' || c == 'b'))) {
	   if (c == '+') c = *type;
	   access = (access & ~(O_RDONLY | O_WRONLY)) | O_RDWR;
	   flags = S_IREAD | S_IWRITE;
	}

	if	(c == 't') access |= O_TEXT;
	else if (c == 'b') access |= O_BINARY;
	else		   access |= (_fmode & (O_TEXT | O_BINARY));

	fd = sopen(name, access, dos_sharing ? shareflag : 0, flags);

	if (fd < 0) return (NULL);

	if ((fp = fdopen(fd,mode)) == NULL)
	   close(fd);
	else if (setvbuf(fp,NULL,_IOFBF,BUFSIZ)) {
	   fclose(fp);
	   return (NULL);
	}

	return (fp);
#endif

#if defined(__TOS__)
	char *p, *q, nm[10];
	FILE fp;

	strcpy(nm,mode);
	for(p=q=nm; *p; p++) if (*p!='t') *q++=*p;
	*q='\0';

	fp = fopen(name,nm);

	return (fp);
#endif
}/*sfopen()*/


/* ------------------------------------------------------------------------- */
void main (int argc, char *argv[])
{
	char  buffer[256];
	char *p;
	FILE *fp;
	word  crc;

	dos_sharecheck();

	if (argc != 2 || !(fp = sfopen(argv[1],"rt",SH_DENYNO)))
	   exit (1);
	setvbuf(fp,NULL,_IOFBF,16384);

	fgets(buffer,255,fp);
	p = strtok(buffer,"\r\n\032");
	printf("%s\n",p);

	crc16rinit(crctab,CRC16RPOLY);
	crc = CRC16RINIT;

	while (fgets(buffer,255,fp)) {
	      p = strtok(buffer,"\r\n\032");
	      crc = crc16rblock(crctab,crc,(byte *) p,strlen(buffer));
	      crc = crc16rblock(crctab,crc,(byte *) "\r\n",2);
	}
	fclose(fp);

	crc = CRC16RPOST(crc);
	printf("CRC=%05u\n",crc);
}


/* end of nodecrc.c */
