
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>


#include "wconfig.h"

#include "WINGs.h"



typedef struct W_UserDefaults {
    WMPropList *defaults;

    WMPropList *appDomain;

    WMPropList *searchListArray;
    WMPropList **searchList;	       /* cache for searchListArray */

    char dirty;

    char dontSync;

    char *path;                        /* where is db located */

    time_t timestamp;                  /* last modification time */

    struct W_UserDefaults *next;

} UserDefaults;


static UserDefaults *sharedUserDefaults = NULL;

char *WMUserDefaultsDidChangeNotification = "WMUserDefaultsDidChangeNotification";


static void synchronizeUserDefaults(void *foo);

extern char *WMGetApplicationName();

#define DEFAULTS_DIR "/Defaults"

#define UD_SYNC_INTERVAL  2000



char*
wusergnusteppath()
{
    static char *path = NULL;
    char *gspath;
    int pathlen;

    if (!path) {
	gspath = getenv("GNUSTEP_USER_ROOT");
	if (gspath) {
	    gspath = wexpandpath(gspath);
	    pathlen = strlen(gspath) + 4;
	    path = wmalloc(pathlen);
	    strcpy(path, gspath);
	    wfree(gspath);
	} else {
	    pathlen = strlen(wgethomedir()) + 10;
	    path = wmalloc(pathlen);
	    strcpy(path, wgethomedir());
	    strcat(path, "/GNUstep");
	}
    }

    return path;
}


char*
wdefaultspathfordomain(char *domain)
{
    char *path;
    char *gspath;

    gspath = wusergnusteppath();
    path = wmalloc(strlen(gspath)+strlen(DEFAULTS_DIR)+strlen(domain)+4);
    strcpy(path, gspath);
    strcat(path, DEFAULTS_DIR);
    strcat(path, "/");
    strcat(path, domain);

    return path;
}


static void
#ifdef HAVE_ATEXIT
saveDefaultsChanges(void)
#else
saveDefaultsChanges(int foo, void *bar)
#endif
{
    /* save the user defaults databases */
    synchronizeUserDefaults(NULL);
}


/* set to save changes in defaults when program is exited */
static void
registerSaveOnExit(void)
{
    static Bool registeredSaveOnExit = False;

    if (!registeredSaveOnExit) {
#ifdef HAVE_ATEXIT
        atexit(saveDefaultsChanges);
#else
        on_exit(saveDefaultsChanges, (void*)NULL);
#endif
        registeredSaveOnExit = True;
    }
}


static void
synchronizeUserDefaults(void *foo)
{
    UserDefaults *database = sharedUserDefaults;

    while (database) {
        if (!database->dontSync)
            WMSynchronizeUserDefaults(database);
        database = database->next;
    }
}


static void
addSynchronizeTimerHandler(void)
{
    static Bool initialized = False;

    if (!initialized) {
        WMAddPersistentTimerHandler(UD_SYNC_INTERVAL, synchronizeUserDefaults,
                                    NULL);
        initialized = True;
    }
}


void
WMEnableUDPeriodicSynchronization(WMUserDefaults *database, Bool enable)
{
    database->dontSync = !enable;
}


void
WMSynchronizeUserDefaults(WMUserDefaults *database)
{
    Bool fileIsNewer = False, release = False, notify = False;
    WMPropList *plF, *key;
    char *path;
    struct stat stbuf;

    if (!database->path) {
        path = wdefaultspathfordomain(WMGetApplicationName());
        release = True;
    } else {
        path = database->path;
    }

    if (stat(path, &stbuf) >= 0 && stbuf.st_mtime > database->timestamp)
        fileIsNewer = True;

    if (database->appDomain && (database->dirty || fileIsNewer)) {
        if (database->dirty && fileIsNewer) {
            plF = WMReadPropListFromFile(path);
            if (plF) {
                plF = WMMergePLDictionaries(plF, database->appDomain, False);
                WMReleasePropList(database->appDomain);
                database->appDomain = plF;
                key = database->searchList[0];
                WMPutInPLDictionary(database->defaults, key, plF);
                notify = True;
            } else {
                /* something happened with the file. just overwrite it */
                wwarning(_("cannot read domain from file '%s' when syncing"),
                         path);
                WMWritePropListToFile(database->appDomain, path, True);
            }
        } else if (database->dirty) {
            WMWritePropListToFile(database->appDomain, path, True);
        } else if (fileIsNewer) {
            plF = WMReadPropListFromFile(path);
            if (plF) {
                WMReleasePropList(database->appDomain);
                database->appDomain = plF;
                key = database->searchList[0];
                WMPutInPLDictionary(database->defaults, key, plF);
                notify = True;
            } else {
                /* something happened with the file. just overwrite it */
                wwarning(_("cannot read domain from file '%s' when syncing"),
                         path);
                WMWritePropListToFile(database->appDomain, path, True);
            }
        }

        database->dirty = 0;

        if (stat(path, &stbuf) >= 0)
            database->timestamp = stbuf.st_mtime;

        if (notify) {
            WMPostNotificationName(WMUserDefaultsDidChangeNotification,
                                   database, NULL);
        }
    }

    if (release)
        wfree(path);

}


void
WMSaveUserDefaults(WMUserDefaults *database)
{
    if (database->appDomain) {
        struct stat stbuf;
        char *path;
        Bool release = False;

        if (!database->path) {
            path = wdefaultspathfordomain(WMGetApplicationName());
            release = True;
        } else {
            path = database->path;
        }
        WMWritePropListToFile(database->appDomain, path, True);
        database->dirty = 0;
        if (stat(path, &stbuf) >= 0)
            database->timestamp = stbuf.st_mtime;
        if (release)
            wfree(path);
    }
}


WMUserDefaults*
WMGetStandardUserDefaults(void)
{
    WMUserDefaults *defaults;
    WMPropList *domain;
    WMPropList *key;
    struct stat stbuf;
    char *path;
    int i;

    if (sharedUserDefaults) {
        defaults = sharedUserDefaults;
        while (defaults) {
            /* path == NULL only for StandardUserDefaults db */
            if (defaults->path == NULL)
                return defaults;
            defaults = defaults->next;
        }
    }

    /* we didn't found the database we are looking for. Go read it. */
    defaults = wmalloc(sizeof(WMUserDefaults));
    memset(defaults, 0, sizeof(WMUserDefaults));

    defaults->defaults = WMCreatePLDictionary(NULL, NULL, NULL);

    defaults->searchList = wmalloc(sizeof(WMPropList*)*3);

    /* application domain */
    key = WMCreatePLString(WMGetApplicationName());
    defaults->searchList[0] = key;

    /* temporary kluge. wmaker handles synchronization itself */
    if (strcmp(WMGetApplicationName(), "WindowMaker")==0) {
        defaults->dontSync = 1;
    }

    path = wdefaultspathfordomain(WMGetFromPLString(key));

    if (stat(path, &stbuf) >= 0)
        defaults->timestamp = stbuf.st_mtime;

    domain = WMReadPropListFromFile(path);

    if (!domain)
        domain = WMCreatePLDictionary(NULL, NULL, NULL);

    if (path)
        wfree(path);

    defaults->appDomain = domain;

    if (domain)
        WMPutInPLDictionary(defaults->defaults, key, domain);

    /* global domain */
    key = WMCreatePLString("WMGLOBAL");
    defaults->searchList[1] = key;

    path = wdefaultspathfordomain(WMGetFromPLString(key));

    domain = WMReadPropListFromFile(path);

    wfree(path);

    if (!domain)
        domain = WMCreatePLDictionary(NULL, NULL, NULL);

    if (domain)
        WMPutInPLDictionary(defaults->defaults, key, domain);

    /* terminate list */
    defaults->searchList[2] = NULL;

    defaults->searchListArray = WMCreatePLArray(NULL,NULL);

    i = 0;
    while (defaults->searchList[i]) {
        WMAddToPLArray(defaults->searchListArray,
                       defaults->searchList[i]);
        i++;
    }

    if (sharedUserDefaults)
        defaults->next = sharedUserDefaults;
    sharedUserDefaults = defaults;

    addSynchronizeTimerHandler();
    registerSaveOnExit();

    return defaults;
}


WMUserDefaults*
WMGetDefaultsFromPath(char *path)
{
    WMUserDefaults *defaults;
    WMPropList *domain;
    WMPropList *key;
    struct stat stbuf;
    char *name;
    int i;

    assert(path != NULL);

    if (sharedUserDefaults) {
        defaults = sharedUserDefaults;
        while (defaults) {
            if (defaults->path && strcmp(defaults->path, path) == 0)
                return defaults;
            defaults = defaults->next;
        }
    }

    /* we didn't found the database we are looking for. Go read it. */
    defaults = wmalloc(sizeof(WMUserDefaults));
    memset(defaults, 0, sizeof(WMUserDefaults));

    defaults->defaults = WMCreatePLDictionary(NULL, NULL, NULL);

    defaults->searchList = wmalloc(sizeof(WMPropList*)*2);

    /* the domain we want, go in the first position */
    name = strrchr(path, '/');
    if (!name)
        name = path;
    else
        name++;

    key = WMCreatePLString(name);
    defaults->searchList[0] = key;

    if (stat(path, &stbuf) >= 0)
        defaults->timestamp = stbuf.st_mtime;

    domain = WMReadPropListFromFile(path);

    if (!domain)
        domain = WMCreatePLDictionary(NULL, NULL, NULL);

    defaults->path = wstrdup(path);

    defaults->appDomain = domain;

    if (domain)
        WMPutInPLDictionary(defaults->defaults, key, domain);

    /* terminate list */
    defaults->searchList[1] = NULL;

    defaults->searchListArray = WMCreatePLArray(NULL,NULL);

    i = 0;
    while (defaults->searchList[i]) {
        WMAddToPLArray(defaults->searchListArray,
                       defaults->searchList[i]);
        i++;
    }

    if (sharedUserDefaults)
        defaults->next = sharedUserDefaults;
    sharedUserDefaults = defaults;

    addSynchronizeTimerHandler();
    registerSaveOnExit();

    return defaults;
}


/* Returns a WMPropList array with all keys in the user defaults database.
 * Free the array with WMReleasePropList() when no longer needed,
 * but do not free the elements of the array! They're just references. */
WMPropList*
WMGetUDKeys(WMUserDefaults *database)
{
    return WMGetPLDictionaryKeys(database->appDomain);
}


WMPropList*
WMGetUDObjectForKey(WMUserDefaults *database, char *defaultName)
{
    WMPropList *domainName, *domain;
    WMPropList *object = NULL;
    WMPropList *key = WMCreatePLString(defaultName);
    int i = 0;
    
    while (database->searchList[i] && !object) {
	domainName = database->searchList[i];
	domain = WMGetFromPLDictionary(database->defaults, domainName);
	if (domain) {
	    object = WMGetFromPLDictionary(domain, key);
	}
	i++;
    }
    WMReleasePropList(key);
    
    return object;
}


void
WMSetUDObjectForKey(WMUserDefaults *database, WMPropList *object,
		    char *defaultName)
{
    WMPropList *key = WMCreatePLString(defaultName);

    database->dirty = 1;

    WMPutInPLDictionary(database->appDomain, key, object);
    WMReleasePropList(key);
}


void
WMRemoveUDObjectForKey(WMUserDefaults *database, char *defaultName)
{
    WMPropList *key = WMCreatePLString(defaultName);

    database->dirty = 1;

    WMRemoveFromPLDictionary(database->appDomain, key);
    
    WMReleasePropList(key);
}


char*
WMGetUDStringForKey(WMUserDefaults *database, char *defaultName)
{
    WMPropList *val;
    
    val = WMGetUDObjectForKey(database, defaultName);

    if (!val)
	return NULL;

    if (!WMIsPLString(val))
	return NULL;

    return WMGetFromPLString(val);
}


int
WMGetUDIntegerForKey(WMUserDefaults *database, char *defaultName)
{
    WMPropList *val;
    char *str;
    int value;

    val = WMGetUDObjectForKey(database, defaultName);
    
    if (!val)
	return 0;

    if (!WMIsPLString(val))
	return 0;
    
    str = WMGetFromPLString(val);
    if (!str)
	return 0;
    
    if (sscanf(str, "%i", &value)!=1)
	return 0;

    return value;
}



float
WMGetUDFloatForKey(WMUserDefaults *database, char *defaultName)
{
    WMPropList *val;
    char *str;
    float value;

    val = WMGetUDObjectForKey(database, defaultName);
    
    if (!val || !WMIsPLString(val))
	return 0.0;

    if (!(str = WMGetFromPLString(val)))
	return 0.0;

    if (sscanf(str, "%f", &value)!=1)
	return 0.0;

    return value;
}



Bool
WMGetUDBoolForKey(WMUserDefaults *database, char *defaultName)
{
    WMPropList *val;
    int value;
    char *str;

    val = WMGetUDObjectForKey(database, defaultName);
    
    if (!val)
	return False;

    if (!WMIsPLString(val))
	return False;
    
    str = WMGetFromPLString(val);
    if (!str)
	return False;
    
    if (sscanf(str, "%i", &value)==1 && value!=0)
	return True;

    if (strcasecmp(str, "YES")==0)
	return True;

    if (strcasecmp(str, "Y")==0)
	return True;

    return False;
}


void
WMSetUDIntegerForKey(WMUserDefaults *database, int value, char *defaultName)
{
    WMPropList *object;
    char buffer[128];

    sprintf(buffer, "%i", value);
    object = WMCreatePLString(buffer);
 
    WMSetUDObjectForKey(database, object, defaultName);
    WMReleasePropList(object);
}




void
WMSetUDStringForKey(WMUserDefaults *database, char *value, char *defaultName)
{
    WMPropList *object;

    object = WMCreatePLString(value);

    WMSetUDObjectForKey(database, object, defaultName);
    WMReleasePropList(object);
}



void
WMSetUDFloatForKey(WMUserDefaults *database, float value, char *defaultName)
{
    WMPropList *object;
    char buffer[128];

    sprintf(buffer, "%f", value);
    object = WMCreatePLString(buffer);

    WMSetUDObjectForKey(database, object, defaultName);
    WMReleasePropList(object);
}



void
WMSetUDBoolForKey(WMUserDefaults *database, Bool value, char *defaultName)
{
    static WMPropList *yes = NULL, *no = NULL;

    if (!yes) {
	yes = WMCreatePLString("YES");
	no = WMCreatePLString("NO");
    }

    WMSetUDObjectForKey(database, value ? yes : no, defaultName);
}


WMPropList*
WMGetUDSearchList(WMUserDefaults *database)
{
    return database->searchListArray;
}


void
WMSetUDSearchList(WMUserDefaults *database, WMPropList *list)
{
    int i, c;
    
    if (database->searchList) {
	i = 0;
	while (database->searchList[i]) {
	    WMReleasePropList(database->searchList[i]);
	    i++;
	}
	wfree(database->searchList);
    }
    if (database->searchListArray) {
	WMReleasePropList(database->searchListArray);
    }

    c = WMGetPropListItemCount(list);
    database->searchList = wmalloc(sizeof(WMPropList*)*(c+1));

    for (i=0; i<c; i++) {
	database->searchList[i] = WMGetFromPLArray(list, i);
    }
    database->searchList[c] = NULL;
    
    database->searchListArray = WMDeepCopyPropList(list);
}


