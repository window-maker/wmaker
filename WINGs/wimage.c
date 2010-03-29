#include <WINGsP.h>

WMImage *WMRetainImage(WMImage *image)
{
    return cairo_surface_reference(image);
}

void WMDestroyImage(WMImage *image)
{
    cairo_surface_destroy(image);
}


unsigned int WMGetImageWidth(WMImage *image)
{
    return cairo_image_surface_get_width(image);
}

unsigned int WMGetImageHeight(WMImage *image)
{
    return cairo_image_surface_get_height(image);
}
