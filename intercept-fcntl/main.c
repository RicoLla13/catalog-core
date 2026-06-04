#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define OK_PREFIX "[ OK  ]"
#define ERR_PREFIX "[ ERR ]"

int main(int argc, char *argv[])
{
	const char *dir_path = "/tmp/intercept-fcntl-root";
	const char *file_path = "/tmp/intercept-fcntl-root/intercept-fcntl.txt";
	int fd = -1;
	int dupfd = -1;
	int rc;
	int failed = 0;
	int fdflags;
	int status_flags;
	struct flock flk;

	(void) argc;
	(void) argv;

	errno = 0;
	rc = access(dir_path, F_OK);
	if (rc == 0) {
		printf(OK_PREFIX " access(\"%s\", F_OK) succeeded: rc=%d errno=%d\n",
			dir_path, rc, errno);
	} else {
		printf(ERR_PREFIX " access(\"%s\", F_OK) failed: rc=%d errno=%d (%s)\n",
			dir_path, rc, errno, strerror(errno));
		return 1;
	}

	errno = 0;
	fd = openat(AT_FDCWD, file_path, O_CREAT | O_TRUNC | O_RDWR, 0644);
	if (fd >= 0) {
		printf(
			OK_PREFIX " openat(AT_FDCWD, \"%s\", O_CREAT|O_TRUNC|O_RDWR, 0644) succeeded: fd=%d errno=%d\n",
			file_path, fd, errno);
	} else {
		printf(
			ERR_PREFIX " openat(AT_FDCWD, \"%s\", O_CREAT|O_TRUNC|O_RDWR, 0644) failed: fd=%d errno=%d (%s)\n",
			file_path, fd, errno, strerror(errno));
		return 1;
	}

	errno = 0;
	fdflags = fcntl(fd, F_GETFD);
	if (fdflags >= 0) {
		printf(OK_PREFIX " fcntl(%d, F_GETFD) succeeded: flags=%d errno=%d\n",
			fd, fdflags, errno);
	} else {
		printf(ERR_PREFIX " fcntl(%d, F_GETFD) failed: rc=%d errno=%d (%s)\n",
			fd, fdflags, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	rc = fcntl(fd, F_SETFD, FD_CLOEXEC);
	if (rc == 0) {
		printf(OK_PREFIX " fcntl(%d, F_SETFD, FD_CLOEXEC) succeeded: rc=%d errno=%d\n",
			fd, rc, errno);
	} else {
		printf(ERR_PREFIX " fcntl(%d, F_SETFD, FD_CLOEXEC) failed: rc=%d errno=%d (%s)\n",
			fd, rc, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	fdflags = fcntl(fd, F_GETFD);
	if (fdflags >= 0 && (fdflags & FD_CLOEXEC)) {
		printf(
			OK_PREFIX " fcntl(%d, F_GETFD) after set succeeded: flags=%d errno=%d\n",
			fd, fdflags, errno);
	} else {
		printf(
			ERR_PREFIX " fcntl(%d, F_GETFD) after set returned unexpected flags=%d errno=%d (%s)\n",
			fd, fdflags, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	status_flags = fcntl(fd, F_GETFL);
	if (status_flags >= 0) {
		printf(
			OK_PREFIX " fcntl(%d, F_GETFL) succeeded: flags=%#x errno=%d\n",
			fd, status_flags, errno);
	} else {
		printf(
			ERR_PREFIX " fcntl(%d, F_GETFL) failed: rc=%d errno=%d (%s)\n",
			fd, status_flags, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	rc = fcntl(fd, F_SETFL, status_flags | O_NONBLOCK);
	if (rc == 0) {
		printf(
			OK_PREFIX " fcntl(%d, F_SETFL, flags|O_NONBLOCK) succeeded: rc=%d errno=%d\n",
			fd, rc, errno);
	} else {
		printf(
			ERR_PREFIX " fcntl(%d, F_SETFL, flags|O_NONBLOCK) failed: rc=%d errno=%d (%s)\n",
			fd, rc, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	status_flags = fcntl(fd, F_GETFL);
	if (status_flags >= 0 && (status_flags & O_NONBLOCK)) {
		printf(
			OK_PREFIX " fcntl(%d, F_GETFL) after set succeeded: flags=%#x errno=%d\n",
			fd, status_flags, errno);
	} else {
		printf(
			ERR_PREFIX " fcntl(%d, F_GETFL) after set returned unexpected flags=%#x errno=%d (%s)\n",
			fd, status_flags, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	dupfd = fcntl(fd, F_DUPFD, 10);
	if (dupfd >= 10) {
		printf(OK_PREFIX " fcntl(%d, F_DUPFD, 10) succeeded: newfd=%d errno=%d\n",
			fd, dupfd, errno);
	} else {
		printf(ERR_PREFIX " fcntl(%d, F_DUPFD, 10) failed: rc=%d errno=%d (%s)\n",
			fd, dupfd, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	status_flags = fcntl(dupfd, F_GETFL);
	if (status_flags >= 0 && (status_flags & O_NONBLOCK)) {
		printf(
			OK_PREFIX " fcntl(%d, F_GETFL) on duplicate succeeded: flags=%#x errno=%d\n",
			dupfd, status_flags, errno);
	} else {
		printf(
			ERR_PREFIX " fcntl(%d, F_GETFL) on duplicate returned unexpected flags=%#x errno=%d (%s)\n",
			dupfd, status_flags, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	memset(&flk, 0, sizeof(flk));
	flk.l_type = F_WRLCK;
	flk.l_whence = SEEK_SET;
	flk.l_start = 0;
	flk.l_len = 0;

	errno = 0;
	rc = fcntl(fd, F_SETLK, &flk);
	if (rc == 0) {
		printf(OK_PREFIX " fcntl(%d, F_SETLK, WRLCK) succeeded: rc=%d errno=%d\n",
			fd, rc, errno);
	} else {
		printf(ERR_PREFIX " fcntl(%d, F_SETLK, WRLCK) failed: rc=%d errno=%d (%s)\n",
			fd, rc, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	flk.l_type = F_UNLCK;
	errno = 0;
	rc = fcntl(fd, F_SETLK, &flk);
	if (rc == 0) {
		printf(OK_PREFIX " fcntl(%d, F_SETLK, UNLCK) succeeded: rc=%d errno=%d\n",
			fd, rc, errno);
	} else {
		printf(ERR_PREFIX " fcntl(%d, F_SETLK, UNLCK) failed: rc=%d errno=%d (%s)\n",
			fd, rc, errno, strerror(errno));
		failed = 1;
		goto out;
	}

out:
	if (dupfd >= 0) {
		errno = 0;
		rc = close(dupfd);
		if (rc == 0) {
			printf(OK_PREFIX " close(%d) succeeded: rc=%d errno=%d\n",
				dupfd, rc, errno);
		} else {
			printf(ERR_PREFIX " close(%d) failed: rc=%d errno=%d (%s)\n",
				dupfd, rc, errno, strerror(errno));
			failed = 1;
		}
	}

	if (fd >= 0) {
		errno = 0;
		rc = close(fd);
		if (rc == 0) {
			printf(OK_PREFIX " close(%d) succeeded: rc=%d errno=%d\n", fd, rc,
				errno);
		} else {
			printf(ERR_PREFIX " close(%d) failed: rc=%d errno=%d (%s)\n", fd, rc,
				errno, strerror(errno));
			failed = 1;
		}
	}

	return failed;
}
