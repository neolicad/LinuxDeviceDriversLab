#include <sys/select.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char ** args) {
    int fd, n_available, n;
    char buf[11];
    fd_set writefds;
    
    fd = open("/dev/scullpipe", O_WRONLY);
    if (fd == -1) {
        printf("open failed! error: %s\n", strerror(errno));
    }
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);

    n_available = select(FD_SETSIZE, NULL, &writefds, NULL, NULL);
    if (n_available == -1) {
        printf("select failed! error: %s\n", strerror(errno));
        return -1;
    }
    printf("select finished successfully!\n");

    strncpy(buf, "hi\n", 3);

    n = write(fd, buf, 3);
    if (n == -1) {
        printf("write failed! error: %s\n", strerror(errno));
        return -1;
    }
    printf("write succeeded!\n");
    return 0;
}
