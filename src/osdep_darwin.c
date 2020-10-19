#include <sys/types.h>
#include <sys/sysctl.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <WINGs/WUtil.h>

#include "wconfig.h"
#include "osdep.h"

/*
 * copy argc and argv for an existing process identified by `pid'
 * into suitable storage given in ***argv and *argc.
 *
 * subsequent calls use the same static area for argv and argc.
 *
 * returns 0 for failure, in which case argc := 0 and argv := NULL
 * returns 1 for success
 */
Bool GetCommandForPid(int pid, char ***argv, int *argc)
#ifdef KERN_PROCARGS2
{
	int mib[4];
	unsigned int idx;
	size_t count;
	static char *args = NULL;
	static int argmax = 0;

	*argv = NULL;
	*argc = 0;

	/* the system-wide limit */
	if (argmax == 0) { /* it hopefully doesn't change at runtime *g* */
		mib[0] = CTL_KERN;
		mib[1] = KERN_ARGMAX;
		mib[2] = 0;
		mib[3] = 0;

		count = sizeof(argmax);
		if (sysctl(mib, 2, &argmax, &count, NULL, 0) == -1)
			return False;
	}

	/* if argmax is still 0, something went very seriously wrong */
	assert(argmax > 0);

	/* space for args; no need to free before returning even on errors */
	if (args == NULL)
		args = (char *)wmalloc(argmax);

	/* get process args */
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROCARGS2;
	mib[2] = pid;

	count = argmax;
	if (sysctl(mib, 3, args, &count, NULL, 0) == -1 || count == 0)
		return False;

	/* get argc, skip */
	memcpy(argc, args, sizeof(*argc));
	idx = sizeof(*argc);

	while (args[idx++] != '\0')		/* skip execname */
		;
	while (args[idx] == '\0')		/* padding too */
		idx++;
	/* args[idx] is at at begininng of args now */

    int found = 0;
    char *p = &args[idx];
    while(found < *argc)
    {
        while(*p != '\0') p++; // look for the next \0
        while(*p == '\0') p++; // skip over padding \0s
        found++;

        // Don’t overrun!
        if (p-args >= argmax)
        {
            return False;
        }
    }
    // At this point, p points to the last \0 in the source array. 

    // Buffer needed for the strings
    unsigned stringbuf_size = p - &args[idx];

    // Buffer needed for the pointers (plus one terminating NULL)
    unsigned pointerbuf_size = sizeof(char *) * (*argc + 1);
    
    *argv = wmalloc(pointerbuf_size + stringbuf_size);
    char* stringstart = (char *)(*argv) + pointerbuf_size;

    memcpy(stringstart, &args[idx], stringbuf_size);

    found = 0;
    p = stringstart;
    while(found < *argc)
    {
        (*argv)[found] = p;
                
        while(*p != '\0') p++; // look for the next \0
        while(*p == '\0') p++; // skip over padding \0s
        
        found++;
    }
    (*argv)[found] = NULL; // Terminating NULL
    
	return True;
}
#else /* !KERN_PROCARGS2 */
{
	*argv = NULL;
	*argc = 0;

	return False;
}
#endif
