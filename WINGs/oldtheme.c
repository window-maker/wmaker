#include <wtheme.h>

void W_DrawButtonDarkBack(cairo_t *cairo, WMButton *bPtr, unsigned int width, unsigned int height, WMReliefType relief)
{
	cairo_save(cairo);

	WMColorSpec fill = WMGrayColorSpec();

	cairo_set_source_rgba(cairo, fill.red/255.0, fill.green/255.0, fill.blue/255.0, fill.alpha/255.0);
	cairo_rectangle(cairo, 0, 0, width, height);
	cairo_fill(cairo);
	cairo_stroke(cairo);

	cairo_restore(cairo);
}

void W_DrawButtonLightBack(cairo_t *cairo, WMButton *bPtr, unsigned int width, unsigned int height, WMReliefType relief)
{
	cairo_save(cairo);

	WMColorSpec fill = WMWhiteColorSpec();

	cairo_set_source_rgba(cairo, fill.red/255.0, fill.green/255.0, fill.blue/255.0, fill.alpha/255.0);
	cairo_rectangle(cairo, 0, 0, width, height);
	cairo_fill(cairo);
	cairo_stroke(cairo);

	cairo_restore(cairo);
}

void W_DrawButtonRelief(cairo_t *cairo, unsigned int width, unsigned int height, WMReliefType relief)
{
	cairo_save(cairo);

	WMColorSpec outerlefttop;
	WMColorSpec innerlefttop;
	WMColorSpec outerbottomright;
	WMColorSpec innerbottomright;

	switch (relief) {
		case WRSimple:
		{
			outerlefttop = WMBlackColorSpec();
			outerbottomright = WMBlackColorSpec();
			innerlefttop = WMTransparentColorSpec();
			innerbottomright = WMTransparentColorSpec();
			break;
		}
		case WRRaised:
		{
			outerlefttop = WMWhiteColorSpec();
			outerbottomright = WMBlackColorSpec();
			innerlefttop = WMTransparentColorSpec();
			innerbottomright = WMDarkGrayColorSpec();
			break;
		}
		case WRSunken:
		{
			outerlefttop = WMDarkGrayColorSpec();
			outerbottomright = WMWhiteColorSpec();
			innerlefttop = WMBlackColorSpec();
			innerbottomright = WMTransparentColorSpec();
			break;
		}
		case WRPushed:
		{
			outerlefttop = WMBlackColorSpec();
			outerbottomright = WMWhiteColorSpec();
			innerlefttop = WMTransparentColorSpec();
			innerbottomright = WMTransparentColorSpec();
			break;
		}
		case WRRidge:
		{
			outerlefttop = WMWhiteColorSpec();
			outerbottomright = WMDarkGrayColorSpec();
			innerlefttop = WMTransparentColorSpec();
			innerbottomright = WMTransparentColorSpec();
			break;
		}
		case WRGroove:
		{
			outerlefttop = WMDarkGrayColorSpec();
			outerbottomright = WMDarkGrayColorSpec();
			innerlefttop = WMTransparentColorSpec();
			innerbottomright = WMTransparentColorSpec();
			break;
		}
	}

	cairo_set_line_width(cairo,1);

	WMColorSpecSet(cairo,&outerlefttop);
	cairo_rectangle(cairo,0,0,width-1,0);
	cairo_rectangle(cairo,0,0,0,height-1);
	cairo_stroke(cairo);

	WMColorSpecSet(cairo,&innerlefttop);
	cairo_rectangle(cairo,1,1,width-2,0);
	cairo_rectangle(cairo,1,1,0,height-2);
	cairo_stroke(cairo);

	WMColorSpecSet(cairo,&innerbottomright);
	cairo_rectangle(cairo,1,height-2,width-1,1);
	cairo_rectangle(cairo,width-2,1,1,height-2);
	cairo_stroke(cairo);

	WMColorSpecSet(cairo,&outerbottomright);
	cairo_rectangle(cairo,0,height-1,width,1);
	cairo_rectangle(cairo,width-1,0,1,height);
	cairo_stroke(cairo);

	cairo_restore(cairo);
}
