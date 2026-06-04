#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define OK_PREFIX "[]"
#define ERR_PREFIX "[x]"

static int visit_entry(int rootfd, const char *entry_name, int *checked_pread)
{
	char buf[96];
	struct stat st;
	off_t pos;
	off_t pos_after;
	ssize_t nread;
	int fd;
	int rc;

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = fstatat(rootfd, entry_name, &st, 0);
	if (rc != 0) {
		dprintf(1,
			ERR_PREFIX " fstatat(%d, \"%s\", 0) failed: rc=%d errno=%d (%s)\n",
			rootfd, entry_name, rc, errno, strerror(errno));
		return 1;
	}

	dprintf(1,
		OK_PREFIX " fstatat(%d, \"%s\", 0) succeeded: rc=%d errno=%d mode=%o size=%lld\n",
		rootfd, entry_name, rc, errno, st.st_mode, (long long) st.st_size);

	errno = 0;
	fd = openat(rootfd, entry_name, O_RDONLY, 0);
	if (fd < 0) {
		dprintf(1,
			ERR_PREFIX " openat(%d, \"%s\", O_RDONLY) failed: fd=%d errno=%d (%s)\n",
			rootfd, entry_name, fd, errno, strerror(errno));
		return 1;
	}

	dprintf(1,
		OK_PREFIX " openat(%d, \"%s\", O_RDONLY) succeeded: fd=%d errno=%d\n",
		rootfd, entry_name, fd, errno);

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = fstat(fd, &st);
	if (rc != 0) {
		dprintf(1, ERR_PREFIX " fstat(%d) failed: rc=%d errno=%d (%s)\n",
			fd, rc, errno, strerror(errno));
		(void) close(fd);
		return 1;
	}

	dprintf(1, OK_PREFIX " fstat(%d) succeeded: rc=%d errno=%d size=%lld\n",
		fd, rc, errno, (long long) st.st_size);

	if (!*checked_pread) {
		errno = 0;
		pos = lseek(fd, 0, SEEK_CUR);
		if (pos < 0) {
			dprintf(1, ERR_PREFIX " lseek(%d, 0, SEEK_CUR) failed: off=%lld errno=%d (%s)\n",
				fd, (long long) pos, errno, strerror(errno));
			(void) close(fd);
			return 1;
		}

		memset(buf, 0, sizeof(buf));
		errno = 0;
		nread = pread(fd, buf, 15, 0);
		if (nread < 0) {
			dprintf(1, ERR_PREFIX " pread(%d, 15, 0) failed: rc=%zd errno=%d (%s)\n",
				fd, nread, errno, strerror(errno));
			(void) close(fd);
			return 1;
		}

		buf[nread] = '\0';
		dprintf(1,
			OK_PREFIX " pread(%d, 15, 0) succeeded: rc=%zd errno=%d data=\"%s\"\n",
			fd, nread, errno, buf);

		errno = 0;
		pos_after = lseek(fd, 0, SEEK_CUR);
		if (pos_after != pos) {
			dprintf(1,
				ERR_PREFIX " pread changed the current offset for fd=%d: before=%lld after=%lld errno=%d\n",
				fd, (long long) pos, (long long) pos_after, errno);
			(void) close(fd);
			return 1;
		}

		dprintf(1,
			OK_PREFIX " lseek(%d, 0, SEEK_CUR) after pread confirmed unchanged offset=%lld errno=%d\n",
			fd, (long long) pos_after, errno);
		*checked_pread = 1;
	} else {
		memset(buf, 0, sizeof(buf));
		errno = 0;
		nread = read(fd, buf, sizeof(buf) - 1);
		if (nread < 0) {
			dprintf(1, ERR_PREFIX " read(%d, %zu) failed: rc=%zd errno=%d (%s)\n",
				fd, sizeof(buf) - 1, nread, errno, strerror(errno));
			(void) close(fd);
			return 1;
		}

		buf[nread] = '\0';
		dprintf(1,
			OK_PREFIX " read(%d, %zu) succeeded: rc=%zd errno=%d data=\"%s\"\n",
			fd, sizeof(buf) - 1, nread, errno, buf);
	}

	errno = 0;
	rc = close(fd);
	if (rc != 0) {
		dprintf(1, ERR_PREFIX " close(%d) failed: rc=%d errno=%d (%s)\n",
			fd, rc, errno, strerror(errno));
		return 1;
	}

	dprintf(1, OK_PREFIX " close(%d) succeeded: rc=%d errno=%d\n",
		fd, rc, errno);
	return 0;
}

int main(int argc, char *argv[])
{
	char manifest[256];
	char *entry;
	char *saveptr;
	const char *root_path = "/tmp/intercept-tree-root";
	const char *manifest_path = "/tmp/intercept-tree-root/manifest.txt";
	const char *link_path = "/tmp/intercept-tree-root/current-link.txt";
	const char *missing_path = "/tmp/intercept-tree-root/missing.txt";
	const char *regular_child = "child-under-file";
	int rootfd = -1;
	int manifestfd = -1;
	int leaf_fd = -1;
	int checked_pread = 0;
	int rc;
	int failed = 0;
	struct stat st;
	ssize_t nread;

	(void) argc;
	(void) argv;

	errno = 0;
	rc = access(root_path, F_OK);
	if (rc == 0) {
		dprintf(1, OK_PREFIX " access(\"%s\", F_OK) succeeded: rc=%d errno=%d\n",
			root_path, rc, errno);
	} else {
		dprintf(1, ERR_PREFIX " access(\"%s\", F_OK) failed: rc=%d errno=%d (%s)\n",
			root_path, rc, errno, strerror(errno));
		return 1;
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = stat(manifest_path, &st);
	if (rc == 0) {
		dprintf(1,
			OK_PREFIX " stat(\"%s\") succeeded: rc=%d errno=%d mode=%o size=%lld\n",
			manifest_path, rc, errno, st.st_mode, (long long) st.st_size);
	} else {
		dprintf(1, ERR_PREFIX " stat(\"%s\") failed: rc=%d errno=%d (%s)\n",
			manifest_path, rc, errno, strerror(errno));
		return 1;
	}

	errno = 0;
	rootfd = openat(AT_FDCWD, root_path, O_RDONLY | O_DIRECTORY, 0);
	if (rootfd >= 0) {
		dprintf(1,
			OK_PREFIX " openat(AT_FDCWD, \"%s\", O_RDONLY|O_DIRECTORY) succeeded: fd=%d errno=%d\n",
			root_path, rootfd, errno);
	} else {
		dprintf(1,
			ERR_PREFIX " openat(AT_FDCWD, \"%s\", O_RDONLY|O_DIRECTORY) failed: fd=%d errno=%d (%s)\n",
			root_path, rootfd, errno, strerror(errno));
		return 1;
	}

	errno = 0;
	manifestfd = open(manifest_path, O_RDONLY, 0);
	if (manifestfd >= 0) {
		dprintf(1,
			OK_PREFIX " open(\"%s\", O_RDONLY) succeeded: fd=%d errno=%d\n",
			manifest_path, manifestfd, errno);
	} else {
		dprintf(1,
			ERR_PREFIX " open(\"%s\", O_RDONLY) failed: fd=%d errno=%d (%s)\n",
			manifest_path, manifestfd, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	memset(manifest, 0, sizeof(manifest));
	errno = 0;
	nread = read(manifestfd, manifest, sizeof(manifest) - 1);
	if (nread >= 0) {
		manifest[nread] = '\0';
		dprintf(1,
			OK_PREFIX " read(%d, %zu) manifest succeeded: rc=%zd errno=%d data=\"%s\"\n",
			manifestfd, sizeof(manifest) - 1, nread, errno, manifest);
	} else {
		dprintf(1, ERR_PREFIX " read(%d, %zu) manifest failed: rc=%zd errno=%d (%s)\n",
			manifestfd, sizeof(manifest) - 1, nread, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	rc = close(manifestfd);
	if (rc == 0) {
		dprintf(1, OK_PREFIX " close(%d) succeeded: rc=%d errno=%d\n",
			manifestfd, rc, errno);
		manifestfd = -1;
	} else {
		dprintf(1, ERR_PREFIX " close(%d) failed: rc=%d errno=%d (%s)\n",
			manifestfd, rc, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	for (entry = strtok_r(manifest, "\n", &saveptr);
	     entry;
	     entry = strtok_r(NULL, "\n", &saveptr)) {
		if (visit_entry(rootfd, entry, &checked_pread) != 0) {
			failed = 1;
			goto out;
		}
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = stat(link_path, &st);
	if (rc == 0 && S_ISREG(st.st_mode)) {
		dprintf(1,
			OK_PREFIX " stat(\"%s\") followed the tree symlink: rc=%d errno=%d mode=%o\n",
			link_path, rc, errno, st.st_mode);
	} else {
		dprintf(1,
			ERR_PREFIX " stat(\"%s\") failed or did not follow a regular file: rc=%d errno=%d mode=%o (%s)\n",
			link_path, rc, errno, st.st_mode, strerror(errno));
		failed = 1;
		goto out;
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = lstat(link_path, &st);
	if (rc == 0 && S_ISLNK(st.st_mode)) {
		dprintf(1,
			OK_PREFIX " lstat(\"%s\") preserved the tree symlink: rc=%d errno=%d mode=%o\n",
			link_path, rc, errno, st.st_mode);
	} else {
		dprintf(1,
			ERR_PREFIX " lstat(\"%s\") failed or did not return a symlink: rc=%d errno=%d mode=%o (%s)\n",
			link_path, rc, errno, st.st_mode, strerror(errno));
		failed = 1;
		goto out;
	}

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = stat(missing_path, &st);
	if (rc == -1 && errno == ENOENT) {
		dprintf(1,
			OK_PREFIX " stat(\"%s\") missing as expected: rc=%d errno=%d (%s)\n",
			missing_path, rc, errno, strerror(errno));
	} else {
		dprintf(1,
			ERR_PREFIX " stat(\"%s\") returned unexpected result: rc=%d errno=%d (%s)\n",
			missing_path, rc, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	errno = 0;
	leaf_fd = openat(rootfd, "pages/index.txt", O_RDONLY, 0);
	if (leaf_fd < 0) {
		dprintf(1,
			ERR_PREFIX " openat(%d, \"pages/index.txt\", O_RDONLY) failed: fd=%d errno=%d (%s)\n",
			rootfd, leaf_fd, errno, strerror(errno));
		failed = 1;
		goto out;
	}

	dprintf(1,
		OK_PREFIX " openat(%d, \"pages/index.txt\", O_RDONLY) succeeded: fd=%d errno=%d\n",
		rootfd, leaf_fd, errno);

	memset(&st, 0, sizeof(st));
	errno = 0;
	rc = fstatat(leaf_fd, regular_child, &st, 0);
	if (rc == -1 && errno == ENOTDIR) {
		dprintf(1,
			OK_PREFIX " fstatat(%d, \"%s\", 0) on regular-file dirfd correctly failed: rc=%d errno=%d (%s)\n",
			leaf_fd, regular_child, rc, errno, strerror(errno));
	} else {
		dprintf(1,
			ERR_PREFIX " fstatat(%d, \"%s\", 0) on regular-file dirfd returned unexpected result: rc=%d errno=%d (%s)\n",
			leaf_fd, regular_child, rc, errno, strerror(errno));
		failed = 1;
		goto out;
	}

out:
	if (leaf_fd >= 0) {
		errno = 0;
		rc = close(leaf_fd);
		if (rc == 0) {
			dprintf(1, OK_PREFIX " close(%d) succeeded: rc=%d errno=%d\n",
				leaf_fd, rc, errno);
		} else {
			dprintf(1, ERR_PREFIX " close(%d) failed: rc=%d errno=%d (%s)\n",
				leaf_fd, rc, errno, strerror(errno));
			failed = 1;
		}
	}

	if (manifestfd >= 0) {
		errno = 0;
		rc = close(manifestfd);
		if (rc == 0) {
			dprintf(1, OK_PREFIX " close(%d) succeeded: rc=%d errno=%d\n",
				manifestfd, rc, errno);
		} else {
			dprintf(1, ERR_PREFIX " close(%d) failed: rc=%d errno=%d (%s)\n",
				manifestfd, rc, errno, strerror(errno));
			failed = 1;
		}
	}

	if (rootfd >= 0) {
		errno = 0;
		rc = close(rootfd);
		if (rc == 0) {
			dprintf(1, OK_PREFIX " close(%d) succeeded: rc=%d errno=%d\n",
				rootfd, rc, errno);
		} else {
			dprintf(1, ERR_PREFIX " close(%d) failed: rc=%d errno=%d (%s)\n",
				rootfd, rc, errno, strerror(errno));
			failed = 1;
		}
	}

	return failed;
}
