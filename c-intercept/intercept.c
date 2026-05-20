#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    char buf[128];
    const char* dir_path = "/tmp";
    const char* file_path = "/tmp/aaa";
    const char* message = "written from example";
    int fd;
    int rc;
    struct stat st;
    ssize_t nread;
    ssize_t nwritten;

    (void)argc;
    (void)argv;

    errno = 0;
    rc = access(dir_path, F_OK);
    if (rc == 0) {
        dprintf(1, "access(\"%s\", F_OK) succeeded: rc=%d errno=%d\n", dir_path,
                rc, errno);
    } else {
        dprintf(1, "access(\"%s\", F_OK) failed: rc=%d errno=%d (%s)\n",
                dir_path, rc, errno, strerror(errno));
    }

    errno = 0;
    fd = openat(AT_FDCWD, file_path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd >= 0) {
        dprintf(1,
                "openat(AT_FDCWD, \"%s\", O_CREAT|O_TRUNC|O_WRONLY, 0644) "
                "succeeded: fd=%d errno=%d\n",
                file_path, fd, errno);
    } else {
        dprintf(1,
                "openat(AT_FDCWD, \"%s\", O_CREAT|O_TRUNC|O_WRONLY, 0644) "
                "failed: fd=%d errno=%d (%s)\n",
                file_path, fd, errno, strerror(errno));
        return 1;
    }

    errno = 0;
    nwritten = write(fd, message, strlen(message));
    if (nwritten >= 0) {
        dprintf(1, "write(%d, %zu) succeeded: rc=%zd errno=%d\n", fd,
                strlen(message), nwritten, errno);
    } else {
        dprintf(1, "write(%d, %zu) failed: rc=%zd errno=%d (%s)\n", fd,
                strlen(message), nwritten, errno, strerror(errno));
        return 1;
    }

    errno = 0;
    rc = close(fd);
    if (rc == 0) {
        dprintf(1, "close(%d) succeeded: rc=%d errno=%d\n", fd, rc, errno);
    } else {
        dprintf(1, "close(%d) failed: rc=%d errno=%d (%s)\n", fd, rc, errno,
                strerror(errno));
        return 1;
    }

    errno = 0;
    fd = openat(AT_FDCWD, file_path, O_RDONLY, 0);
    if (fd >= 0) {
        dprintf(1,
                "openat(AT_FDCWD, \"%s\", O_RDONLY) "
                "succeeded: fd=%d errno=%d\n",
                file_path, fd, errno);
    } else {
        dprintf(1,
                "openat(AT_FDCWD, \"%s\", O_RDONLY) "
                "failed: fd=%d errno=%d (%s)\n",
                file_path, fd, errno, strerror(errno));
        return 1;
    }

    memset(&st, 0, sizeof(st));
    errno = 0;
    rc = fstatat(AT_FDCWD, file_path, &st, 0);
    if (rc == 0) {
        dprintf(1,
                "fstatat(AT_FDCWD, \"%s\", 0) succeeded: rc=%d errno=%d mode=%o size=%lld nlink=%lu\n",
                file_path, rc, errno, st.st_mode, (long long)st.st_size,
                (unsigned long)st.st_nlink);
    } else {
        dprintf(1,
                "fstatat(AT_FDCWD, \"%s\", 0) failed: rc=%d errno=%d (%s)\n",
                file_path, rc, errno, strerror(errno));
        return 1;
    }

    memset(&st, 0, sizeof(st));
    errno = 0;
    rc = fstat(fd, &st);
    if (rc == 0) {
        dprintf(1,
                "fstat(%d) succeeded: rc=%d errno=%d mode=%o size=%lld nlink=%lu\n",
                fd, rc, errno, st.st_mode, (long long)st.st_size,
                (unsigned long)st.st_nlink);
    } else {
        dprintf(1, "fstat(%d) failed: rc=%d errno=%d (%s)\n", fd, rc, errno,
                strerror(errno));
        return 1;
    }

    memset(buf, 0, sizeof(buf));
    errno = 0;
    nread = read(fd, buf, sizeof(buf) - 1);
    if (nread >= 0) {
        buf[nread] = '\0';
        dprintf(1, "read(%d, %zu) succeeded: rc=%zd errno=%d data=\"%s\"\n", fd,
                sizeof(buf) - 1, nread, errno, buf);
    } else {
        dprintf(1, "read(%d, %zu) failed: rc=%zd errno=%d (%s)\n", fd,
                sizeof(buf) - 1, nread, errno, strerror(errno));
        return 1;
    }

    errno = 0;
    rc = close(fd);
    if (rc == 0) {
        dprintf(1, "close(%d) succeeded: rc=%d errno=%d\n", fd, rc, errno);
    } else {
        dprintf(1, "close(%d) failed: rc=%d errno=%d (%s)\n", fd, rc, errno,
                strerror(errno));
        return 1;
    }

    return 0;
}
