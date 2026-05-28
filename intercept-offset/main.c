#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define OK_PREFIX "[]"
#define ERR_PREFIX "[x]"

int main(int argc, char *argv[])
{
	char buf[160];
	const char *dir_path = "/tmp";
	const char *file_path = "/tmp/intercept-offset.txt";
	const char *payload = "offset stable remote pread payload";
	int fd = -1;
	int rc;
	int failed = 0;
	off_t pos_before;
	off_t pos_after;
	ssize_t nread;
	ssize_t nwritten;

	(void) argc;
	(void) argv;

	errno = 0;
	rc = access(dir_path, F_OK);
	if (rc == 0) {
		dprintf(1, OK_PREFIX " access(\"%s\", F_OK) succeeded: rc=%d errno=%d\n",
			dir_path, rc, errno);
	} else {
		dprintf(1, ERR_PREFIX " access(\"%s\", F_OK) failed: rc=%d errno=%d (%s)\n",
			dir_path, rc, errno, strerror(errno));
		return 1;
	}

	errno = 0;
	fd = openat(AT_FDCWD, file_path, O_CREAT | O_TRUNC | O_RDWR, 0644);
	if (fd >= 0) {
		dprintf(1,
			OK_PREFIX " openat(AT_FDCWD, \"%s\", O_CREAT|O_TRUNC|O_RDWR, 0644) succeeded: fd=%d errno=%d\n",
			file_path, fd, errno);
	} else {
		dprintf(1,
			ERR_PREFIX " openat(AT_FDCWD, \"%s\", O_CREAT|O_TRUNC|O_RDWR, 0644) failed: fd=%d errno=%d (%s)\n",
			file_path, fd, errno, strerror(errno));
		return 1;
	}

	errno = 0;
	nwritten = write(fd, payload, strlen(payload));
	if (nwritten >= 0) {
		dprintf(1, OK_PREFIX " write(%d, %zu) succeeded: rc=%zd errno=%d\n", fd,
			strlen(payload), nwritten, errno);
	} else {
		dprintf(1, ERR_PREFIX " write(%d, %zu) failed: rc=%zd errno=%d (%s)\n", fd,
			strlen(payload), nwritten, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	pos_before = lseek(fd, 0, SEEK_CUR);
	if (pos_before >= 0) {
		dprintf(1,
			OK_PREFIX " lseek(%d, 0, SEEK_CUR) before pread succeeded: off=%lld errno=%d\n",
			fd, (long long)pos_before, errno);
	} else {
		dprintf(1,
			ERR_PREFIX " lseek(%d, 0, SEEK_CUR) before pread failed: off=%lld errno=%d (%s)\n",
			fd, (long long)pos_before, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	memset(buf, 0, sizeof(buf));
	errno = 0;
	nread = pread(fd, buf, 6, 0);
	if (nread >= 0) {
		buf[nread] = '\0';
		dprintf(1,
			OK_PREFIX " pread(%d, 6, 0) succeeded: rc=%zd errno=%d data=\"%s\"\n",
			fd, nread, errno, buf);
	} else {
		dprintf(1,
			ERR_PREFIX " pread(%d, 6, 0) failed: rc=%zd errno=%d (%s)\n",
			fd, nread, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	memset(buf, 0, sizeof(buf));
	errno = 0;
	nread = pread(fd, buf, 6, 7);
	if (nread >= 0) {
		buf[nread] = '\0';
		dprintf(1,
			OK_PREFIX " pread(%d, 6, 7) succeeded: rc=%zd errno=%d data=\"%s\"\n",
			fd, nread, errno, buf);
	} else {
		dprintf(1,
			ERR_PREFIX " pread(%d, 6, 7) failed: rc=%zd errno=%d (%s)\n",
			fd, nread, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	pos_after = lseek(fd, 0, SEEK_CUR);
	if (pos_after >= 0) {
		dprintf(1,
			OK_PREFIX " lseek(%d, 0, SEEK_CUR) after pread succeeded: off=%lld errno=%d\n",
			fd, (long long)pos_after, errno);
	} else {
		dprintf(1,
			ERR_PREFIX " lseek(%d, 0, SEEK_CUR) after pread failed: off=%lld errno=%d (%s)\n",
			fd, (long long)pos_after, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	if (pos_after != pos_before) {
		dprintf(1,
			ERR_PREFIX " pread unexpectedly changed the current offset: before=%lld after=%lld\n",
			(long long)pos_before, (long long)pos_after);
		failed = 1;
	}

out:
	if (fd >= 0) {
		errno = 0;
		rc = close(fd);
		if (rc == 0) {
			dprintf(1, OK_PREFIX " close(%d) succeeded: rc=%d errno=%d\n", fd, rc,
				errno);
		} else {
			dprintf(1, ERR_PREFIX " close(%d) failed: rc=%d errno=%d (%s)\n", fd, rc,
				errno, strerror(errno));
			failed = 1;
		}
	}

	return failed;
}
