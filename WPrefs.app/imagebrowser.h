

typedef struct _ImageBrowser ImageBrowser;
typedef struct _ImageBrowserDelegate ImageBrowserDelegate;





struct _ImageBrowserDelegate {
    void *data;

    void (*selected)(ImageBrowserDelegate *self,
                     ImageBrowser *browser,
                     char *path);

    void (*closed)(ImageBrowserDelegate *self,
                   ImageBrowser *browser,
                   Bool canceled);

};


ImageBrowser *CreateImageBrowser(WMScreen *scr, char *title, char **paths,
                                 int pathN, WMSize *maxSize,
                                 WMWidget *auxWidget);


