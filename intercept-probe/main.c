#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	const char *existing_path = "/tmp";
	const char *missing_path = "/tmp/intercept-probe-missing";
	int rc;
	int failed = 0;

	(void) argc;
	(void) argv;

	errno = 0;
	rc = access(existing_path, F_OK);
	if (rc == 0) {
		dprintf(1,
			"access(\"%s\", F_OK) succeeded: rc=%d errno=%d\n",
			existing_path, rc, errno);
	} else {
		dprintf(1,
			"access(\"%s\", F_OK) failed: rc=%d errno=%d (%s)\n",
			existing_path, rc, errno, strerror(errno));
		failed = 1;
	}

	errno = 0;
	rc = access(missing_path, F_OK);
	if (rc < 0 && errno == ENOENT) {
		dprintf(1,
			"access(\"%s\", F_OK) missing as expected: rc=%d errno=%d (%s)\n",
			missing_path, rc, errno, strerror(errno));
	} else if (rc == 0) {
		dprintf(1,
			"access(\"%s\", F_OK) unexpectedly succeeded: rc=%d errno=%d\n",
			missing_path, rc, errno);
		failed = 1;
	} else {
		dprintf(1,
			"access(\"%s\", F_OK) failed with unexpected errno: rc=%d errno=%d (%s)\n",
			missing_path, rc, errno, strerror(errno));
		failed = 1;
	}

	return failed;
}
