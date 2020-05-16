/* Pattern search (allows ? and * wildcard ANYWHERE in the search string) */
/* Use patinit() first to 'clean up' a search pattern                     */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "2types.h"


void patinit (register char *pat)           /* Fix to valid *pattern* string */
{
        register int i;
        register char *p;

        for (i = 0; pat[i] && strchr(" ?*",pat[i]); i++);
        if (i > 0) strcpy(pat,&pat[i]);
        for (i = (int) strlen(pat) - 1; i > 0 && strchr(" ?*",pat[i]); pat[i--] = '\0');
        do {              /* Replace any  **  ?*  *?  by '*' in user pattern */
           if      ((p = strstr(pat,"**")) != NULL) strcpy(p,p + 1);
           else if ((p = strstr(pat,"?*")) != NULL) strcpy(p,p + 1);
           else if ((p = strstr(pat,"*?")) != NULL) strcpy(p + 1,p + 2);
        } while (p);
        strcat(pat,"*");                             /* End pattern with '*' */
        if (*pat != '*') {                         /* Start pattern with '*' */
           memmove(pat + 1,pat,strlen(pat) + 1);
           *pat = '*';
        }
        strlwr(pat);
}/*patinit()*/


boolean patimat (char *raw, char *pat)       /* Pattern matching - recursive */
{                                     /* **** Case INSENSITIVE version! **** */
        if (!*pat)
           return (*raw ? false : true);

        if (*pat == '*') {
           if (!*++pat)
              return (true);
           do if ((tolower(*raw) == *pat || *pat == '?') &&
                  patimat(raw + 1,pat + 1))
                 return (true);
           while (*raw++);
        }
        else if (*raw && (*pat == '?' || *pat == tolower(*raw)))
           return (patimat(++raw,++pat));

        return (false);
}/*patimat()*/


/* end of pattern.c */
