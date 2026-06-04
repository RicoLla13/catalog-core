#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define OK_PREFIX "[ OK  ]"
#define ERR_PREFIX "[ ERR ]"

int main(int argc, char *argv[])
{
	const char *existing_path = "/tmp/intercept-simple-root/existing.txt";
	const char *missing_path = "/tmp/intercept-simple-root/missing.txt";
	int rc;
	int failed = 0;

	(void) argc;
	(void) argv;

	/* Positive path probe against the host-backed remote filesystem. */
	errno = 0;
	rc = access(existing_path, F_OK);
	if (rc == 0) {
		printf(
			OK_PREFIX " access(\"%s\", F_OK) succeeded: rc=%d errno=%d\n",
			existing_path, rc, errno);
	} else {
		printf(
			ERR_PREFIX " access(\"%s\", F_OK) failed: rc=%d errno=%d (%s)\n",
			existing_path, rc, errno, strerror(errno));
		failed = 1;
	}

	/* Negative probe to show how remote ENOENT is surfaced back to the guest. */
	errno = 0;
	rc = access(missing_path, F_OK);
	if (rc < 0 && errno == ENOENT) {
		printf(
			OK_PREFIX " access(\"%s\", F_OK) missing as expected: rc=%d errno=%d (%s)\n",
			missing_path, rc, errno, strerror(errno));
	} else if (rc == 0) {
		printf(
			ERR_PREFIX " access(\"%s\", F_OK) unexpectedly succeeded: rc=%d errno=%d\n",
			missing_path, rc, errno);
		failed = 1;
	} else {
		printf(
			ERR_PREFIX " access(\"%s\", F_OK) failed with unexpected errno: rc=%d errno=%d (%s)\n",
			missing_path, rc, errno, strerror(errno));
		failed = 1;
	}

	return failed;
}
