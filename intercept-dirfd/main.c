#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	const char *dir_path = "/tmp";
	const char *relative_file = "intercept-dirfd.txt";
	const char *nested_missing = "intercept-dirfd-missing-child";
	int strict_dirfd = -1;
	int classified_dirfd = -1;
	int filefd = -1;
	int rc;
	int failed = 0;
	struct stat st;

	(void) argc;
	(void) argv;

	errno = 0;
	rc = access(dir_path, F_OK);
	if (rc == 0) {
		dprintf(1, "access(\"%s\", F_OK) succeeded: rc=%d errno=%d\n",
			dir_path, rc, errno);
	} else {
		dprintf(1, "access(\"%s\", F_OK) failed: rc=%d errno=%d (%s)\n",
			dir_path, rc, errno, strerror(errno));
		return 1;
	}

	errno = 0;
	strict_dirfd = openat(AT_FDCWD, dir_path, O_RDONLY | O_DIRECTORY, 0);
	if (strict_dirfd >= 0) {
		dprintf(1,
			"openat(AT_FDCWD, \"%s\", O_RDONLY|O_DIRECTORY) succeeded: fd=%d errno=%d\n",
			dir_path, strict_dirfd, errno);
	} else {
		dprintf(1,
			"openat(AT_FDCWD, \"%s\", O_RDONLY|O_DIRECTORY) failed: fd=%d errno=%d (%s)\n",
			dir_path, strict_dirfd, errno, strerror(errno));
		return 1;
	}

	errno = 0;
	classified_dirfd = openat(AT_FDCWD, dir_path, O_RDONLY, 0);
	if (classified_dirfd >= 0) {
		dprintf(1,
			"openat(AT_FDCWD, \"%s\", O_RDONLY) succeeded: fd=%d errno=%d\n",
			dir_path, classified_dirfd, errno);
	} else {
		dprintf(1,
			"openat(AT_FDCWD, \"%s\", O_RDONLY) failed: fd=%d errno=%d (%s)\n",
			dir_path, classified_dirfd, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	filefd = openat(classified_dirfd, relative_file,
		       O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (filefd >= 0) {
		dprintf(1,
			"openat(%d, \"%s\", O_CREAT|O_TRUNC|O_WRONLY, 0644) succeeded: fd=%d errno=%d\n",
			classified_dirfd, relative_file, filefd, errno);
	} else {
		dprintf(1,
			"openat(%d, \"%s\", O_CREAT|O_TRUNC|O_WRONLY, 0644) failed: fd=%d errno=%d (%s)\n",
			classified_dirfd, relative_file, filefd, errno,
			strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	rc = close(filefd);
	if (rc == 0) {
		dprintf(1, "close(%d) succeeded: rc=%d errno=%d\n", filefd, rc,
			errno);
		filefd = -1;
	} else {
		dprintf(1, "close(%d) failed: rc=%d errno=%d (%s)\n", filefd, rc,
			errno, strerror(errno));
		failed = 1;
		goto out;
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = fstatat(strict_dirfd, relative_file, &st, 0);
	if (rc == 0) {
		dprintf(1,
			"fstatat(%d, \"%s\", 0) succeeded: rc=%d errno=%d mode=%o size=%lld nlink=%lu\n",
			strict_dirfd, relative_file, rc, errno, st.st_mode,
			(long long) st.st_size, (unsigned long) st.st_nlink);
	} else {
		dprintf(1,
			"fstatat(%d, \"%s\", 0) failed: rc=%d errno=%d (%s)\n",
			strict_dirfd, relative_file, rc, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = fstatat(classified_dirfd, relative_file, &st, 0);
	if (rc == 0) {
		dprintf(1,
			"fstatat(%d, \"%s\", 0) via classified dirfd succeeded: rc=%d errno=%d mode=%o size=%lld nlink=%lu\n",
			classified_dirfd, relative_file, rc, errno, st.st_mode,
			(long long) st.st_size, (unsigned long) st.st_nlink);
	} else {
		dprintf(1,
			"fstatat(%d, \"%s\", 0) via classified dirfd failed: rc=%d errno=%d (%s)\n",
			classified_dirfd, relative_file, rc, errno,
			strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	filefd = openat(strict_dirfd, relative_file, O_RDONLY, 0);
	if (filefd >= 0) {
		dprintf(1,
			"openat(%d, \"%s\", O_RDONLY) succeeded: fd=%d errno=%d\n",
			strict_dirfd, relative_file, filefd, errno);
	} else {
		dprintf(1,
			"openat(%d, \"%s\", O_RDONLY) failed: fd=%d errno=%d (%s)\n",
			strict_dirfd, relative_file, filefd, errno,
			strerror(errno));
		failed = 1;
		goto out;
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = fstatat(filefd, nested_missing, &st, 0);
	if (rc == -1 && errno == ENOTDIR) {
		dprintf(1,
			"fstatat(%d, \"%s\", 0) on regular-file dirfd correctly failed: rc=%d errno=%d (%s)\n",
			filefd, nested_missing, rc, errno, strerror(errno));
	} else {
		dprintf(1,
			"fstatat(%d, \"%s\", 0) on regular-file dirfd returned unexpected result: rc=%d errno=%d (%s)\n",
			filefd, nested_missing, rc, errno, strerror(errno));
		failed = 1;
		goto out;
	}

out:
	if (filefd >= 0) {
		errno = 0;
		rc = close(filefd);
		if (rc == 0) {
			dprintf(1, "close(%d) succeeded: rc=%d errno=%d\n", filefd,
				rc, errno);
		} else {
			dprintf(1, "close(%d) failed: rc=%d errno=%d (%s)\n", filefd,
				rc, errno, strerror(errno));
			failed = 1;
		}
	}

	if (classified_dirfd >= 0) {
		errno = 0;
		rc = close(classified_dirfd);
		if (rc == 0) {
			dprintf(1, "close(%d) succeeded: rc=%d errno=%d\n",
				classified_dirfd, rc, errno);
		} else {
			dprintf(1, "close(%d) failed: rc=%d errno=%d (%s)\n",
				classified_dirfd, rc, errno, strerror(errno));
			failed = 1;
		}
	}

	if (strict_dirfd >= 0) {
		errno = 0;
		rc = close(strict_dirfd);
		if (rc == 0) {
			dprintf(1, "close(%d) succeeded: rc=%d errno=%d\n",
				strict_dirfd, rc, errno);
		} else {
			dprintf(1, "close(%d) failed: rc=%d errno=%d (%s)\n",
				strict_dirfd, rc, errno, strerror(errno));
			failed = 1;
		}
	}

	return failed;
}
