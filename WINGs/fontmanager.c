
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
WMConvertFontToPlain(WMScreen *scr, WMFont *font) 
{
    if(!scr || !font)
        return font;

    return font;
}

WMFont *
WMConvertFontToBold(WMScreen *scr, WMFont *font) 
{
	WMFont *newfont=NULL;
	char fname[256];

	if(!scr || !font)
        return font;

	snprintf(fname, 255, font->name);
	changeFontProp(fname, "bold", 2);
	newfont = WMCreateNormalFont(scr, fname);

	if(!newfont)
		newfont = font;

	return newfont;
}

WMFont *
WMConvertFontToItalic(WMScreen *scr, WMFont *font) 
{
	WMFont *newfont=NULL;
	char fname[256];

	if(!scr || !font)
        return font;

	snprintf(fname, 255, font->name);
	changeFontProp(fname, "o", 3);
	newfont = WMCreateNormalFont(scr, fname);

	if(!newfont)
		newfont = font;

	return newfont;
}

WMFont *
WMGetFontOfSize(WMScreen *scr, WMFont *font, int size) 
{
    if(!scr || !font || size<1)
        return font;

    return font;
}


