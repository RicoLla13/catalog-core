#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	const char *path = "/tmp";
	int rc;

	(void)argc;
	(void)argv;

	errno = 0;
	rc = access(path, F_OK);
	if (rc == 0) {
		dprintf(1, "access(\"%s\", F_OK) succeeded: rc=%d errno=%d\n",
			path, rc, errno);
	} else {
		dprintf(1, "access(\"%s\", F_OK) failed: rc=%d errno=%d (%s)\n",
			path, rc, errno, strerror(errno));
	}

	return 0;
}
