
#include "WINGsP.h"


static void
changeFontProp(char *fname, char *newprop, int which)
{
	char before[128], prop[128], after[128];
	char *ptr, *bptr;
	int part=0;
	
	if(!fname || !prop)
		return;

	ptr = fname;
	bptr = before;
	while (*ptr) {
		if(*ptr == '-') {
    	    *bptr = 0;
			if(part==which) bptr = prop;
			else if(part==which+1) bptr = after;
    	  	*bptr++ = *ptr;
			part++;
		} else {
    	  *bptr++ = *ptr;
		}  ptr++;
	}*bptr = 0;
	snprintf(fname, 255, "%s-%s%s", before, newprop, after); 
}


WMFont *
WMGetFontPlain(WMScreen *scrPtr, WMFont *font) 
{
    if(!scrPtr || !font)
        return NULL;
    return font;
}

WMFont *
WMGetFontBold(WMScreen *scrPtr, WMFont *font) 
{
	WMFont *newfont=NULL;
	char fname[256];
	if(!scrPtr || !font)
		return NULL;
	snprintf(fname, 255, font->name);
	changeFontProp(fname, "bold", 2);
	newfont = WMCreateNormalFont(scrPtr, fname);
	if(!newfont)
		newfont = font;
	return newfont;
}

WMFont *
WMGetFontItalic(WMScreen *scrPtr, WMFont *font) 
{
	WMFont *newfont=NULL;
	char fname[256];
	if(!scrPtr || !font)
		return NULL;
	snprintf(fname, 255, font->name);
	changeFontProp(fname, "o", 3);
	newfont = WMCreateNormalFont(scrPtr, fname);
	if(!newfont)
		newfont = font;
	return newfont;
}

WMFont *
WMGetFontOfSize(WMScreen *scrPtr, WMFont *font, int size) 
{
    if(!scrPtr || !font || size<1)
        return NULL;
    return font;
}


