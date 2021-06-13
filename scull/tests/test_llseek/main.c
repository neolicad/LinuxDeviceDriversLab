#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **args) {
    int fd, count;
    off_t offset;
    char buf[7] = "append";

    fd = open("/dev/scull0", O_RDWR);
    if (fd == -1) {
        printf("open failed! error: %s\n", strerror(errno));
        return -1;
    }
    offset = lseek(fd, 4, SEEK_END);
    if (offset == -1) {
        printf("lseek failed! error: %s\n", strerror(errno));
        return -1;
    }
    count = write(fd, buf, 7);
    if (count == -1) {
        printf("write failed! error: %s\n", strerror(errno));
        return -1;
    }
    printf("write succeeded! count = %d\n", count);
    return 0;
}
