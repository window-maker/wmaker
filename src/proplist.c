/* proplist.c- Hand made proplist parser.
 * 	The one in libPropList causes wmaker to crash if an error is found in
 * the parsed file. This parser is also more rigid: it will not accept any
 * property lists with errors, but will print more descriptive error messages
 * and will hopefully not crash.
 *
 *  Window Maker window manager
 * 
 *  Copyright (c) 1998 Alfredo K. Kojima
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 *  USA.
 */

#include "wconfig.h"
#include "WindowMaker.h"

#include <proplist.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


#if 0
#define DPUT(s) 	puts(s)
#else
#define DPUT(s)
#endif


#define INITIAL_BUFFER_SIZE	(16*1024)

#define BUFFER_SIZE_INCREMENT	1024


static int line_number = 1;
static int buffer_size = 0;
static char *buffer = NULL;
static char *file_name;


static proplist_t get_object(FILE *f);
static proplist_t get_array(FILE *f);
static proplist_t get_string(FILE *f);
static proplist_t get_qstring(FILE *f);
static proplist_t get_dictionary(FILE *f);


static INLINE int
get_char(FILE *f)
{
    int c;
    
    c = fgetc(f);
    if (c=='\n')
	line_number++;
    return c;
}



static INLINE int
get_non_space_char(FILE *f)
{
    int c;
    
    while (1) {
	c = fgetc(f);
	if (c=='\n')
	    line_number++;
	else if (!isspace(c))
	    break;
    }

    if (c!=EOF) {
	return c;
    } else {
	return EOF;
    }
}


static char *
unescapestr(char *src)
{
  char *dest = wmalloc(strlen(src)+1);
  char *src_ptr, *dest_ptr;
  char ch;


  for (src_ptr=src, dest_ptr=dest; *src_ptr;  src_ptr++, dest_ptr++)
    {
      if(*src_ptr != '\\')
        *dest_ptr = *src_ptr;
      else
        {
          ch = *(++src_ptr);
          if((ch>='0') && (ch<='3')) /* assume next 2 chars are octal too */
            {
              *dest_ptr = ((ch & 07) << 6);
              *dest_ptr |= ((*(++src_ptr)&07)<<3);
              *dest_ptr |= *(++src_ptr)&07;
            }
          else
            {
              switch(ch)
                {
                case 'a' : *dest_ptr = '\a'; break;
                case 'b' : *dest_ptr = '\b'; break;
                case 't' : *dest_ptr = '\t'; break;
                case 'r' : *dest_ptr = '\r'; break;
                case 'n' : *dest_ptr = '\n'; break;
                case 'v' : *dest_ptr = '\v'; break;
                case 'f' : *dest_ptr = '\f'; break;
                default  : *dest_ptr = *src_ptr;
                }
            }
        }
    }
  *dest_ptr = 0;

  return dest;
}


#define CHECK_BUFFER_SIZE(ptr) \
	    if ((ptr) >= buffer_size-1) {\
		buffer_size += BUFFER_SIZE_INCREMENT;\
		buffer = wrealloc(buffer, buffer_size);\
	     }


#define ISSTRINGABLE(c) (isalnum(c) || (c)=='.' || (c)=='_' || (c)=='/' \
		|| (c)=='+')



#define COMPLAIN(msg)  wwarning(_("syntax error in %s, line %i:%s"), \
				file_name, line_number, msg)


static proplist_t
get_qstring(FILE *f)
{
    int c;
    int ptr = 0;
    int escaping = 0;
    int ok = 1;

    while (1) {
	c = get_char(f);
	if (!escaping) {
	    if (c=='\\') {
		escaping = 1;
		continue;
	    }
	    if (c=='"')
		break;
	} else {
	    CHECK_BUFFER_SIZE(ptr);
	    buffer[ptr++] = '\\';	    
	    escaping = 0;
	}
	if (c==EOF) {
	    ptr--;
	    ok = 0;
	    COMPLAIN(_("unterminated string"));
	    break;
	} else {
	    CHECK_BUFFER_SIZE(ptr);
	    buffer[ptr++] = c;
	}
    }
    
    buffer[ptr] = 0;

    if (!ok)
	return NULL;
    else {
	char *tmp = unescapestr(buffer);
	proplist_t pl = PLMakeString(tmp);
	wfree(tmp);
	return pl;
    }
}



static proplist_t
get_string(FILE *f)
{
    int c;
    int ptr = 0;
    
    while (1) {
	c = get_char(f);
	if (ISSTRINGABLE(c)) {
	    CHECK_BUFFER_SIZE(ptr);
	    buffer[ptr++] = c;
	} else {
	    if (c!=EOF) {
		ungetc(c, f);
	    }
	    break;
	}
    }
    buffer[ptr] = 0;

    if (ptr==0)
	return NULL;
    else {
	char *tmp = unescapestr(buffer);
	proplist_t pl = PLMakeString(tmp);
	wfree(tmp);
	return pl;
    }
}




static proplist_t
get_array(FILE *f)
{
    int c;
    int ok=1, first=1;
    proplist_t list, obj;
    
    list = PLMakeArrayFromElements(NULL);
    
    while (1) {
	c = get_non_space_char(f);
	if (c==EOF) {
	    COMPLAIN(_("unterminated array"));
	    ok = 0;
	    break;
	} else if (c==')') {
	    break;
	} else if (c==',') {
	    /* continue normally */
	} else {
	    if (!first) {
		COMPLAIN(_("missing , in array or unterminated array"));
		ok = 0;
		break;
	    } else {
		ungetc(c, f);
	    }
	}
	first = 0;
	/* get the data */
	obj = get_object(f);
	if (!obj) {
	    COMPLAIN(_("could not get array element"));
	    ok = 0;
	    break;
	}
	list = PLAppendArrayElement(list, obj);
	PLRelease(obj);
    }
    
    if (ok)
	return list;
    else {
	PLRelease(list);
	return NULL;
    }
}


static proplist_t
get_dictionary(FILE *f)
{
    int c;
    int ok = 1;
    proplist_t dict, key, value;
    
    dict = PLMakeDictionaryFromEntries(NULL, NULL);

    while (1) {
	c = get_non_space_char(f);
	
	if (c==EOF) {
	    COMPLAIN(_("unterminated dictionary"));
	    ok = 0;
	    break;
	} else if (c=='}') {
	    break;
	}
	
	/* get the entry */
	
	/* get key */
	DPUT("getting dict key");
	if (c=='"')
	    key = get_qstring(f);
	else if (ISSTRINGABLE(c)) {
	    ungetc(c, f);
	    key = get_string(f);
	} else {
	    if (c=='=')
		COMPLAIN(_("missing dictionary key"));
	    else
		COMPLAIN(_("missing dictionary entry key or unterminated dictionary"));
	    ok = 0;
	    break;
	}
	
	if (!key) {
	    COMPLAIN(_("error parsing dictionary key"));
	    ok = 0;
	    break;
	}
	DPUT("getting =");
	/* get = */
	c = get_non_space_char(f);
	if (c!='=') {
	    PLRelease(key);
	    COMPLAIN(_("missing = in dictionary entry"));
	    ok = 0;
	    break;
	}
	DPUT("getting dict entry data");
	/* get data */
	value = get_object(f);
	if (!value) {
	    /*
	    COMPLAIN(_("error parsing dictionary entry value"));
	     */
	    ok = 0;
	    PLRelease(key);
	    break;
	}
	DPUT("getting ;");
	/* get ; */
	c = get_non_space_char(f);
	if (c!=';') {
	    COMPLAIN(_("missing ; in dictionary entry"));
	    ok = 0;
	    PLRelease(key);
	    PLRelease(value);
	    break;
	}
	dict = PLInsertDictionaryEntry(dict, key, value);
	PLRelease(key);
	PLRelease(value);
    }

    if (!ok) {
	PLRelease(dict);
	return NULL;
    } else {
	return dict;
    }
}


static proplist_t
get_data(FILE *f)
{

    
    COMPLAIN("the data datatype is not yet implemented");
    
    return NULL;
}





static proplist_t
get_object(FILE *f)
{
    int c;
    proplist_t pl;
    
    c = get_non_space_char(f);
    
    switch (c) {
	/* END OF FILE */
     case EOF:
	DPUT("EOF");
	pl = NULL;
	break;
	
	/* dictionary */
     case '{':
	DPUT("getting dictionary");
	pl = get_dictionary(f);
	break;
	
	/* array */
     case '(':
	DPUT("getting arrray");
	pl = get_array(f);
	break;
	
	/* data */
     case '<':
	DPUT("getting data");
	pl = get_data(f);
	break;

	/* quoted string */
     case '"':
	DPUT("getting qstring");
	pl = get_qstring(f);
	break;

	/* string */
     default:
	if (ISSTRINGABLE(c)) {
	    DPUT("getting string");
	    /* put back */
	    ungetc(c, f);
	    pl = get_string(f);
	} else {
	    COMPLAIN(_("was expecting a string, dictionary, data or array. If it's a string, try enclosing it with \"."));
	    if (c=='#' || c=='/') {
		wwarning(_("Comments are not allowed inside WindowMaker owned domain files."));
	    }
	    pl = NULL;
	}
	break;
    }

    return pl;
}


proplist_t 
ReadProplistFromFile(char *file)
{
    FILE *f;
    proplist_t pl = NULL;
    
    f = fopen(file, "r");
    if (!f) {
	wsyserror(_("could not open domain file %s"), file);
	return NULL;
    }

    file_name = file;
    line_number = 1;
    buffer_size = INITIAL_BUFFER_SIZE;
    buffer = wmalloc(buffer_size);

    pl = get_object(f);

    /* check for illegal characters after EOF */
    if (get_non_space_char(f)!=EOF && pl) {
	COMPLAIN(_("extra data after end of file"));
	/* 
	 * We can't just ignore garbage after the file because the "garbage"
	 * could be the data and the real garbage be in the beginning of
	 * the file (wich is now, inside pl)
	 */
	PLRelease(pl);
	pl = NULL;
    }

    wfree(buffer);

    fclose(f);

    if (pl) {
	proplist_t fpl;

	fpl = PLMakeString(file);
	PLSetFilename(pl, fpl);
	PLRelease(fpl);
    }
    return pl;
}



