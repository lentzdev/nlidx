/* ======================================================== Rev: 10 Sep 1993 */
/* Indexing system on raw ASCII nodelist (St.Louis/FTS-0005 format)          */
/* Design & COPYRIGHT (C) 1993-1995 by Arjen G. Lentz; ALL RIGHTS RESERVED   */
/* A project of LENTZ SOFTWARE-DEVELOPMENT, Amersfoort, The Netherlands (EU) */
/* ------------------------------------------------------------------------- */
/* LICENSE: Permission is hereby granted to implement the nodeidx format in   /
/* your own applications (e.g. nodelist compilers, msg-editors, mailers,      /
/* outbound managers, BBS software, etc), on the condition that no            /
/* alterations (be it changes or additions) made to this format. The format   /
/* allows for backward-compatible exptensions; of course this is only         /
/* possible if no conflicting extentions are introduced. For this purpose,    /
/* any alteration of this format requires *explicit* prior approval from me.  /
/* Arjen Lentz @ FidoNet 2:283/512, 2:283/550, 2:283/551, agl@bitbike.iaf.nl  /
/* ========================================================================= */
/* Implementation info: This format was specifically designed to operate     */
/* in multi-tasking and multi-user environments, without the need for an     */
/* application to temporarily close an indexes/nodelist while a nodelist     */
/* compiler is busy.                                                         */
/* All files are to be opened in shared deny-none mode, even a nodelist      */
/* diffing utility should adhere to this principle!                          */
/* The index uses the raw nodelist(s), so those should remain available.     */
/* Each domain gets its own index, which may consist of multiple raw files.  */
/* When creating new nodelists/indexes, utilities should use temporary       */
/* filenames to ensure other programs cannot be confused by this activity.   */
/* When finished, a new index receives a new rotating sequence number, and   */
/* a semaphore file <domain>.FLG is touched to indicate the change to any    */
/* applications. So applications can automatically switch to new indexes.    */
/* Old indexes and raw nodelist files may be removed only if they are no     */
/* long in use by any application. This is checked by scanning all indexes,  */
/* removing the old ones if possible (the file system will not allow you to  */
/* delete a file if it is opened by another process), then deleting raw      */
/* nodelist files only if they are no listed in any of the still new/active  */
/* nodelist indexes. This requires some logic: nodeidx.c provides a sample.  */
/* ========================================================================= */

#include "2types.h"


#define NL_IDSTRING                "NLidx"
#define NL_MAJOR                   (1)
#define NL_MINOR                   (0)
#define NL_REVISION                ((NL_MAJOR << 8) | NL_MINOR)
#define NL_REVMAJOR(revision)      (((revision) >> 8) & 0x00ff)
#define NL_REVMINOR(revision)      ((revision) & 0x00ff)

#define NL_MAXFILES                (256)
#define NL_ENTFILE(fileofs)        (((int) ((fileofs) >> 24)) & 0x00ff)
#define NL_ENTOFS(fileofs)         ((fileofs) & 0x00ffffffL)
#define NL_ENTRY(fileidx,fileofs)  ((((FILE_OFS) (fileidx)) << 24) | (fileofs))

enum { NL_NORM, NL_PVT, NL_HOLD, NL_DOWN, NL_HUB, NL_NC, NL_RC, NL_ZC };
#define NL_TYPEMASK(flags)         ((flags) & 0x07)
#define NL_HASPOINTS               (0x8000)             /* System has points */

#define NL_FPERIODIC               (0x0001)             /* Periodic raw file */


/* HEADER NODE ... NET ... [POINT ... BOSS ...] NAME ... FILE ... ZONE ... */
typedef struct _nlheader NLHEADER;
typedef struct _nlfile   NLFILE;
typedef struct _nlzone   NLZONE;
typedef struct _nlnet    NLNET;
typedef struct _nlnode   NLNODE;
typedef struct _nlboss   NLBOSS;
typedef struct _nlpoint  NLPOINT;


struct _nlheader {              /* ------- Header record at start of index   */
        char     idstring[5];           /* Nodelist index ID string          */
        char     creator[40];           /* Name/ver index creating program   */
        word     revision;              /* Index format revision major/minor */
        long     timestamp;             /* UNIX timestamp of this index file */
        word     files;                 /* Number of file records            */
        FILE_OFS file_ofs;              /* Offset of file records in index   */
        word     name_size;             /* Total size of name string block   */
        FILE_OFS name_ofs;              /* Offset of name strings in index   */
        word     zones;                 /* Number of zone records            */
        FILE_OFS zone_ofs;              /* Offset of zone records in index   */
        word     bosses;                /* Number of boss records            */
        FILE_OFS boss_ofs;              /* Offset of boss records in index   */
};

struct _nlfile {                /* ------- File record --------------------- */
        long    timestamp;              /* UNIX timestamp of this raw file   */
        word    file_flags;             /* Information flags                 */
        word    name_ofs;               /* Ofs in name string block to fname */
};

struct _nlzone {                /* ------- Zone record --------------------- */
        word     zone;                  /* Zone number                       */
        word     flags;                 /* Information flags                 */
        FILE_OFS entry;                 /* File number and offset to entry   */
        word     nets;                  /* Number of nets in this zone       */
        FILE_OFS net_ofs;               /* Offset of net records in index    */
};

struct _nlnet {                 /* ------- Net record ---------------------- */
        word     net;                   /* Net number                        */
        word     flags;                 /* Information flags                 */
        FILE_OFS entry;                 /* File number and offset to entry   */
        word     nodes;                 /* Number of nodes in this net       */
        FILE_OFS node_ofs;              /* Offset of node records in index   */
};

struct _nlnode {                /* ------- Node record --------------------- */
        word     node;                  /* Node number                       */
        word     flags;                 /* Information flags                 */
        FILE_OFS entry;                 /* File number and offset to entry   */
};

struct _nlboss {                /* ------- Boss record --------------------- */
        word     zone, net, node;       /* Node number                       */
        word     flags;                 /* Information flags                 */
        FILE_OFS entry;                 /* File number and offset to entry   */
        word     points;                /* Number of points under this node  */
        FILE_OFS point_ofs;             /* Offset of point records in index  */
};

struct _nlpoint {               /* ------- Point record -------------------- */
        word     point;                 /* Point number                      */
        word     flags;                 /* Information flags                 */
        FILE_OFS entry;                 /* File number and offset to entry   */
};


/* end of nodeidx.h */
