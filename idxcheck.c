/* Little util to dump a complete nodeidx format index to stdout */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys\stat.h>
#include "nodeidx.h"


#define MAX_BUFFER 255

NLHEADER  nlheader;
char	 *nlname;
NLFILE	 *nlfile;
NLZONE	 *nlzone;
NLNET	 *nlnet;
NLNODE	  nlnode;
NLBOSS	 *nlboss;
NLPOINT   nlpoint;

FILE  *idxfp;
char   buffer[MAX_BUFFER + 1];
FILE  *nlfp;
byte   cur_file;
long   cur_ofs;


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
void *myalloc (word size)
{
	void *np;

	if (!(np = malloc(size))) {
	   printf("Error allocating memory\n");
	   exit (1);
	}
	memset(np,0,size);

	return (np);
}/*myalloc()*/


/* ------------------------------------------------------------------------- */
char *getline (long entry)
{
	char *p   = buffer;
	word  got = 0;
	int   c;

	if (!nlfp || NL_ENTFILE(entry) != cur_file) {
	   if (nlfp) fclose(nlfp);
	   cur_file = NL_ENTFILE(entry);
	   if (!(nlfp = sfopen(&nlname[nlfile[cur_file].name_ofs],"rb",SH_DENYNO))) {
	      printf("%08lx: Can't open raw file '%s'\n",
		     entry,&nlname[nlfile[cur_file].name_ofs]);
	      exit (1);
	   }
	   cur_ofs = 0L;
	}

	if (NL_ENTOFS(entry) != cur_ofs) {
	   cur_ofs = NL_ENTOFS(entry);
	   fseek(nlfp,cur_ofs,SEEK_SET);
	}

	while ((c = getc(nlfp)) != EOF && !ferror(nlfp) && got < MAX_BUFFER) {
	      if (c == '\032') break;
	      got++;
	      if (c == '\r' || c == '\n') {
		 c = getc(nlfp);
		 if (ferror(nlfp)) break;
		 if (c == '\r' || c == '\n') got++;
		 else			     ungetc(c,nlfp);
		 *p = '\0';
		 cur_ofs += got;
		 return (buffer);
	      }
	      *p++ = c;
	}

	if (ferror(nlfp)) {
	   printf("%s %ld: Error reading file\n",
		  &nlname[nlfile[cur_file].name_ofs], cur_ofs);
	   exit (1);
	}
	if (got == MAX_BUFFER) {
	   printf("%s %ld: Line to long\n",
		  &nlname[nlfile[cur_file].name_ofs], cur_ofs);
	   exit (1);
	}
	if (got) {
	   printf("%s %ld: Unexpected end of file\n",
		  &nlname[nlfile[cur_file].name_ofs], cur_ofs);
	   exit (1);
	}

	cur_ofs += got;
	return (buffer);
}


/* ------------------------------------------------------------------------- */
char *showflags (int flags)
{
	static char buf[20];
	char *p;

	sprintf(buf,"%-4s ", (flags & NL_HASPOINTS) ? "Boss" : "");

	switch (NL_TYPEMASK(flags)) {
	       case NL_ZC:   p = "Zone";   break;
	       case NL_RC:   p = "Region"; break;
	       case NL_NC:   p = "Host";   break;
	       case NL_HUB:  p = "Hub";    break;
	       case NL_DOWN: p = "Down";   break;
	       case NL_HOLD: p = "Hold";   break;
	       case NL_PVT:  p = "Pvt";    break;
	       case NL_NORM: p = "Node";   break;
	}
	sprintf(&buf[strlen(buf)],"%-6s ",p);

	return (buf);
}


/* ------------------------------------------------------------------------- */
void main (int argc, char *argv[])
{
	word zone, net, node, boss, point;

	if (argc != 2) exit (0);

	dos_sharecheck();

	if (!(idxfp = sfopen(argv[1],"rb",SH_DENYNO))) exit (1);
	setvbuf(idxfp,NULL,_IOFBF,16384);

	fread(&nlheader,sizeof (NLHEADER),1,idxfp);
	printf("IDstring %*.*s\n",
	       sizeof (nlheader.idstring), sizeof (nlheader.idstring),
	       nlheader.idstring);
	printf("Creator  %s\n",nlheader.creator);
	printf("Revision %u.%02u\n",
	       NL_REVMAJOR(nlheader.revision),
	       NL_REVMINOR(nlheader.revision));
	printf("Files    %5u\n",nlheader.files);
	printf("Zones    %5u\n",nlheader.zones);
	printf("Bosses   %5u\n",nlheader.bosses);
	printf("\n");

	nlname = (char *) myalloc(nlheader.name_size);
	fseek(idxfp,nlheader.name_ofs,SEEK_SET);
	fread(nlname,nlheader.name_size,1,idxfp);

	nlfile = (NLFILE *) myalloc(nlheader.files * sizeof (NLFILE));
	fseek(idxfp,nlheader.file_ofs,SEEK_SET);
	fread(nlfile,sizeof (NLFILE) * nlheader.files,1,idxfp);

	nlzone = (NLZONE *) myalloc(nlheader.zones * sizeof (NLZONE));
	fseek(idxfp,nlheader.zone_ofs,SEEK_SET);
	fread(nlzone,sizeof (NLZONE) * nlheader.zones,1,idxfp);

	if (nlheader.bosses) {
	   nlboss = (NLBOSS *) myalloc(nlheader.bosses * sizeof (NLBOSS));
	   fseek(idxfp,nlheader.boss_ofs,SEEK_SET);
	   fread(nlboss,sizeof (NLBOSS) * nlheader.bosses,1,idxfp);
	}

	nlfp = NULL;
	cur_file = 0;
	cur_ofs = 0L;

	for (zone = 0; zone < nlheader.zones; zone++) {
	    printf("%s %-5u (nets %5u) %s\n",
		   showflags(nlzone[zone].flags), nlzone[zone].zone,
		   nlzone[zone].nets,
		   getline(nlzone[zone].entry));

	    nlnet = (NLNET *) myalloc(nlzone[zone].nets * sizeof (NLNET));
	    fseek(idxfp,nlzone[zone].net_ofs,SEEK_SET);
	    fread(nlnet,sizeof (NLNET) * nlzone[zone].nets,1,idxfp);
	    for (net = 0; net < nlzone[zone].nets; net++) {
		printf("    %s %-5u (nodes %5u) %s\n",
		       showflags(nlnet[net].flags), nlnet[net].net,
		       nlnet[net].nodes,
		       getline(nlnet[net].entry));

#if 1
		fseek(idxfp,nlnet[net].node_ofs,SEEK_SET);
		for (node = 0; node < nlnet[net].nodes; node++) {
		    fread(&nlnode,sizeof (NLNODE),1,idxfp);
		    printf("        %s %-5u %s\n",
			   showflags(nlnode.flags), nlnode.node,
			   getline(nlnode.entry));
		}
#endif
	    }
	    free(nlnet);
	}

	printf("\n\n");

	for (boss = 0; boss < nlheader.bosses; boss++) {
	    printf("%s %u:%u/%u (points %5u) %s\n",
		   showflags(nlboss[boss].flags),
		   nlboss[boss].zone, nlboss[boss].net, nlboss[boss].node,
		   nlboss[boss].points,
		   getline(nlboss[boss].entry));

	    fseek(idxfp,nlboss[boss].point_ofs,SEEK_SET);
	    for (point = 0; point < nlboss[boss].points; point++) {
		fread(&nlpoint,sizeof (NLPOINT),1,idxfp);
		printf("    %s %5u %s\n",
		       showflags(nlpoint.flags), nlpoint.point,
		       getline(nlpoint.entry));
	    }
	}

	if (nlfp) fclose(nlfp);
	fclose(idxfp);
}


/* end of idxcheck.c */
