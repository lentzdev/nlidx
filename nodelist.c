/* Part of Xenia Edit; this source by itself is NOT compilable, it just    */
/* serves as an example of how nodeidx can be used to look up a node/point */

/* all kinds of includes */
#include "2types.h"


typedef struct _domain DOMAIN;
struct _domain {                        /* --------------------------------- */
        char    *domname;               /* Full domain name                  */
        char    *abbrev;                /* 8-char abbreviation               */
        char    *idxbase;               /* base of idx filename (NULL=none)  */
        byte     number;                /* Number of index used              */
        long     timestamp;             /* Timestamp of index used           */
        long     flagstamp;             /* Timestamp of the change flag file */
        boolean  errors;                /* Can't use THIS index now          */
        DOMAIN  *next;                  /* Ptr to next item, NULL = end list */
};
typedef struct _dompat DOMPAT;
struct _dompat {                        /* --------------------------------- */
        char   *pattern;                /* Domain pattern for conversion     */
        DOMAIN *domptr;                 /* Pointer to right domain structure */
        DOMPAT *next;                   /* Ptr to next item, NULL = end list */
};


/* ------------------------------------------------------------------------- */
/* Domain/nodeidx configuration lines, as used by Xenia/XenEdit:
   Even with 'nolist' these lines are important, because they resolve domain
   name differences between nutty users who don't know what a domain is.
   DOMAIN <longname> <short> <index path+basename>    <domain resolve patterns>
   ;DOMAIN fidonet.org  FidoNet C:\XENIA\NODELIST\FIDONET 1:* 2:* 3:* 4:* 5:* 6:* *fido*
   ;DOMAIN blabla.ftn   BlaBla  C:\XENIA\NODELIST\BLABLA  99:* *blabla*
   ;DOMAIN yanet        YANet   nolist                    1313:* *yanet*
*/

/* This is part of Xenia/XenEdit's config parsing code */
#if 0
   if (!strcmp(option,"domain")) {
      DOMAIN *domp, **dompp;
      DOMPAT *patp, **patpp;
      char *domname, *abbrev, *idxbase;

      domname = strtok(parm," ");
      abbrev  = strtok(NULL," ");
      idxbase = strtok(NULL," ");
      if (!domname || !abbrev || !idxbase) {
         fprintf(stderr,"\nInvalid number of domain parms\n");
         exit (ERL_FATAL);
      }
      if (strlen(abbrev) > 8) abbrev[8] = '\0';

      for (dompp = &domain; (domp = *dompp) != NULL; dompp = &domp->next) {
          i = stricmp(domp->domname,domname);
          if (!i) {
             fprintf(stderr,"\nDuplicate domain name '%s'\n",domname);
             exit (ERL_FATAL);
          }
          if (i > 0) break;          /* sort low to high */
      }
      domp = (DOMAIN *) malloc (sizeof (DOMAIN));
      if (!domp) {
         fprintf(stderr,"\nCan't allocate space for domain struct\n");
         exit (ERL_FATAL);
      }
      domp->domname   = ctl_string(domname);
      domp->abbrev    = ctl_string(abbrev);
      domp->idxbase   = stricmp(idxbase,"nolist") ? ctl_string(idxbase) : NULL;
      domp->timestamp = 0L;
      domp->flagstamp = 0L;
      domp->number    = 0;
      domp->errors    = false;
      domp->next      = *dompp;
      *dompp = domp;

      while ((p = strtok(NULL," ")) != NULL) {
            strlwr(p);
            for (patpp = &dompat; (patp = *patpp) != NULL; patpp = &patp->next) {
                i = strcmp(patp->pattern,p);
                if (i <= 0) break;     /* sort high to low !! */
            }
            if (!i) continue;          /* Duplicate pattern.. */
            patp = (DOMPAT *) malloc (sizeof (DOMPAT));
            if (!patp) {
               fprintf(stderr,"\nCan't allocate space for dompat struct\n");
               exit (ERL_FATAL);
            }
            patp->pattern = ctl_string(p);
            patp->domptr  = domp;
            patp->next    = *patpp;
            *patpp = patp;
      }
   }
#endif
/* ------------------------------------------------------------------------- */


typedef struct _adrinfo ADRINFO;
struct _adrinfo {
        char *address;
        char *info;
        long  used;
};
#define NUM_ADRINFO 50

typedef struct _nlentry NLENTRY;
struct _nlentry {
        char *name, *location, *sysop, *phone, *flags;
};

#define MAX_BUFFER 256

static ADRINFO *adrinfo   = NULL;
static char    *idxbase   = NULL;
static FILE    *idxfp     = NULL;
static int      last_zi   = 0,
                last_ni   = 0,
                last_bi   = 0;

static NLHEADER nlheader;
static char    *nlname  = NULL;
static NLFILE  *nlfile  = NULL;
static NLZONE  *nlzone  = NULL;
static NLNET   *nlnet   = NULL;
static NLNODE  *nlnode  = NULL;
static NLBOSS  *nlboss  = NULL;
static NLPOINT *nlpoint = NULL;

static NLENTRY  nlentry;


/* ------------------------------------------------------------------------- */
static boolean getline (long entry)
{
        static char      buffer[MAX_BUFFER + 1];
        static FILE     *nlfp = NULL;
        static char     *cur_name = NULL;
        static FILE_OFS  cur_ofs;
        char *p = buffer;
        word  got = 0;
        int   c;

        p = &nlname[nlfile[NL_ENTFILE(entry)].name_ofs];
        if (!nlfp || stricmp(p,cur_name)) {
           if (nlfp) fclose(nlfp);
           if (cur_name) free(cur_name);
           cur_name = strdup(p);
           if ((nlfp = sfopen(cur_name,"rb",DENY_NONE)) == NULL)
              return (false);
           cur_ofs = 0L;
        }

        if (NL_ENTOFS(entry) != cur_ofs) {
           cur_ofs = NL_ENTOFS(entry);
           fseek(nlfp,cur_ofs,SEEK_SET);
        }

        p = buffer;
        while ((c = getc(nlfp)) != EOF && !ferror(nlfp) && got < MAX_BUFFER) {
              if (c == '\032') break;
              got++;
              if (c == '\r' || c == '\n') {
                 c = getc(nlfp);
                 if (ferror(nlfp)) break;
                 if (c == '\r' || c == '\n') got++;
                 else                        ungetc(c,nlfp);
                 *p = '\0';
                 cur_ofs += got;

                 for (p = buffer; *p; p++)
                     if (*p == '_') *p = ' ';
                 if (strtok(buffer,",") == buffer)
                    strtok(NULL,",");
                 nlentry.name     = strtok(NULL,",");
                 nlentry.location = strtok(NULL,",");
                 nlentry.sysop    = strtok(NULL,",");
                 nlentry.phone    = strtok(NULL,",");
                 if ((nlentry.flags = strtok(NULL,"")) == NULL)
                    nlentry.flags = "";
                 else if (*nlentry.flags == ',')
                    nlentry.flags++;

                 return (nlentry.phone ? true : false);
              }
              *p++ = c;
        }

        clearerr(nlfp);
        cur_ofs = ftell(nlfp);

        return (false);
}


/* ------------------------------------------------------------------------- */
boolean findidx (DOMAIN *domp)
{
        FILE    *newfp;
        NLHEADER nlh, newnlh;
        char     buffer[MAX_BUFFER + 1];
        int      len, i, dummy;
        byte     number;
        boolean  found = false;
        char    *p;

        if (!adrinfo) {
           if ((adrinfo = (ADRINFO *) malloc(NUM_ADRINFO * sizeof (ADRINFO))) == NULL)
              return (false);
           memset(adrinfo,0,NUM_ADRINFO * sizeof (ADRINFO));
        }

        if (domp->timestamp) {
           sprintf(buffer,"%s.I%02u",domp->idxbase,(int) domp->number);
           if ((newfp = sfopen(buffer,"rb",DENY_NONE)) != NULL) {
              if (fread(&newnlh,sizeof (NLHEADER),1,newfp) != 1 ||
                  strncmp(newnlh.idstring,NL_IDSTRING,sizeof (newnlh.idstring)) ||
                  NL_REVMAJOR(newnlh.revision) != NL_MAJOR) {
                 fclose(newfp);
              }
              else {
                 domp->timestamp = newnlh.timestamp;
                 memmove(&nlh,&newnlh,sizeof (NLHEADER));
                 if (idxfp) fclose(idxfp);
                 idxfp = newfp;
                 found = true;
              }
           }
        }

        if (!found) {
           sprintf(buffer,"%s.I??",domp->idxbase);
           for (p = ffirst(buffer); p; p = fnext()) {
               len = strlen(p);
               if (!isdigit(p[len-2]) || !isdigit(p[len-1]))
                  continue;
               number = atoi(&p[len-2]);
               sprintf(buffer,"%s.I%02u",domp->idxbase,(int) number);
               if ((newfp = sfopen(buffer,"rb",DENY_NONE)) == NULL)
                  continue;

               if (fread(&newnlh,sizeof (NLHEADER),1,newfp) != 1 ||
                   strncmp(newnlh.idstring,NL_IDSTRING,sizeof (newnlh.idstring)) ||
                   NL_REVMAJOR(newnlh.revision) != NL_MAJOR ||
                   newnlh.timestamp < domp->timestamp) {
                  fclose(newfp);
                  continue;
               }

               domp->number    = number;
               domp->timestamp = newnlh.timestamp;
               memmove(&nlh,&newnlh,sizeof (NLHEADER));
               if (idxfp) fclose(idxfp);
               idxfp = newfp;
               found = true;
           }
           fnclose();
        }

        if (found) {
           if (nlname)  { free(nlname);  nlname  = NULL; }
           if (nlfile)  { free(nlfile);  nlfile  = NULL; }
           if (nlzone)  { free(nlzone);  nlzone  = NULL; }
           if (nlnet)   { free(nlnet);   nlnet   = NULL; }
           if (nlnode)  { free(nlnode);  nlnode  = NULL; }
           if (nlboss)  { free(nlboss);  nlboss  = NULL; }
           if (nlpoint) { free(nlpoint); nlpoint = NULL; }

           for (i = 0; i < NUM_ADRINFO; i++) {
               if (adrinfo[i].used &&
                   (parseaddress(adrinfo[i].address,&dummy,&dummy,&dummy,&dummy,buffer),
                    !stricmp(buffer,domp->abbrev))) {
                  free(adrinfo[i].address);
                  free(adrinfo[i].info);
                  adrinfo[i].address = adrinfo[i].info = NULL;
                  adrinfo[i].used = 0L;
               }
           }

           idxbase = domp->idxbase;
           memmove(&nlheader,&nlh,sizeof (NLHEADER));

           if ((nlname = (char *) malloc(nlheader.name_size)) == NULL ||
               fseek(idxfp,nlheader.name_ofs,SEEK_SET) ||
               fread(nlname,nlheader.name_size,1,idxfp) != 1 ||
               (nlfile = (NLFILE *) malloc(nlheader.files * sizeof (NLFILE))) == NULL ||
               fseek(idxfp,nlheader.file_ofs,SEEK_SET) ||
               fread(nlfile,sizeof (NLFILE) * nlheader.files,1,idxfp) != 1) {
              fclose(idxfp);
              idxfp = NULL;
              domp->errors = true;
           }   
           else
              domp->errors = false;
        }

        return (domp->errors ? false : true);
}


/* ------------------------------------------------------------------------- */
char *get_adrinfo (char *address)
{
        char domname[128];
        int zone, net, node, point;
        int zi, ni, si, bi, pi;
        DOMAIN *domp;
        char buffer[MAX_BUFFER + 1];
        long oldest_stamp;
        char *p;
        int i, n;

        if (!adrinfo) {
           if ((adrinfo = (ADRINFO *) malloc(NUM_ADRINFO * sizeof (ADRINFO))) == NULL)
              return (NULL);
           memset(adrinfo,0,NUM_ADRINFO * sizeof (ADRINFO));
        }

        if (idxbase) {
           struct stat f;

           for (domp = domain; domp; domp = domp->next)
               if (domp->idxbase && !stricmp(idxbase,domp->idxbase)) break;

           sprintf(buffer,"%s.FLG",domp->idxbase);
           if (!getstat(buffer,&f) && f.st_mtime != domp->flagstamp) {
              domp->timestamp = 0L;
              domp->flagstamp = f.st_mtime;
              findidx(domp);
           }
        }

        if (!address || !parseaddress(address,&zone,&net,&node,&point,domname))
           return (NULL);

        for (domp = domain; domp; domp = domp->next)
            if (!stricmp(domname,domp->abbrev)) break;
        if (!domp || !domp->idxbase)
           return (NULL);

        if (domp->idxbase != idxbase) {
           if (!findidx(domp))
              return (NULL);
        }
        else if (domp->errors)
           return (NULL);

        for (i = 0; i < NUM_ADRINFO; i++) {
            if (adrinfo[i].used && !stricmp(address,adrinfo[i].address)) {
               adrinfo[i].used = time(NULL);
               return (adrinfo[i].info);
            }
        }

        zi = ni = si = bi = pi = 0;
        *buffer = '\0';

        if (point && nlheader.bosses) {
           if (!nlboss &&
               ((nlboss = (NLBOSS *) malloc(nlheader.bosses * sizeof (NLBOSS))) == NULL ||
                fseek(idxfp,nlheader.boss_ofs,SEEK_SET) ||
                fread(nlboss,sizeof (NLBOSS) * nlheader.bosses,1,idxfp) != 1))
              goto err;

           for (bi = 0; bi < nlheader.bosses; bi++) {
               if (nlboss[bi].zone > zone) break;
               if (nlboss[bi].zone < zone) continue;
               if (nlboss[bi].net > net) break;
               if (nlboss[bi].net < net) continue;
               if (nlboss[bi].node > node) break;
               if (nlboss[bi].node < node) continue;

               if (!nlpoint || bi != last_bi) {
                  if (nlpoint) free(nlpoint);
                  last_bi = bi;
                  if ((nlpoint = (NLPOINT *) malloc(nlboss[bi].points * sizeof (NLPOINT))) == NULL ||
                      fseek(idxfp,nlboss[bi].point_ofs,SEEK_SET) ||
                      fread(nlpoint,sizeof (NLPOINT) * nlboss[bi].points,1,idxfp) != 1)
                     goto err;
               }

               for (pi = 0; pi < nlboss[bi].points; pi++) {
                   if (nlpoint[pi].point == point) {
                      if (!getline(nlpoint[pi].entry)) break;
                      sprintf(buffer,"%s (Point), %s",
                              nlentry.name, nlentry.location);
                      goto oki;
                   }
               }

               if (!getline(nlboss[bi].entry)) return (NULL);
               sprintf(buffer,"Unlisted point of %s, %s",
                       nlentry.name, nlentry.location);
               goto oki;
           }
        }

        if (!nlzone && nlheader.zones) {
           if ((nlzone = (NLZONE *) malloc(nlheader.zones * sizeof (NLZONE))) == NULL ||
               fseek(idxfp,nlheader.zone_ofs,SEEK_SET) ||
               fread(nlzone,sizeof (NLZONE) * nlheader.zones,1,idxfp) != 1)
              goto err;
        }

        for (zi = 0; zi < nlheader.zones; zi++) {
            if (nlzone[zi].zone > zone) break;
            if (nlzone[zi].zone < zone) continue;

            if (!nlnet || zi != last_zi) {
               if (nlnet) free(nlnet);
               if (nlnode) {
                  free(nlnode);
                  nlnode = NULL;
               }
               last_zi = zi;
               if ((nlnet = (NLNET *) malloc(nlzone[zi].nets * sizeof (NLNET))) == NULL ||
                   fseek(idxfp,nlzone[zi].net_ofs,SEEK_SET)||
                   fread(nlnet,sizeof (NLNET) * nlzone[zi].nets,1,idxfp) != 1)
                  goto err;
            }

            for (ni = 0; ni < nlzone[zi].nets; ni++) {
                if (nlnet[ni].net != net) continue;

                if (!nlnode || ni != last_ni) {
                   if (nlnode) free(nlnode);
                   last_ni = ni;
                   if ((nlnode = (NLNODE *) malloc(nlnet[ni].nodes * sizeof (NLNODE))) == NULL ||
                    fseek(idxfp,nlnet[ni].node_ofs,SEEK_SET) ||
                    fread(nlnode,sizeof (NLNODE) * nlnet[ni].nodes,1,idxfp) != 1)
                   goto err;
                }

                for (si = 0; si < nlnet[ni].nodes; si++) {
                    if (nlnode[si].node != node) continue;

                    if (!getline(nlnode[si].entry)) goto err;
                    switch (NL_TYPEMASK(nlnode[si].flags)) {
                           case NL_ZC:   p = " (ZC)";   break;
                           case NL_RC:   p = " (RC)";   break;
                           case NL_NC:   p = " (Host)"; break;
                           case NL_HUB:  p = " (Hub)";  break;
                           case NL_DOWN: p = " (Down)"; break;
                           case NL_HOLD: p = " (Hold)"; break;
                           case NL_PVT:  p = " (Pvt)";  break;
                           default:      p = strstr(nlentry.flags,"CM") ? " (CM)" : "";
                                         break;
                    }
                    sprintf(buffer,"%s%s%s, %s",
                            point ? "Point of " : "",
                            nlentry.name, p, nlentry.location);

                    goto oki;
                }

                if (!getline(nlnet[ni].entry)) goto err;
                sprintf(buffer,"%snlisted system in net %s",
                        point ? "Point of u" : "U",
                        nlentry.name);
                goto oki;
            }

            if (!getline(nlzone[zi].entry)) goto err;
            sprintf(buffer,"%system in unlisted net, %s",
                    point ? "Point of s" : "S",
                    nlentry.name);
            goto oki;
        }
        sprintf(buffer,"%system in unlisted zone",
                point ? "Point of s" : "S");

oki:    oldest_stamp = 0L;
        for (i = n = 0; i < NUM_ADRINFO; i++) {
            if (!adrinfo[i].used) {
               n = i;
               break;
            }
            if (!oldest_stamp || adrinfo[i].used < oldest_stamp) {
               oldest_stamp = adrinfo[i].used;
               n = i;
            }
        }
        if (adrinfo[n].address) free(adrinfo[n].address);
        if (adrinfo[n].info)    free(adrinfo[n].info);

        adrinfo[n].address = strdup(address);
        adrinfo[n].info    = strdup(buffer);
        adrinfo[n].used    = time(NULL);

        return (adrinfo[n].info);

err:    if (idxfp) {
           fclose(idxfp);
           idxfp = NULL;
        }
        domp->errors = true;

        return (NULL);
}/*get_adrinfo()*/


/* end of nodelist.c */
