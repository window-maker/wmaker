
#include "wconfig.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_BSD_STRING_H
#include <bsd/string.h>
#endif

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
	{{3, 1}, {0, 0}, {4, 0}, {1, 0}, {8, 0}, {6, 0}},
	{{1, 1}, {1, 1}, {2, 0}, {3, 0}, {5, 0}, {1, 1}},
	{{1, 1}, {1, 1}, {1, 1}, {1, 1}, {5, 0}, {1, 1}},
	{{3, 1}, {5, 0}, {4, 0}, {1, 0}, {5, 0}, {6, 0}},
	{{3, 1}, {3, 1}, {3, 1}, {3, 1}, {5, 0}, {3, 1}},
	{{-1, -1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},	/* final state */
	{{6, 1}, {6, 1}, {7, 0}, {6, 1}, {5, 0}, {3, 0}},
	{{6, 1}, {6, 1}, {6, 1}, {6, 1}, {5, 0}, {6, 1}},
	{{-1, -1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},	/* final state */
};

char *wtokennext(char *word, char **next)
{
	char *ptr;
	char *ret, *t;
	int state, ctype;

	t = ret = wmalloc(strlen(word) + 1);
	ptr = word;

	state = 0;
	while (1) {
		if (*ptr == 0)
			ctype = PRC_EOS;
		else if (*ptr == '\\')
			ctype = PRC_ESCAPE;
		else if (*ptr == '"')
			ctype = PRC_DQUOTE;
		else if (*ptr == '\'')
			ctype = PRC_SQUOTE;
		else if (*ptr == ' ' || *ptr == '\t')
			ctype = PRC_BLANK;
		else
			ctype = PRC_ALPHA;

		if (mtable[state][ctype].output) {
			*t = *ptr;
			t++;
			*t = 0;
		}
		state = mtable[state][ctype].nstate;
		ptr++;
		if (mtable[state][0].output < 0) {
			break;
		}
	}

	if (*ret == 0)
		t = NULL;
	else
		t = wstrdup(ret);

	wfree(ret);

	if (ctype == PRC_EOS)
		*next = NULL;
	else
		*next = ptr;

	return t;
}

/* separate a string in tokens, taking " and ' into account */
void wtokensplit(char *command, char ***argv, int *argc)
{
	char *token, *line;
	int count;

	count = 0;
	line = command;
	do {
		token = wtokennext(line, &line);
		if (token) {
			if (count == 0)
				*argv = wmalloc(sizeof(char **));
			else
				*argv = wrealloc(*argv, (count + 1) * sizeof(char **));
			(*argv)[count++] = token;
		}
	} while (token != NULL && line != NULL);

	*argc = count;
}

char *wtokenjoin(char **list, int count)
{
	int i, j;
	char *flat_string, *wspace;

	j = 0;
	for (i = 0; i < count; i++) {
		if (list[i] != NULL && list[i][0] != 0) {
			j += strlen(list[i]);
			if (strpbrk(list[i], " \t"))
				j += 2;
		}
	}

	flat_string = wmalloc(j + count + 1);

	for (i = 0; i < count; i++) {
		if (list[i] != NULL && list[i][0] != 0) {
			if (i > 0)
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

void wtokenfree(char **tokens, int count)
{
	while (count--)
		wfree(tokens[count]);
	wfree(tokens);
}

char *wtrimspace(const char *s)
{
	char *t;

	if (s == NULL)
		return NULL;

	while (isspace(*s) && *s)
		s++;
	t = (char *)s + strlen(s) - 1;
	while (t > s && isspace(*t))
		t--;

	return wstrndup(s, t - s + 1);
}

char *wstrdup(const char *str)
{
	assert(str != NULL);

	return strcpy(wmalloc(strlen(str) + 1), str);
}

char *wstrndup(const char *str, size_t len)
{
	char *copy;

	assert(str != NULL);

	len = WMIN(len, strlen(str));
	copy = strncpy(wmalloc(len + 1), str, len);
	copy[len] = 0;

	return copy;
}

char *wstrconcat(char *str1, char *str2)
{
	char *str;

	if (!str1)
		return wstrdup(str2);
	else if (!str2)
		return wstrdup(str1);

	str = wmalloc(strlen(str1) + strlen(str2) + 1);
	strcpy(str, str1);
	strcat(str, str2);

	return str;
}

char *wstrappend(char *dst, char *src)
{
	if (!dst)
		return wstrdup(src);
	else if (!src || *src == 0)
		return dst;

	dst = wrealloc(dst, strlen(dst) + strlen(src) + 1);
	strcat(dst, src);

	return dst;
}


#ifdef HAVE_STRLCAT
size_t
wstrlcat(char *dst, const char *src, size_t siz)
{
	return strlcat(dst, src, siz);
}
#else
/*	$OpenBSD: strlcat.c,v 1.13 2005/08/08 08:05:37 espie Exp $	*/

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
wstrlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}
#endif /* HAVE_STRLCAT */

#ifdef HAVE_STRLCPY
size_t
wstrlcpy(char *dst, const char *src, size_t siz)
{
	return strlcpy(dst, src, siz);
}
#else

/*	$OpenBSD: strlcpy.c,v 1.11 2006/05/05 15:27:38 millert Exp $	*/

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
wstrlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}
#endif /* HAVE_STRLCPY */
