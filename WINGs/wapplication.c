
#include <unistd.h>
#include <X11/Xlocale.h>

#include "WINGsP.h"
#include "wconfig.h"
#include "userdefaults.h"


struct W_Application WMApplication;

const char *_WINGS_progname = NULL;

Bool W_ApplicationInitialized(void)
{
	return _WINGS_progname != NULL;
}

void WMInitializeApplication(const char *applicationName, int *argc, char **argv)
{
	int i;

	assert(argc != NULL);
	assert(argv != NULL);
	assert(applicationName != NULL);

	setlocale(LC_ALL, "");

#ifdef I18N
	if (getenv("NLSPATH"))
		bindtextdomain("WINGs", getenv("NLSPATH"));
	else
		bindtextdomain("WINGs", LOCALEDIR);
	bind_textdomain_codeset("WINGs", "UTF-8");
#endif

	_WINGS_progname = argv[0];

	WMApplication.applicationName = wstrdup(applicationName);
	WMApplication.argc = *argc;

	WMApplication.argv = wmalloc((*argc + 1) * sizeof(char *));
	for (i = 0; i < *argc; i++) {
		WMApplication.argv[i] = wstrdup(argv[i]);
	}
	WMApplication.argv[i] = NULL;

	/* initialize notification center */
	W_InitNotificationCenter();
}

void WMReleaseApplication(void) {
	int i;

	/*
	 * We save the configuration on exit, this used to be handled
	 * through an 'atexit' registered function but if application
	 * properly calls WMReleaseApplication then the info to that
	 * will have been freed by us.
	 */
	w_save_defaults_changes();

	W_ReleaseNotificationCenter();

	if (WMApplication.applicationName) {
		wfree(WMApplication.applicationName);
		WMApplication.applicationName = NULL;
	}

	if (WMApplication.argv) {
		for (i = 0; i < WMApplication.argc; i++)
			wfree(WMApplication.argv[i]);

		wfree(WMApplication.argv);
		WMApplication.argv = NULL;
	}
}

void WMSetResourcePath(const char *path)
{
	if (WMApplication.resourcePath)
		wfree(WMApplication.resourcePath);
	WMApplication.resourcePath = wstrdup(path);
}

char *WMGetApplicationName(void)
{
	return WMApplication.applicationName;
}

static char *checkFile(const char *path, const char *folder, const char *ext, const char *resource)
{
	char *ret;
	int extralen;
	size_t slen;

	if (!path || !resource)
		return NULL;

	extralen = (ext ? strlen(ext) + 1 : 0) + (folder ? strlen(folder) + 1 : 0) + 1;
	slen = strlen(path) + strlen(resource) + 1 + extralen;
	ret = wmalloc(slen);

	if (wstrlcpy(ret, path, slen) >= slen)
		goto error;

	if (folder &&
		(wstrlcat(ret, "/", slen) >= slen ||
		 wstrlcat(ret, folder, slen) >= slen))
			goto error;

	if (ext &&
		(wstrlcat(ret, "/", slen) >= slen ||
		 wstrlcat(ret, ext, slen) >= slen))
			goto error;

	if (wstrlcat(ret, "/", slen) >= slen ||
	    wstrlcat(ret, resource, slen) >= slen)
		goto error;

	if (access(ret, F_OK) != 0)
		goto error;

	return ret;

error:
	if (ret)
		wfree(ret);
	return NULL;
}

char *WMPathForResourceOfType(const char *resource, const char *ext)
{
	const char *gslocapps, *gssysapps, *gsuserapps;
	char *path, *appdir;
	char buffer[PATH_MAX];
	size_t slen;

	path = appdir = NULL;

	/*
	 * Paths are searched in this order:
	 * - resourcePath/ext
	 * - dirname(argv[0])/ext
	 * - WMAKER_USER_ROOT/Applications/ApplicationName.app/ext
	 * - GNUSTEP_USER_APPS/ApplicationName.app/ext
	 * - GNUSTEP_LOCAL_APPS/ApplicationName.app/ext
	 * - /usr/local/lib/GNUstep/Applications/ApplicationName.app/ext
	 * - GNUSTEP_SYSTEM_APPS/ApplicationName.app/ext
	 * - /usr/lib/GNUstep/Applications/ApplicationName.app/ext
	 */

	if (WMApplication.resourcePath) {
		path = checkFile(WMApplication.resourcePath, NULL, ext, resource);
		if (path)
			goto out;
	}

	if (WMApplication.argv[0]) {
		char *ptr_slash;

		ptr_slash = strrchr(WMApplication.argv[0], '/');
		if (ptr_slash != NULL) {
			char tmp[ptr_slash - WMApplication.argv[0] + 1];

			strncpy(tmp, WMApplication.argv[0], sizeof(tmp)-1);
			tmp[sizeof(tmp) - 1] = '\0';

			path = checkFile(tmp, NULL, ext, resource);
			if (path)
				goto out;
		}
	}

	snprintf(buffer, sizeof(buffer), "Applications/%s.app", WMApplication.applicationName);
	path = checkFile(GETENV("WMAKER_USER_ROOT"), buffer, ext, resource);
	if (path)
		goto out;

	slen = strlen(WMApplication.applicationName) + sizeof("/.app");
	appdir = wmalloc(slen);
	if (snprintf(appdir, slen, "/%s.app", WMApplication.applicationName) >= slen)
		goto out;

	gsuserapps = GETENV("GNUSTEP_USER_APPS");
	if (!gsuserapps) {
		snprintf(buffer, sizeof(buffer), "%s/Applications", wusergnusteppath());
		gsuserapps = buffer;
	}
	path = checkFile(gsuserapps, appdir, ext, resource);
	if (path)
		goto out;

	gslocapps = GETENV("GNUSTEP_LOCAL_APPS");
	if (!gslocapps)
		gslocapps = "/usr/local/lib/GNUstep/Applications";
	path = checkFile(gslocapps, appdir, ext, resource);
	if (path)
		goto out;

	gssysapps = GETENV("GNUSTEP_SYSTEM_APPS");
	if (!gssysapps)
		gssysapps = "/usr/lib/GNUstep/Applications";
	path = checkFile(gssysapps, appdir, ext, resource);
	if (path)
		goto out;

	path = checkFile("/usr/GNUstep/System/Applications", appdir, ext, resource); /* falls through */

out:
	if (appdir)
		wfree(appdir);

	return path;

}
