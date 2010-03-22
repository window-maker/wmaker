
#include <stdio.h>
#include <unistd.h>

#include <WINGs/WUtil.h>

/*
 * gcc -D{$os} -I ../../WINGs ${includes...} -Wall -Wextra -g -ggdb3 -c -o ${plat}.o ${plat}.c
 * gcc -I ../../WINGs ${includes...} -g -ggdb3  -c -o test.o test.c
 * gcc -g -ggdb3 -o test ${plat}.o ../../WINGs/.libs/libWUtil.a test.o
 *
 * $ ./test 1 2 'foo bar' "`date`" `date`
 * arg[0] = [./test]
 * arg[1] = [1]
 * arg[2] = [2]
 * arg[3] = [foo bar]
 * arg[4] = [Sat Mar 20 18:36:22 CET 2010]
 * arg[5] = [Sat]
 * arg[6] = [Mar]
 * arg[7] = [20]
 * arg[8] = [18:36:22]
 * arg[9] = [CET]
 * arg[10] = [2010]
 * $
 */

Bool GetCommandForPid(int pid, char ***argv, int *argc);
extern char *__progname;

int main(int argc, char **argv) {

	char **nargv;
	int i, nargc;

	if (argc < 2) {
		printf("Usage: %s arg arg arg ...\n", __progname);
		return 0;
	}

	if (GetCommandForPid(getpid(), &nargv, &nargc) == False) {
		printf("GetCommandForPid() failed\n");
	} else {
		printf("nargv = %d\n", nargc);
		for(i = 0; i < nargc; i++)
			printf("arg[%d] = [%s]\n", i, nargv[i]);
	}

	return 0;
}
