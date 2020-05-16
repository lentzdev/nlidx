/* NodeIDX - Nodelist index processor (nodeidx format) ==== Rev: 23 Oct 1995 */
/* Indexing system on raw ASCII nodelist (St.Louis/FTS-0005 format)	     */
/* Design & COPYRIGHT (C) 1993-1995 by Arjen G. Lentz; ALL RIGHTS RESERVED   */
/* A project of LENTZ SOFTWARE-DEVELOPMENT, Amersfoort, The Netherlands (EU) */
/* ------------------------------------------------------------------------- */
/* Permission is hereby granted to implement the nodeidx format in your own  */
/* applications (e.g. nodelist compilers, msg-editors, mailers, outbound     */
/* managers, BBS software, etc), on the condition that no alterations	     */
/* (be it changes or additions) made to this format. The format allows for   */
/* backward-compatible exptensions; of course this is only possible if no    */
/* conflicting extentions are introduced. For this purpose, any alteration   */
/* of this format requires *explicit* prior approval from me.		     */
/* Arjen Lentz @ FidoNet 2:283/512, 2:283/550, 2:283/551, agl@bitbike.iaf.nl */
/* ========================================================================= */
/* This source compiles cleanly with BC 3.1, BC/2 1.0, WatcomC 10.0 DOS,OS/2 */
/* ========================================================================= */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <ctype.h>
#include <time.h>
#include <sys\stat.h>
#include "nodeidx.h"


#define USERLIST 1


#define PRGNAME "NodeIDX"
#define VERSION "1.07"

#if defined(__OS2__)
#define NIX_OS "OS/2"
#elif defined(__MSDOS__)
#define NIX_OS "DOS"
#elif defined(_Windows)
#define NIX_OS "Windows"
#elif defined(__TOS__)
#define NIX_OS "ST"
#endif

#define DENY_ALL   0x0000
#define DENY_RDWR  0x0010
#define DENY_WRITE 0x0020
#define DENY_READ  0x0030
#define DENY_NONE  0x0040

#define MAX_BUFFER 255

enum { NL_BOSS = NL_ZC + 1, NL_POINT };

typedef struct _nodelist NODELIST;
typedef struct _merge	 MERGE;

typedef struct _idxfile  IDXFILE;
typedef struct _idxzone  IDXZONE;
typedef struct _idxnet	 IDXNET;
typedef struct _idxboss  IDXBOSS;
typedef struct _idxpoint IDXPOINT;

struct _nodelist {
	char	 *idxname;
	char	 *idxpath;
	long	  old_timestamp;
	byte	  number;
	char	 *basename;
	long	  timestamp;
	word	  file_flags;
	word	  zone;
	MERGE	 *merge;
	NODELIST *next;
};

struct _merge {
	word	 zone, section;
	char	*filename;
	long	 timestamp;
	word	 file_flags;
	boolean  used;
	MERGE	*next;
};


struct _idxfile {
	byte	 file;
	char	*filename;
	NLFILE	 nlfile;
	IDXFILE *next;
};

struct _idxzone {
	NLZONE	 nlzone;
	IDXNET	*netptr;
	IDXZONE *next;
};

struct _idxnet {
	NLNET	  nlnet;
	IDXNET	 *next;
};

struct _idxboss {
	NLBOSS	  nlboss;
	IDXPOINT *pointptr;
	IDXBOSS  *next;
};

struct _idxpoint {
	NLPOINT   nlpoint;
	IDXPOINT *next;
};


/* ------------------------------------------------------------------------- */
NODELIST *nodelist, *nlp;	/* Linked list of nodelists, ptr to current  */
NLHEADER  nlheader;		/* Index header record of current nodelist   */
NLNODE	  nlnode;		/* Current node record			     */
IDXFILE  *idxfile,  *ifp;	/* Index raw files list, ptr to current      */
IDXZONE  *idxzone,  *izonep;	/* Index zone list, ptr to current	     */
IDXNET		    *inetp;	/* Ptr to current net			     */
IDXBOSS  *idxboss,  *ibp;	/* Index boss list, ptr to current	     */
IDXPOINT	    *ipp;	/* Ptr to current point 		     */
char	  buffer[MAX_BUFFER + 1];		/* Raw list read line buffer */
word	  cur_zone, cur_region, cur_net;	/* Current zone/region/net   */
boolean   skip_zone, skip_region, skip_net;	/* Skipping cur zone/reg/net */
char	 *cur_name;				/* Current raw file name     */
byte	  cur_file;				/* Current raw file idx #    */
FILE_OFS  cur_ofs;				/* Current raw file offset   */
dword	  cur_line;				/* Current raw nodelist line */
FILE	 *idxfp;				/* File ptr of index	     */
FILE_OFS  idxofs;				/* Offset into index file    */
FILE	 *rptfp;				/* File ptr of report file   */
FILE	 *gatefp;				/* File ptr of gates file    */
#if USERLIST
FILE	 *userfp;				/* File ptr of user/adr file */
#endif
word	  yearday;				/* Current day of year 1-366 */
boolean   delold = false;			/* Delete old periodic lists */
#if USERLIST
boolean   userlist = false;			/* Create user/address lists */
#endif
boolean   do_break = false;			/* Ctrl-C/Break flag - abort */

struct _total {
	  dword node,
		down,
		hold,
		zone,
		region,
		host,
		hub,
		pvt,
		boss,
		point,
		gate;
#if USERLIST
	  dword user;
#endif
	  dword open,
		separate,
		accessible;
} total;

typedef struct _oldidx OLDIDX;
struct _oldidx {
	NODELIST *nlp;
	byte	  number;
	OLDIDX	 *next;
};

typedef struct _oldfile OLDFILE;
struct _oldfile {
	char	*filename;
	boolean  check;
	boolean  keep;
	OLDFILE *next;
};

/* ------------------------------------------------------------------------- */
char	*ffirst 		 (char *filespec);
char	*fnext			 (void);
void	 fnclose		 (void);
int	 getstat		 (char *pathname, struct stat *statbuf);
boolean  fexist 		 (char *pathname);
FILE	*sfopen 		 (char *name, char *mode, int shareflag);
int	 nlgets 		 (FILE *fp);
boolean  idxwrite		 (void *ptr, word len);
void	*myalloc		 (word size);
char	*mystrdup		 (char *s);
boolean  findlist		 (char *filespec, char **foundname, word *file_flags);
boolean  read_ctl		 (void);
boolean  idxchange		 (NODELIST *nlp);
void	 getfile		 (char *name, long timestamp, word file_flags);
void	 getboss		 (word zone, word net, word node, boolean bossentry);
boolean  addpoint		 (word point, int flags);
boolean  putnode		 (word node,  int flags);
boolean  addnet 		 (word net,   int flags);
boolean  addzone		 (word zone,  int flags);
boolean  write_net		 (void);
boolean  write_pointbossfilezone (void);
void	 deloldidx		 (void);
boolean  process		 (char *name, long timestamp, word file_flags);
void	 main			 (int argc, char *argv[]);


/* ------------------------------------------------------------------------- */
#if defined(__MSDOS__) || defined(__OS2__)


static struct find_t ffblk;


char *ffirst (char *name)
{
    return (_dos_findfirst(name,_A_NORMAL,&ffblk) ? NULL : ffblk.name);
}/*ffirst()*/


char *fnext (void)
{
    return (_dos_findnext(&ffblk) ? NULL : ffblk.name);
}/*fnext()*/


void fnclose (void)
{
#if defined(__WATCOMC__)
    _dos_findclose(&ffblk);
#else
    while (fnext());
#endif
}/*fnclose()*/


/* ------------------------------------------------------------------------- */
int getstat (char *pathname, struct stat *statbuf)
{
    struct find_t ffdta;
    struct tm	  t;
    unsigned res;

    res = _dos_findfirst(pathname,_A_NORMAL,&ffdta);
    if (!res) {
       t.tm_sec   = ((ffdta.wr_time & 0x001f) << 1);	     /* seconds after the minute -- [0,61] */
       t.tm_min   = ((ffdta.wr_time & 0x07e0) >> 5);	     /* minutes after the hour	 -- [0,59] */
       t.tm_hour  = ((ffdta.wr_time & 0xf800) >> 11);	     /* hours after midnight	 -- [0,23] */
       t.tm_mday  = (ffdta.wr_date & 0x001f);		     /* day of the month	 -- [1,31] */
       t.tm_mon   = ((ffdta.wr_date & 0x01e0) >> 5) - 1;     /* months since January	 -- [0,11] */
       t.tm_year  = (((ffdta.wr_date & 0xfe00) >> 9) + 80);  /* years since 1900		   */
       t.tm_isdst = 0;
       statbuf->st_atime = statbuf->st_mtime = statbuf->st_ctime = mktime(&t);
       statbuf->st_size  = ffdta.size;
    }

#if defined(__WATCOMC__)
     _dos_findclose(&ffdta);
#else
     while (!_dos_findnext(&ffdta));
#endif

     return (res ? -1 : 0);
}/*getstat()*/

#endif /*__OS2__*/


/* ------------------------------------------------------------------------- */
boolean fexist (char *pathname)
{
	return ((!access(pathname,0)) ? true : false);
}/*fexist()*/


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

	fd = sopen(name, access, shareflag, flags);

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
int nlgets (FILE *fp)
{
	char *p   = buffer;
	int   got = 0;
	int   c;

	while ((c = getc(fp)) != EOF && !ferror(fp) && got < MAX_BUFFER) {
	      if (c == '\032') break;
	      got++;
	      if (c == '\r' || c == '\n') {
		 c = getc(fp);
		 if (ferror(fp)) break;
		 if (c == '\r' || c == '\n') got++;
		 else			     ungetc(c,fp);
		 *p = '\0';
		 return (got);
	      }
	      *p++ = c;
	}

	if (ferror(fp)) {
	   printf("%s %lu: Error reading file\n",cur_name,cur_line);
	   return (-1);
	}
	if (got == MAX_BUFFER) {
	   printf("%s %lu: Line too long\n",cur_name,cur_line);
	   return (-1);
	}
	if (got) {
	   printf("%s %lu: Unexpected end of file\n",cur_name,cur_line);
	   return (-1);
	}

	return (0);
}/*nlgets()*/


/* ------------------------------------------------------------------------- */
boolean idxwrite (void *ptr, word len)
{
	if (fwrite(ptr,len,1,idxfp) != 1) {
	   printf("Error writing temporary index file '%s%s.I$$'\n",
		  nlp->idxpath,nlp->idxname);
	   return (false);
	}

	idxofs += len;

	return (true);
}/*idxwrite()*/


/* ------------------------------------------------------------------------- */
void *myalloc (word size)
{
	void *np;

	if ((np = malloc(size)) == NULL) {
	   printf("Error allocating memory\n");
	   exit (1);
	}
	memset(np,0,size);

	return (np);
}/*myalloc()*/


/* ------------------------------------------------------------------------- */
char *mystrdup (char *s)
{
	char *p = (char *) myalloc((word) (strlen(s) + 1));

	if (p) strcpy(p,s);

	return (p);
}/*mystrdup()*/


/* ------------------------------------------------------------------------- */
boolean findlist (char *filespec, char **foundname, word *file_flags)
{
	int d, thisyear, lastyear, special;
	int l;
	char *p;
	static char work[MAX_BUFFER + 1];

	l = strlen(filespec);
	if (l < 3 || strcmp(&filespec[l - 2],".*")) {
	   *foundname = mystrdup(filespec);
	   return (true);
	}

	thisyear = lastyear = special = -1;

	for (p = ffirst(filespec); p; p = fnext()) {
	    l = strlen(p);
	    if (l < 5 || p[l - 4] != '.') continue;
	    if (!isdigit(p[l - 3]) || !isdigit(p[l - 2]) || !isdigit(p[l - 1]))
	       continue;
	    d = atoi(&p[l - 3]);

	    if (d > 0 && d <= 366) {
	       if (d <= yearday) {
		  if (d > thisyear) thisyear = d;
	       }
	       else if (d > lastyear) lastyear = d;
	    }
	    else if (d > special) special = d;
	}
	fnclose();

	if	(special  >  0) d = special;
	else if (thisyear >  0) d = thisyear;
	else if (lastyear >  0) d = lastyear;
	else if (special  == 0) d = special;
	else {
	    printf("No file found for '%s'\n",filespec);
	    return (false);
	}

	strcpy(work,filespec);
	sprintf(&work[strlen(work) - 1],"%03u",d);
	*foundname = mystrdup(work);
	*file_flags |= NL_FPERIODIC;

	return (true);
}/*findlist()*/


/* ------------------------------------------------------------------------- */
boolean read_ctl (void)
{
	FILE *fp;
	char *option, *idxname, *parm1, *parm2, *parm3;
	char *p;
	word line, errors;
	NODELIST *nlp, **nlpp;
	struct stat st;

	sprintf(buffer,"%s.CFG",PRGNAME);
	if ((fp = sfopen(buffer,"rt",DENY_NONE)) == NULL) {
	   printf("Can't open control file '%s'\n",buffer);
	   return (false);
	}

	line = errors = 0;
	while (fgets(buffer,MAX_BUFFER,fp)) {
	      line++;
	      if ((p = strchr(buffer,';')) != NULL) *p = '\0';
	      if ((option  = strtok(buffer," \t\n")) == NULL) continue;
	      if (!stricmp(option,"delete")) {
		 delold = true;
		 continue;
	      }
#if USERLIST
	      if (!stricmp(option,"userlist")) {
		 userlist = true;
		 continue;
	      }
#endif
	      idxname = strtok(NULL," \t\n");
	      parm1   = strtok(NULL," \t\n");
	      parm2   = strtok(NULL," \t\n");
	      if (!idxname || !parm1) {
		 printf("Too few parms on line %hu of CFG file\n",line);
		 errors++;
		 continue;
	      }
	      parm3   = strtok(NULL," \t\n");

	      for (nlpp = &nodelist; (nlp = *nlpp) != NULL; nlpp = &nlp->next)
		  if (!stricmp(idxname,nlp->idxname)) break;
	      if (!nlp) {
		 nlp = myalloc(sizeof (NODELIST));
		 nlp->idxname = mystrdup(idxname);
		 nlp->next = *nlpp;
		 *nlpp = nlp;
	      }

	      if (!stricmp(option,"NodeList")) {
		 char work[MAX_BUFFER + 1];

		 if (nlp->idxpath) {
		    printf("Duplicate NODELIST entry on line %hu of CFG file\n",line);
		    errors++;
		    continue;
		 }

		 if (!parm2 || !parm3) {
		    printf("Too few NODELIST parms on line %hu of CFG file\n",line);
		    errors++;
		    continue;
		 }
		 strcpy(work,parm1);
		 if (work[strlen(work) - 1] != '\\')
		    strcat(work,"\\");
		 nlp->idxpath = mystrdup(work);
		 if (sscanf(parm2,"%hu",&nlp->zone) != 1) {
		    printf("Invalid NODELIST parm '%s' on line %hu of CFG file\n",parm2,line);
		    errors++;
		    continue;
		 }
		 if (!findlist(parm3,&nlp->basename,&nlp->file_flags)) {
		    errors++;
		    continue;
		 }
		 if (getstat(nlp->basename,&st)) {
		    printf("Can't read dir-info for nodelist basefile '%s'\n",nlp->basename);
		    errors++;
		    continue;
		 }
		 nlp->timestamp = st.st_mtime;
	      }
	      else if (!stricmp(option,"DelMerge") || !stricmp(option,"Merge")) {
		 MERGE *mp, **mpp;
		 word n = 0;

		 for (mpp = &nlp->merge; (mp = *mpp) != NULL; mpp = &mp->next, n++);
		 if (n == (NL_MAXFILES - 1)) {		/* plus basename */
		    printf("Index '%s%s' file table full (%u entries)\n",
			   nlp->idxname,nlp->idxname,(int) NL_MAXFILES);
		    errors++;
		    continue;
		 }
		 mp = myalloc(sizeof (MERGE));
		 mp->zone = mp->section = 0;
		 if (!stricmp(option,"DelMerge")) {
		    if (!parm2) {
		       printf("Too few DELMERGE parms on line %hu of CFG file\n",line);
		       errors++;
		       continue;
		    }
		    mp->section = 0;
		    if (strpbrk(parm1,"/.") ||
			sscanf(parm1,"%hu:%hu",&mp->zone,&mp->section) < 1) {
		       printf("Invalid DELMERGE parm '%s' on line %hu of CFG file\n",parm1,line);
		       errors++;
		       continue;
		    }
		    if (!mp->section && mp->zone) mp->section = mp->zone;
		    if (!findlist(parm2,&mp->filename,&mp->file_flags)) {
		       errors++;
		       continue;
		    }
		 }
		 else {
		    if (!findlist(parm1,&mp->filename,&mp->file_flags)) {
		       errors++;
		       continue;
		    }
		 }
		 if (getstat(mp->filename,&st)) {
		    printf("Can't read dir-info for (Del)Merge file '%s'\n",mp->filename);
		    errors++;
		    continue;
		 }
		 mp->timestamp = st.st_mtime;
		 mp->next = *mpp;
		 *mpp = mp;
	      }
	}

	fclose(fp);

	for (nlp = nodelist; nlp; nlp = nlp->next) {
	    if (!nlp->idxpath) {
	       printf("No NODELIST line in CFG file for domain '%s'\n",nlp->idxname);
	       errors++;
	       continue;
	    }
	    else {
	       int  i, len = strlen(nlp->idxname) + 4;
	       long hightime = 0L;
	       byte number   = 0;
	       NLHEADER nlh;

	       sprintf(buffer,"%s%s.I??",nlp->idxpath,nlp->idxname);
	       for (p = ffirst(buffer); p; p = fnext()) {
		   if (strlen(p) != len ||
		       !isdigit(p[len-2]) || !isdigit(p[len-1]))
		      continue;
		   sprintf(buffer,"%s%s",nlp->idxpath,p);
		   if ((fp = sfopen(buffer,"rb",DENY_NONE)) == NULL)
		      continue;
		   i = fread(&nlh,sizeof (NLHEADER),1,fp);
		   fclose(fp);
		   if (i != 1 ||
		       strncmp(nlh.idstring,NL_IDSTRING,sizeof (nlh.idstring)) ||
		       NL_REVMAJOR(nlh.revision) != NL_MAJOR ||
		       nlh.timestamp < hightime)
		      continue;
		   hightime = nlh.timestamp;
		   number = atoi(&p[len-2]);
	       }
	       fnclose();
	       nlp->old_timestamp = hightime;
	       nlp->number = number;
	    }
	}

	return (errors ? false : true);
}/*read_ctl()*/


/* ------------------------------------------------------------------------- */
boolean idxchange (NODELIST *nlp)
{
	FILE *fp = NULL;
	MERGE *mp;
	char   *nlname = NULL;
	NLFILE *nlfile = NULL;
	boolean change = true;
	word i;

	if (!nlp->old_timestamp)
	   goto fini;

	sprintf(buffer,"%s%s.I%02u",nlp->idxpath,nlp->idxname,(int) nlp->number);
	if ((fp = sfopen(buffer,"rb",DENY_NONE)) == NULL ||
	    fread(&nlheader,sizeof (NLHEADER),1,fp) != 1)
	   goto fini;

	for (mp = nlp->merge, i = 1; mp; mp = mp->next, i++);
	if (i != nlheader.files)
	   goto fini;

	if ((nlname = (char *) myalloc(nlheader.name_size)) == NULL ||
	    fseek(fp,nlheader.name_ofs,SEEK_SET) ||
	    fread(nlname,nlheader.name_size,1,fp) != 1 ||
	    (nlfile = (NLFILE *) myalloc((word) (nlheader.files * sizeof (NLFILE)))) == NULL ||
	    fseek(fp,nlheader.file_ofs,SEEK_SET) ||
	    fread(nlfile,sizeof (NLFILE) * nlheader.files,1,fp) != 1)
	   goto fini;

	if (strcmp(&nlname[nlfile[0].name_ofs],nlp->basename) ||
	    nlfile[0].timestamp != nlp->timestamp ||
	    nlfile[0].file_flags != nlp->file_flags)
	   goto fini;
	for (i = 1, mp = nlp->merge; i < nlheader.files; i++, mp = mp->next) {
	    if (stricmp(&nlname[nlfile[i].name_ofs],mp->filename) ||
		nlfile[i].timestamp != mp->timestamp ||
		nlfile[i].file_flags != mp->file_flags)
	       goto fini;
	}
	change = false;

fini:	if (fp) fclose(fp);
	if (nlname) free (nlname);
	if (nlfile) free (nlfile);

	return (change);
}/*idxchange()*/


/* ------------------------------------------------------------------------- */
void getfile (char *name, long timestamp, word file_flags)
{
	IDXFILE *np, **npp;

	for (npp = &idxfile; (np = *npp) != NULL; npp = &np->next) {
	    if (!stricmp(name,np->filename)) {
	       ifp = np;
	       return;
	    }
	}
	/* NL_MAXFILES checked when reading CFG */
	np = myalloc(sizeof (IDXFILE));
	np->file = nlheader.files++;
	np->filename = mystrdup(name);
	np->nlfile.timestamp  = timestamp;
	np->nlfile.file_flags = file_flags;
	np->next = *npp;
	*npp = np;

	ifp = np;
}/*getfile()*/


/* ------------------------------------------------------------------------- */
void getboss (word zone, word net, word node, boolean bossentry)
{
	IDXBOSS *np, **npp;

	for (npp = &idxboss; (np = *npp) != NULL; npp = &np->next) {
	    if (np->nlboss.zone > zone) break;
	    if (np->nlboss.zone < zone) continue;
	    if (np->nlboss.net > net) break;
	    if (np->nlboss.net < net) continue;
	    if (np->nlboss.node > node) break;
	    if (np->nlboss.node < node) continue;
	    ibp = np;
	    return;
	}
	np = myalloc(sizeof (IDXBOSS));
	np->nlboss.zone = zone;
	np->nlboss.net	= net;
	np->nlboss.node = node;
	if (bossentry) {
	   /* flags adjusted during write phase */
	   np->nlboss.entry = NL_ENTRY(cur_file,cur_ofs);
	}
	else {
	   np->nlboss.flags = nlnode.flags;
	   np->nlboss.entry = nlnode.entry;
	}
	np->next = *npp;
	*npp = np;
	nlheader.bosses++;

	total.boss++;
	ibp = np;
}/*getboss()*/


/* ------------------------------------------------------------------------- */
boolean addpoint (word point, int flags)
{
	IDXPOINT *np, **npp;

	if (!inetp) {
	   printf("%s %lu: Point %hu outside node\n", cur_name, cur_line, point);
	   return (false);
	}

	if (!ibp)
	   getboss(cur_zone,cur_net,nlnode.node,false);

	for (npp = &ibp->pointptr; (np = *npp) != NULL; npp = &np->next) {
	    if (point == np->nlpoint.point)
	       return (true);
#if 0
	    if (point == np->nlpoint.point) {
	       printf("%s %lu: Duplicate point number %hu:%hu/%hu.%hu\n",
		      cur_name, cur_line,
		      ibp->nlboss.zone, ibp->nlboss.net,
		      ibp->nlboss.node, point);
	       return (false);
	    }
#endif
	}
	np = myalloc(sizeof (IDXPOINT));
	np->nlpoint.point = point;
	np->nlpoint.flags = (word) flags;
	np->nlpoint.entry = NL_ENTRY(cur_file,cur_ofs);
	np->next	  = *npp;
	*npp = np;
	ibp->nlboss.points++;

	total.point++;
	return (true);
}/*addpoint()*/


/* ------------------------------------------------------------------------- */
boolean putnode (word node, int flags)
{
	if (!inetp) {
	   printf("%s %lu: Node %hu outside net\n", cur_name, cur_line, node);
	   return (false);
	}

	memset(&nlnode,0,sizeof (NLNODE));
	nlnode.node  = node;
	nlnode.flags = (word) flags;
	nlnode.entry = NL_ENTRY(cur_file,cur_ofs);
	idxwrite(&nlnode,sizeof (NLNODE));
	inetp->nlnet.nodes++;

	total.node++;
	switch (NL_TYPEMASK(flags)) {
	       case NL_ZC:   total.zone++;   break;
	       case NL_RC:   total.region++; break;
	       case NL_NC:   total.host++;   break;
	       case NL_HUB:  total.hub++;    break;
	       case NL_DOWN: total.down++;   break;
	       case NL_HOLD: total.hold++;   break;
	       case NL_PVT:  total.pvt++;    break;
	}

	ibp = NULL;

	return (true);
}/*putnode()*/


/* ------------------------------------------------------------------------- */
boolean addnet (word net, int flags)
{
	IDXNET *np, **npp;

	if (!izonep) {
	   if (!addzone(nlp->zone,(flags & ~NL_TYPEMASK(flags)) | NL_ZC))
	      return (false);
	   cur_region = nlp->zone;
	}

	for (npp = &izonep->netptr; (np = *npp) != NULL; npp = &np->next) {
	    if (net == np->nlnet.net) { 			/*AGL:23oct95*/
	       if (np->nlnet.nodes < 2) 			/*AGL:23oct95*/
		  return (true);				/*AGL:23oct95*/
	       printf("%s %lu: Duplicate net number %hu:%hu\n",
		      cur_name, cur_line, cur_zone, net);
	       return (false);					/*AGL:23oct95*/
	    }
	}
	np = myalloc(sizeof (IDXNET));
	np->nlnet.net	   = net;
	np->nlnet.flags    = (word) flags;
	np->nlnet.entry    = NL_ENTRY(cur_file,cur_ofs);
	np->nlnet.node_ofs = idxofs;
	np->next	   = *npp;
	*npp = np;
	izonep->nlzone.nets++;

	inetp = np;
	cur_net = net;

	return (putnode(0,flags));
}/*addnet()*/


/* ------------------------------------------------------------------------- */
boolean addzone (word zone, int flags)
{
	IDXZONE *np, **npp;

	for (npp = &idxzone;
	     (np = *npp) != NULL && np->nlzone.zone < zone;
	     npp = &np->next);
	if (np && zone == np->nlzone.zone) {
	   printf("%s %lu: Duplicate zone number %hu\n",
		  cur_name, cur_line, zone);
	   return (false);
	}
	np = myalloc(sizeof (IDXZONE));
	np->nlzone.zone  = zone;
	np->nlzone.flags = (word) flags;
	np->nlzone.entry = NL_ENTRY(cur_file,cur_ofs);
	np->next	 = *npp;
	*npp = np;
	nlheader.zones++;

	izonep = np;
	cur_zone = zone;
	return (addnet(zone,flags));
}/*addzone()*/


/* ------------------------------------------------------------------------- */
boolean write_net (void)
{
	void *next;

	if (inetp) {
	   izonep->nlzone.net_ofs = idxofs;
	   for (inetp = izonep->netptr; inetp; inetp = (IDXNET *) next) {
	       if (!idxwrite(&inetp->nlnet,sizeof (NLNET)))
		  return (false);
	       next = inetp->next;
	       free(inetp);
	   }
	}

	izonep = NULL;
	inetp = NULL;
	ibp = NULL;

	return (true);
}/*write_net()*/


/* ------------------------------------------------------------------------- */
boolean write_pointbossfilezone (void)
{
	void *next;
	NLNET  wnet;
	NLNODE wnode;
	word inet, inode;

	if (!write_net())
	   return (false);

	if (idxboss) {
	   for (ibp = idxboss; ibp; ibp = ibp->next) {
	       if (ibp->nlboss.points) {
		  /* Do nodelist lookup to update boss and node entry flags! */
		  for (izonep = idxzone; izonep; izonep = izonep->next) {
		      if (ibp->nlboss.zone == izonep->nlzone.zone) {
			 if (ibp->nlboss.net == ibp->nlboss.zone && !ibp->nlboss.node)
			    izonep->nlzone.flags |= NL_HASPOINTS;

			 if (fseek(idxfp,izonep->nlzone.net_ofs,SEEK_SET))
			    goto err;
			 for (inet = 0; inet < izonep->nlzone.nets; inet++) {
			     if (fread(&wnet,sizeof (NLNET),1,idxfp) != 1)
				goto err;
			     if (ibp->nlboss.net == wnet.net) {
				if (!ibp->nlboss.node) {
				   wnet.flags |= NL_HASPOINTS;
				   if (fseek(idxfp,-((long) sizeof (NLNET)),SEEK_CUR) ||
				       fwrite(&wnet,sizeof (NLNET),1,idxfp) != 1)
				      goto err;
				}

				if (fseek(idxfp,wnet.node_ofs,SEEK_SET))
				   goto err;
				for (inode = 0; inode < wnet.nodes; inode++) {
				    if (fread(&wnode,sizeof (NLNODE),1,idxfp) != 1)
				       goto err;
				    if (ibp->nlboss.node == wnode.node) {
				       wnode.flags |= NL_HASPOINTS;
				       if (fseek(idxfp,-((long) sizeof (NLNODE)),SEEK_CUR) ||
					   fwrite(&wnode,sizeof (NLNODE),1,idxfp) != 1)
					  goto err;

				       ibp->nlboss.flags = wnode.flags;
				       break;
				    }
				}
				break;
			     }
			 }

			 if (fseek(idxfp,0L,SEEK_END))
			    goto err;
			 break;
		      }
		  }

		  ibp->nlboss.point_ofs = idxofs;
		  for (ipp = ibp->pointptr; ipp; ipp = (IDXPOINT *) next) {
		      if (!idxwrite(&ipp->nlpoint,sizeof (NLPOINT)))
			 return (false);
		      next = ipp->next;
		      free(ipp);
		  }
	       }
	   }

	   nlheader.boss_ofs = idxofs;
	   for (ibp = idxboss; ibp; ibp = (IDXBOSS *) next) {
	       if (ibp->nlboss.points) {
		  if (!idxwrite(&ibp->nlboss,sizeof (NLBOSS)))
		     return (false);
	       }
	       else {
		  nlheader.bosses--;
		  total.boss--;
	       }
	       next = ibp->next;
	       free(ibp);
	   }
	}

	nlheader.name_ofs = idxofs;
	nlheader.name_size = 0;
	for (ifp = idxfile; ifp; ifp = ifp->next) {
	    ifp->nlfile.name_ofs = nlheader.name_size;
	    if (!idxwrite(ifp->filename,(word) (strlen(ifp->filename) + 1))) /*Incl.'\0'*/
	       return (false);
	    nlheader.name_size += (word) (strlen(ifp->filename) + 1);
	}

	nlheader.file_ofs = idxofs;
	for (ifp = idxfile; ifp; ifp = (IDXFILE *) next) {
	    if (!idxwrite(&ifp->nlfile,sizeof (NLFILE)))
	       return (false);
	    next = ifp->next;
	    free(ifp->filename);
	    free(ifp);
	}

	if (idxzone) {
	   nlheader.zone_ofs = idxofs;
	   for (izonep = idxzone; izonep; izonep = (IDXZONE *) next) {
	       if (!idxwrite(&izonep->nlzone,sizeof (NLZONE)))
		  return (false);
	       next = izonep->next;
	       free(izonep);
	   }
	}

	idxboss = ibp = NULL;
	idxfile = ifp = NULL;
	idxzone = izonep = NULL;

	return (true);

err:	printf("Error writing temporary index file '%s%s.I$$'\n",
	       nlp->idxpath,nlp->idxname);
	return (false);
}/*write_pointbossfilezone()*/


/* ------------------------------------------------------------------------- */
boolean process (char *name, long timestamp, word file_flags)
{
	FILE  *fp;
	int    got;
	int    type;
	word   number;
	char  *p;
#if USERLIST
	char  *username;
#endif
	MERGE *mp;
	char	 *old_name = cur_name;
	byte	  old_file = cur_file;
	FILE_OFS  old_ofs  = cur_ofs;
	dword	  old_line = cur_line;

	ibp = NULL;

	if ((fp = sfopen(name,"rb",DENY_NONE)) == NULL) {
	   printf("Can't open file '%s'\n",name);
	   return (false);
	}
	setvbuf(fp,NULL,_IOFBF,16384);

	getfile(name,timestamp,file_flags);
	cur_name = name;
	cur_file = ifp->file;

	if (fprintf(rptfp,"Reading '%s'\n",cur_name) <= 0) {
	   printf("Error writing temporary report file '%s%s.R$$'\n",
		  nlp->idxpath,nlp->idxname);
	   goto err;
	}

	for (cur_ofs = 0L, cur_line = 1;
	     (got = nlgets(fp)) > 0;
	     cur_ofs += got, cur_line++) {

	    if (do_break) goto err;

	    if (!buffer[0])
	       continue;

	    if (buffer[0] == ';') {
	       if (isalpha(buffer[1])) {
		  if (fprintf(rptfp,"%s\n",buffer) <= 0) {
		     printf("Error writing temporary report file '%s%s.R$$'\n",
			    nlp->idxpath,nlp->idxname);
		     goto err;
		  }
	       }
	       continue;
	    }

	    /* Allow digit here for point lists with missing leading comma */
	    if (buffer[0] == ',' || isdigit(buffer[0])) {
	       type = NL_NORM;
	       p = strtok(buffer,",");
	    }
	    else {
	       p = strtok(buffer,",");
	       if      (!stricmp(p,"Zone"))   type = NL_ZC;
	       else if (!stricmp(p,"Region")) type = NL_RC;
	       else if (!stricmp(p,"Host"))   type = NL_NC;
	       else if (!stricmp(p,"Hub"))    type = NL_HUB;
	       else if (!stricmp(p,"Down"))   type = NL_DOWN;
	       else if (!stricmp(p,"Hold"))   type = NL_HOLD;
	       else if (!stricmp(p,"Pvt"))    type = NL_PVT;
	       else if (!stricmp(p,"Boss"))   type = NL_BOSS;
	       else if (!stricmp(p,"Point"))  type = NL_POINT;
	       else			      type = NL_NORM;
	       p = strtok(NULL,",");
	    }
	    if (!p || !isdigit(*p)) {
	       printf("%s %u: Bad nodelist line\n",cur_name,cur_line);
	       goto err;
	    }
	    number = (word) atol(p);

	    switch (type) {
		   case NL_ZC:
			skip_zone = skip_region = skip_net = false;

			for (mp = nlp->merge; mp; mp = mp->next) {
			    if (mp->used || !mp->zone || mp->zone != cur_zone)
				continue;
			    mp->used = true;
			    if (!process(mp->filename,mp->timestamp,mp->file_flags))
			       goto err;
			}
			if (!write_net())
			   goto err;

			for (mp = nlp->merge; mp; mp = mp->next) {
			    if (mp->used || !mp->zone || 
				mp->zone != number || mp->section != number)
			       continue;
			    mp->used = true;
			    if (!process(mp->filename,mp->timestamp,mp->file_flags))
			       goto err;
			    skip_zone = true;
			    break;
			}
			break;

		   case NL_RC:
			skip_region = skip_net = false;

			for (mp = nlp->merge; mp; mp = mp->next) {
			    if (mp->used || !mp->zone ||
				mp->zone != cur_zone || mp->section != number)
			       continue;
			    mp->used = true;
			    if (!process(mp->filename,mp->timestamp,mp->file_flags))
			       goto err;
			    skip_region = true;
			    break;
			}
			break;

		   case NL_NC:
			skip_net = false;

			for (mp = nlp->merge; mp; mp = mp->next) {
			    if (mp->used || !mp->zone ||
				mp->zone != cur_zone || mp->section != number)
			       continue;
			    mp->used = true;
			    if (!process(mp->filename,mp->timestamp,mp->file_flags))
			       goto err;
			    skip_net = true;
			    break;
			}
			break;

		   default:
			/* nothing */
			break;
	    }

	    if (skip_zone || skip_region || skip_net) continue;

	    switch (type) {
		   case NL_ZC:
			if (!addzone(number,type))
			   goto err;
			cur_region = number;
			fprintf(stderr,"%23.23s  Zone %5u\r", "", cur_zone);
			break;

		   case NL_RC:
			if (!addnet(number,type))
			   goto err;
			cur_region = number;
			fprintf(stderr,"%9.9s  Region %5u\r", "",cur_region);
			break;

		   case NL_NC:
			if (!addnet(number,type))
			   goto err;
			fprintf(stderr,"Net %5u\r", cur_net);
			break;

		   case NL_HUB:
			ibp = NULL;
			/* hub fallthrough to down/hold/pvt/norm */

		   case NL_DOWN:
		   case NL_HOLD:
		   case NL_PVT:
		   case NL_NORM:
			if (ibp) {
			   if (!addpoint(number,type))
			      goto err;
			}
			else {
			   if (!putnode(number,type))
			      goto err;
			}
			break;

		   case NL_BOSS:
			{ word zone, net, node;

			  if (sscanf(p,"%hu:%hu/%hu",&zone,&net,&node) != 3) {
			     printf("%s %lu: Bad boss entry\n",cur_name,cur_line);
			     goto err;
			  }
			  getboss(zone,net,node,true);
			}
			break;

		   case NL_POINT:
			/*ibp = NULL;*/
			if (!addpoint(number,NL_NORM))
			   goto err;
			break;
	    }

	    strtok(NULL,",");			/* Name     */
	    strtok(NULL,",");			/* Location */
#if USERLIST
	    username = strtok(NULL,",");	/* Sysop    */
#else
	    strtok(NULL,",");			/* Sysop    */
#endif
	    strtok(NULL,",");			/* Phone    */
	    strtok(NULL,",");			/* Speed    */
	    while ((p = strtok(NULL,",")) != NULL) {
		  char work[MAX_BUFFER + 1];

		  strupr(p);
		  if (p[0] != 'G')
		     continue;
		  p++;
		  *work = '\0';
		  switch (type) {
			 case NL_ZC:
			 case NL_RC:
			 case NL_NC:	sprintf(work,"%u:%u/0",
						cur_zone,cur_net);
					break;
			 case NL_DOWN:	/* Don't use */
					break;
			 case NL_HUB:
			 case NL_HOLD:
			 case NL_PVT:
			 case NL_NORM:	if (ibp)
					   sprintf(work,"%u:%u/%u.%u",
						   ibp->nlboss.zone,ibp->nlboss.net,
						   ibp->nlboss.node,number);
					else
					   sprintf(work,"%u:%u/%u",
						   cur_zone,cur_net,number);
					break;
			 case NL_BOSS:	sprintf(work,"%u:%u/%u",
						ibp->nlboss.zone,ibp->nlboss.net,
						ibp->nlboss.node);
					break;
			 case NL_POINT: sprintf(work,"%u:%u/%u.%u",
						ibp->nlboss.zone,ibp->nlboss.net,
						ibp->nlboss.node,number);
					break;
		  }

		  if (work[0]) {
		     if (fprintf(gatefp,"%s %s %u\n",p,work,cur_region) <= 0) {
			printf("Error writing temporary gate list '%s%s.G$$'\n",
			       nlp->idxpath,nlp->idxname);
			goto err;
		     }
		     total.gate++;
		  }
	    }

#if USERLIST
	    if (userlist && username) {
	       char work[MAX_BUFFER + 1];

	       for (p = username; (p = strchr(p,'_')) != NULL; *p++ = ' ');
	       *work = '\0';
	       switch (type) {
		      case NL_ZC:
		      case NL_RC:
		      case NL_NC:    sprintf(work,"%u:%u/0",
					     cur_zone,cur_net);
				     break;
		      case NL_DOWN:  /* Don't use */
				     break;
		      case NL_HUB:
		      case NL_HOLD:
		      case NL_PVT:
		      case NL_NORM:  if (ibp)
					sprintf(work,"%u:%u/%u.%u",
						ibp->nlboss.zone,ibp->nlboss.net,
						ibp->nlboss.node,number);
				     else
					sprintf(work,"%u:%u/%u",
						cur_zone,cur_net,number);
				     break;
		      case NL_BOSS:  sprintf(work,"%u:%u/%u",
					     ibp->nlboss.zone,ibp->nlboss.net,
					     ibp->nlboss.node);
				     break;
		      case NL_POINT: sprintf(work,"%u:%u/%u.%u",
					     ibp->nlboss.zone,ibp->nlboss.net,
					     ibp->nlboss.node,number);
				     break;
	       }

	       if (work[0]) {
		  if (fprintf(userfp,"%s,%s\n",username,work) <= 0) {
		     printf("Error writing temporary user list '%s%s.U$$'\n",
			    nlp->idxpath,nlp->idxname);
		     goto err;
		  }
		  total.user++;
	       }
	    }
#endif

	    if (type == NL_POINT)
	       ibp = NULL;
	}
	fclose(fp);
	fp = NULL;

	if (got < 0) {
err:	   if (fp) fclose(fp);
	   return (false);
	}

	fprintf(rptfp,"Finished with '%s'\n",cur_name);
	ibp = NULL;
	cur_name = old_name;
	cur_file = old_file;
	cur_ofs  = old_ofs;
	cur_line = old_line;

	return (true);
}/*process()*/


/* ------------------------------------------------------------------------- */
OLDFILE *getoldfile (OLDFILE **base, char *filename)
{
	OLDFILE *op, **opp;

	for (opp = base; (op = *opp) != NULL; opp = &op->next)
	    if (!stricmp(filename,op->filename)) break;
	if (!op) {
	   op = myalloc(sizeof (OLDFILE));
	   op->filename = mystrdup(filename);
	   op->next = *opp;
	   *opp = op;
	}

	return (op);
}/*getoldfile()*/


/* ------------------------------------------------------------------------- */
void deloldidx (void)
{
	FILE	*fp;
	MERGE	*mp;
	OLDIDX	*oldidx  = NULL, *ip;
	OLDFILE *oldfile = NULL, *op;
	char	*nlname  = NULL;
	NLFILE	*nlfile  = NULL;
	int	 len;
	word	 i;
	char	*p;

	for (nlp = nodelist; nlp; nlp = nlp->next) {
	    sprintf(buffer,"%s%s.I??",nlp->idxpath,nlp->idxname);
	    len = strlen(nlp->idxname) + 4;
	    for (p = ffirst(buffer); p; p = fnext()) {
		if (strlen(p) != len ||
		    !isdigit(p[len-2]) || !isdigit(p[len-1]))
		   continue;

		ip = myalloc(sizeof (OLDIDX));
		ip->nlp    = nlp;
		ip->number = atoi(&p[len-2]);
		ip->next   = oldidx;
		oldidx = ip;
	    }
	    fnclose();

	    getoldfile(&oldfile,nlp->basename)->keep = true;
	    for (mp = nlp->merge; mp; mp = mp->next)
		getoldfile(&oldfile,mp->filename)->keep = true;
	}

	for (ip = oldidx; ip; ip = ip->next) {
	    sprintf(buffer,"%s%s.I%02u",ip->nlp->idxpath,ip->nlp->idxname,ip->number);
	    if ((fp = sfopen(buffer,"rb",DENY_NONE)) == NULL)
	       return;

	    if (fread(&nlheader,sizeof (NLHEADER),1,fp) != 1 ||
		strncmp(nlheader.idstring,NL_IDSTRING,sizeof (nlheader.idstring)) ||
		NL_REVMAJOR(nlheader.revision) != NL_MAJOR ||
		(nlname = (char *) malloc(nlheader.name_size)) == NULL ||
		fseek(fp,nlheader.name_ofs,SEEK_SET) ||
		fread(nlname,nlheader.name_size,1,fp) != 1 ||
		(nlfile = (NLFILE *) malloc(nlheader.files * sizeof (NLFILE))) == NULL ||
		fseek(fp,nlheader.file_ofs,SEEK_SET) ||
		fread(nlfile,sizeof (NLFILE) * nlheader.files,1,fp) != 1) {
	       fclose(fp);
	       if (nlname) free(nlname);
	       if (nlfile) free(nlfile);
	       return;
	    }

	    fclose(fp);

	    for (i = 0; i < nlheader.files; i++) {
		op = getoldfile(&oldfile,&nlname[nlfile[i].name_ofs]);
		if (!(nlfile[i].file_flags & NL_FPERIODIC))
		   op->keep = true;
		else if (!op->keep)
		   op->check = true;
	    }

	    if (ip->number == ip->nlp->number || unlink(buffer)) {
	       for (op = oldfile; op; op = op->next)
		   if (op->check) op->keep = true;
	    }
	    else {
	       fprintf(stderr,"Deleting old index %s                    \n",
		       buffer);
	       sprintf(buffer,"%s%s.G%02u",ip->nlp->idxpath,ip->nlp->idxname,ip->number);
	       if (fexist(buffer)) unlink(buffer);
	       sprintf(buffer,"%s%s.R%02u",ip->nlp->idxpath,ip->nlp->idxname,ip->number);
	       if (fexist(buffer)) unlink(buffer);
	       sprintf(buffer,"%s%s.U%02u",ip->nlp->idxpath,ip->nlp->idxname,ip->number);
	       if (fexist(buffer)) unlink(buffer);
	    }

	    for (op = oldfile; op; op = op->next)
		op->check = false;

	    free(nlname); nlname = NULL;
	    free(nlfile); nlfile = NULL;
	}

	if (delold) {
	   for (op = oldfile; op; op = op->next) {
	       if (!op->keep && fexist(op->filename)) {
		  fprintf(stderr,"Deleting old file %s                    \n",
			  op->filename);
		  unlink(op->filename);
	       }
	   }
	}
}/*deloldidx()*/


/* ------------------------------------------------------------------------- */
#if defined(__MSDOS__)
static int __far critical_handler (unsigned __deverr, unsigned __errcode, unsigned __far *__devhdr)
{
	__deverr  = __deverr;
	__errcode = __errcode;
	__devhdr  = __devhdr;

	return (3);	/* FAIL */
}/*critical_handler()*/
#endif


/* ------------------------------------------------------------------------- */
void break_handler (int sig)
{
	sig = sig;

	signal(SIGINT,SIG_IGN);
	do_break = true;
	signal(SIGINT,break_handler);
}/*break_handler()*/


/* ------------------------------------------------------------------------- */
void main (int argc, char *argv[])
{
	char tmpname[MAX_BUFFER + 1];
	byte newnumber;
	FILE *fp;
	struct tm *t;
	long l;
	int i;
	MERGE *mp;

	signal(SIGINT,break_handler);

#if defined(__MSDOS__)
#if defined(__WATCOMC__)
	_harderr(critical_handler);
#else
	harderr(critical_handler);
#endif
#endif

	fprintf(stderr,"%s Version %s %s (NL format revision %u.%02u); Nodelist index processor\n",
		       PRGNAME, VERSION, NIX_OS, (int) NL_MAJOR, (int) NL_MINOR);
	fprintf(stderr,"Design & COPYRIGHT (C) 1993-1995 by Arjen G. Lentz; ALL RIGHTS RESERVED\n");
	fprintf(stderr,"%s is a project of LENTZ SOFTWARE-DEVELOPMENT, Amersfoort, The Netherlands\n\n",
		       PRGNAME);

	putenv("TZ=GMT0");
	tzset();
	l = time(NULL);
	t = localtime((time_t *) &l);
	yearday = (word) (t->tm_yday + 1);

	nodelist = NULL;
	tmpname[0] = '\0';
	idxfp = NULL;

	if (!read_ctl())
	   exit (255);

	for (nlp = nodelist; nlp && !do_break; nlp = nlp->next) {
	    if (argc >= 2) {
	       for (i = 1; i < argc; i++) {
		   if (!stricmp(argv[i],nlp->idxname) ||
		       !stricmp(argv[i],"*") || !stricmp(argv[i],"All"))
		      break;
	       }
	       if (i == argc)
		  continue;
	    }
	    else if (!idxchange(nlp)) {
	       fprintf(stderr,"Skipping unchanged %s                                        \n",
			      nlp->idxname);
	       continue;
	    }

	    if (nlp->old_timestamp)
	       newnumber = (nlp->number + 1) % 100;
	    else
	       newnumber = nlp->number;

	    fprintf(stderr,"Processing %s (default zone %hu) %s               \n",
			   nlp->idxname, nlp->zone, nlp->idxpath);

	    memset(&nlheader,0,sizeof (NLHEADER));
	    strncpy(nlheader.idstring,NL_IDSTRING,sizeof (nlheader.idstring));
	    sprintf(nlheader.creator,"%s %s %s (NLrev %u.%02u)\032",
				     PRGNAME, VERSION, NIX_OS,
				     (int) NL_MAJOR, (int) NL_MINOR);
	    nlheader.revision = NL_REVISION;
	    nlheader.timestamp = time(NULL);

	    idxfile = NULL;
	    idxzone = izonep = NULL;
	    inetp = NULL;
	    idxboss = ibp = NULL;
	    ipp = NULL;
	    cur_zone = cur_region = cur_net = 0;
	    skip_zone = skip_region = skip_net = false;
	    memset(&total,0,sizeof (struct _total));
	    idxfp = gatefp = rptfp = NULL;

	    sprintf(tmpname,"%s%s.I$$",nlp->idxpath,nlp->idxname);
	    if (fexist(tmpname)) unlink(tmpname);
	    if ((idxfp = sfopen(tmpname,"w+b",DENY_NONE)) == NULL) {  /* We need to read too */
	       printf("Can't create temporary index '%s'\n",tmpname);
	       goto err;
	    }
	    setvbuf(idxfp,NULL,_IOFBF,16384);
	    idxofs = 0L;
	    if (!idxwrite(&nlheader,sizeof (NLHEADER)))
	       goto err;

	    sprintf(buffer,"%s%s.G$$",nlp->idxpath,nlp->idxname);
	    if ((gatefp = sfopen(buffer,"wt",DENY_NONE)) == NULL) {
	       printf("Can't create temporary gate list '%s'\n",buffer);
	       goto err;
	    }
	    setvbuf(gatefp,NULL,_IOFBF,16384);

	    sprintf(buffer,"%s%s.R$$",nlp->idxpath,nlp->idxname);
	    if ((rptfp = sfopen(buffer,"wt",DENY_NONE)) == NULL) {
	       printf("Can't create temporary report file '%s'\n",buffer);
	       goto err;
	    }
	    setvbuf(rptfp,NULL,_IOFBF,16384);

#if USERLIST
	    if (userlist) {
	       sprintf(buffer,"%s%s.U$$",nlp->idxpath,nlp->idxname);
	       if ((userfp = sfopen(buffer,"wt",DENY_NONE)) == NULL) {
		  printf("Can't create temporary user file '%s'\n",buffer);
		  goto err;
	       }
	       setvbuf(userfp,NULL,_IOFBF,16384);
	    }
#endif

	    if (!process(nlp->basename,nlp->timestamp,nlp->file_flags))
	       goto err;

	    skip_zone = skip_region = skip_net = false;
	    for (mp = nlp->merge; mp; mp = mp->next) {
		if (mp->used) continue;
		mp->used = true;
		if (!process(mp->filename,mp->timestamp,mp->file_flags))
		   goto err;
	    }

	    if (!write_pointbossfilezone())
	       goto err;

	    fclose(gatefp);
	    gatefp = NULL;

#if USERLIST
	    if (userlist) {
	       fclose(userfp);
	       userfp = NULL;
	    }
#endif

	    fprintf(rptfp,"\n\n");
	    fprintf(rptfp,"    Network size report of all files in this nodelist:\n");
	    fprintf(rptfp,"\n");
	    fprintf(rptfp,"    Total listed nodes      %6lu\n",total.node);
	    fprintf(rptfp,"    Nodes down              %6lu\n",total.down);
	    fprintf(rptfp,"    Nodes on hold           %6lu\n",total.hold);
	    fprintf(rptfp,"                            ------\n");
	    total.open = total.node - (total.down + total.hold);
	    fprintf(rptfp,"    Total open nodes        %6lu\n",total.open);
	    fprintf(rptfp,"    Zone coordinators       %6lu\n",total.zone);
	    fprintf(rptfp,"    Regional coordinators   %6lu\n",total.region);
	    fprintf(rptfp,"    Network hosts           %6lu\n",total.host);
	    fprintf(rptfp,"    Network hubs            %6lu\n",total.hub);
	    fprintf(rptfp,"                            ------\n");
	    total.separate = total.open - (total.zone + total.region + total.host + total.hub);
	    fprintf(rptfp,"    Total separate nodes    %6lu\n",total.separate);
	    fprintf(rptfp,"    Private nodes           %6lu\n",total.pvt);
	    fprintf(rptfp,"                            ======\n");
	    total.accessible = total.separate - total.pvt;
	    fprintf(rptfp,"    Total accessible nodes  %6lu\n",total.accessible);
	    fprintf(rptfp,"\n");
	    fprintf(rptfp,"    Total domain gates      %6lu\n",total.gate);
	    fprintf(rptfp,"\n");
	    fprintf(rptfp,"    Total boss nodes        %6lu\n",total.boss);
	    fprintf(rptfp,"    Total point systems     %6lu\n",total.point);
#if USERLIST
	    if (userlist) {
	       fprintf(rptfp,"\n");
	       fprintf(rptfp,"    Total users in list     %6lu\n",total.user);
	    }
#endif
	    fprintf(rptfp,"\n\n");

	    fclose(rptfp);
	    rptfp = NULL;

	    if (fseek(idxfp,0L,SEEK_SET) ||  /* Mucks up idxofs, but don't need */
		!idxwrite(&nlheader,sizeof (NLHEADER)))
	       goto err;
	    fclose(idxfp);
	    idxfp = NULL;

	    sprintf(buffer,"%s%s.I%02u", nlp->idxpath, nlp->idxname, (int) newnumber);
	    if (fexist(buffer) && unlink(buffer)) {
	       printf("Can't delete old index '%s'\n",buffer);
	       goto err;
	    }
	    sprintf(tmpname,"%s%s.I$$", nlp->idxpath, nlp->idxname);
	    if (rename(tmpname,buffer)) {
	       printf("Can't rename index '%s' -> '%s'\n",tmpname,buffer);
	       goto err;
	    }

	    sprintf(buffer,"%s%s.G%02u", nlp->idxpath, nlp->idxname, (int) newnumber);
	    if (fexist(buffer) && unlink(buffer)) {
	       printf("Can't delete old gate list '%s'\n",buffer);
	       goto err;
	    }
	    sprintf(tmpname,"%s%s.G$$", nlp->idxpath, nlp->idxname);
	    if (rename(tmpname,buffer)) {
	       printf("Can't rename gate list '%s' -> '%s'\n",tmpname,buffer);
	       goto err;
	    }

	    sprintf(buffer,"%s%s.R%02u", nlp->idxpath, nlp->idxname, (int) newnumber);
	    if (fexist(buffer) && unlink(buffer)) {
	       printf("Can't delete old gate list '%s'\n",buffer);
	       goto err;
	    }
	    sprintf(tmpname,"%s%s.R$$", nlp->idxpath, nlp->idxname);
	    if (rename(tmpname,buffer)) {
	       printf("Can't rename report file '%s' -> '%s'\n",tmpname,buffer);
	       goto err;
	    }

	    sprintf(buffer,"%s%s.FLG", nlp->idxpath, nlp->idxname);
	    if ((fp = sfopen(buffer,"wb",DENY_NONE)) != NULL)
	       fclose(fp);

#if USERLIST
	    if (userlist) {
	       sprintf(buffer,"%s%s.U%02u", nlp->idxpath, nlp->idxname, (int) newnumber);
	       if (fexist(buffer) && unlink(buffer)) {
		  printf("Can't delete old user list '%s'\n",buffer);
		  goto err;
	       }
	       sprintf(tmpname,"%s%s.U$$", nlp->idxpath, nlp->idxname);
	       if (rename(tmpname,buffer)) {
		  printf("Can't rename user file '%s' -> '%s'\n",tmpname,buffer);
		  goto err;
	       }
	    }
#endif

	    nlp->number = newnumber;
	    continue;

err:	    if (!do_break)
	       printf("Error processing %s\n",nlp->idxname);

	    if (idxfp)	fclose(idxfp);
	    sprintf(tmpname,"%s%s.I$$", nlp->idxpath, nlp->idxname);
	    if (fexist(tmpname)) unlink(tmpname);
	    sprintf(tmpname,"%s%s.I%02u", nlp->idxpath, nlp->idxname, (int) newnumber);
	    if (fexist(tmpname)) unlink(tmpname);

	    if (gatefp) fclose(gatefp);
	    sprintf(tmpname,"%s%s.G$$", nlp->idxpath, nlp->idxname);
	    if (fexist(tmpname)) unlink(tmpname);
	    sprintf(tmpname,"%s%s.G%02u", nlp->idxpath, nlp->idxname, (int) newnumber);
	    if (fexist(tmpname)) unlink(tmpname);

	    if (rptfp)	fclose(rptfp);
	    sprintf(tmpname,"%s%s.R$$", nlp->idxpath, nlp->idxname);
	    if (fexist(tmpname)) unlink(tmpname);
	    sprintf(tmpname,"%s%s.R%02u", nlp->idxpath, nlp->idxname, (int) newnumber);
	    if (fexist(tmpname)) unlink(tmpname);

#if USERLIST
	    if (userlist) {
	       if (userfp) fclose(userfp);
	       sprintf(tmpname,"%s%s.U$$", nlp->idxpath, nlp->idxname);
	       if (fexist(tmpname)) unlink(tmpname);
	       sprintf(tmpname,"%s%s.U%02u", nlp->idxpath, nlp->idxname, (int) newnumber);
	       if (fexist(tmpname)) unlink(tmpname);
	    }
#endif
	}

	if (do_break)
	   fprintf(stderr,"Abort by operator                      \n");
	else {
	   deloldidx();
	   fprintf(stderr,"Finished!                              \n");
	}

	exit (0);
}/*main()*/


/* end of nodeidx.c */
