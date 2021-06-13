#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **args) {
    int fd, count, i;
    off_t offset;
    char buf[100];

    fd = open("/dev/scull0", O_RDONLY);
    if (fd == -1) {
        printf("open failed! error: %s\n", strerror(errno));
        return -1;
    }
    count = read(fd, buf, 15);
    if (count == -1) {
        printf("read failed! error: %s\n", strerror(errno));
        return -1;
    }
    for (i = 0; i < 15; i++) {
        printf("buf[%d] = %c, value: %d\t", i, buf[i], buf[i]);
    }
    printf("\n");
    return 0;
}
