
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "WUtil.h"



#define PRC_ALPHA	0
#define PRC_BLANK	1
#define PRC_ESCAPE	2
#define PRC_DQUOTE	3
#define PRC_EOS		4
#define PRC_SQUOTE	5

typedef struct {
    short nstate;
    short output;
} DFA;


static DFA mtable[9][6] = {
    {{3,1},{0,0},{4,0},{1,0},{8,0},{6,0}},
    {{1,1},{1,1},{2,0},{3,0},{5,0},{1,1}},
    {{1,1},{1,1},{1,1},{1,1},{5,0},{1,1}},
    {{3,1},{5,0},{4,0},{1,0},{5,0},{6,0}},
    {{3,1},{3,1},{3,1},{3,1},{5,0},{3,1}},
    {{-1,-1},{0,0},{0,0},{0,0},{0,0},{0,0}}, /* final state */
    {{6,1},{6,1},{7,0},{6,1},{5,0},{3,0}},
    {{6,1},{6,1},{6,1},{6,1},{5,0},{6,1}},
    {{-1,-1},{0,0},{0,0},{0,0},{0,0},{0,0}}, /* final state */
};

char*
wtokennext(char *word, char **next)
{
    char *ptr;
    char *ret, *t;
    int state, ctype;

    t = ret = wmalloc(strlen(word)+1);
    ptr = word;
    
    state = 0;
    *t = 0;
    while (1) {
	if (*ptr==0) 
	    ctype = PRC_EOS;
	else if (*ptr=='\\')
	    ctype = PRC_ESCAPE;
	else if (*ptr=='"')
	    ctype = PRC_DQUOTE;
	else if (*ptr=='\'')
	    ctype = PRC_SQUOTE;
	else if (*ptr==' ' || *ptr=='\t')
	    ctype = PRC_BLANK;
	else
	    ctype = PRC_ALPHA;

	if (mtable[state][ctype].output) {
	    *t = *ptr; t++;
	    *t = 0;
	}
	state = mtable[state][ctype].nstate;
	ptr++;
	if (mtable[state][0].output<0) {
	    break;
	}
    }

    if (*ret==0)
	t = NULL;
    else
	t = wstrdup(ret);

    free(ret);
    
    if (ctype==PRC_EOS)
	*next = NULL;
    else
	*next = ptr;
    
    return t;
}


/* separate a string in tokens, taking " and ' into account */
void
wtokensplit(char *command, char ***argv, int *argc)
{
    char *token, *line;
    int count;

    count = 0;
    line = command;
    do {
	token = wtokennext(line, &line);
	if (token) {
	    if (count == 0)
		*argv = wmalloc(sizeof(char**));
	    else
		*argv = wrealloc(*argv, (count+1)*sizeof(char**));
	    (*argv)[count++] = token;
	}
    } while (token!=NULL && line!=NULL);

    *argc = count;
}





char*
wtokenjoin(char **list, int count)
{
    int i, j;
    char *flat_string, *wspace;

    j = 0;
    for (i=0; i<count; i++) {
        if (list[i]!=NULL && list[i][0]!=0) {
            j += strlen(list[i]);
            if (strpbrk(list[i], " \t"))
                j += 2;
        }
    }
    
    flat_string = wmalloc(j+count+1);

    *flat_string = 0;
    for (i=0; i<count; i++) {
	if (list[i]!=NULL && list[i][0]!=0) {
	    if (i>0)
		strcat(flat_string, " ");
            wspace = strpbrk(list[i], " \t");
            if (wspace)
                strcat(flat_string, "\"");
	    strcat(flat_string, list[i]);
            if (wspace)
                strcat(flat_string, "\"");
	}
    }

    return flat_string;
}



void
wtokenfree(char **tokens, int count)
{
    while (--count) free(tokens[count]);
    free(tokens);
}



char*
wtrimspace(char *s)
{
    char *t;
    char *c;
    
    while (isspace(*s) && *s) s++;
    t = s+strlen(s);
    while (t > s && isspace(*t)) t--;
    
    c = wmalloc(t-s + 1);
    memcpy(c, s, t-s);
    c[t-s] = 0;
    
    return c;
}



