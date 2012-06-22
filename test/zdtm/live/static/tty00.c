#define _XOPEN_SOURCE
#include <stdlib.h>
#include "zdtmtst.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <sys/ioctl.h>

const char *test_doc	= "Check that a control terminal is restored";
const char *test_author	= "Andrey Vagin <avagin@openvz.org>";

static int sighup = 0;
static void sighup_handler(int signo)
{
	test_msg("SIGHUP is here\n");
	sighup = 1;
}

int main(int argc, char ** argv)
{
	int fdm, fds, status;
	char *slavename;
	pid_t pid;

	test_init(argc, argv);

	fdm = open("/dev/ptmx", O_RDWR);
	if (fdm == -1) {
		err("Can't open a master pseudoterminal");
		return 1;
	}

	grantpt(fdm);
	unlockpt(fdm);
	slavename = ptsname(fdm);

	pid = test_fork();
	if (pid < 0) {
		err("fork() failed");
		return 1;
	}

	if (pid == 0) {
		close(fdm);
		signal(SIGHUP, sighup_handler);

		if (setsid() == -1)
			return 1;

		/* set up a controlling terminal */
		fds = open(slavename, O_RDWR);
		if (fds == -1) {
			err("Can't open a slave pseudoterminal %s", slavename);
			return 1;
		}

		if (ioctl(fdm, TIOCSCTTY, 1) < 0) {
			err("Can't setup a controlling terminal");
			return 1;
		}
		close(fds);

		test_waitsig();
		if (sighup)
			return 0;
		return 1;
	}


	test_daemon();

	test_waitsig();

	close(fdm);

	if (kill(pid, SIGTERM) == -1) {
		err("kill failed");
		return 1;
	}

	pid = waitpid(pid, &status, 0);
	if (pid < 0)
		return 1;

	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status)) {
			fail("The child returned %d", WEXITSTATUS(status));
			return 1;
		}
	} else
		err("The child has been killed by %d", WTERMSIG(status));

	pass();

	return 0;
}
