#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int fd; 
    char buf[1000];
    int n;

    fd = open("/dev/scullpipe", O_RDONLY);
    n = read(fd, buf, 10); 
    if (n < 0) {
        printf("Read failed!\n");
        return -1;
    }
    printf("Read succeeded! buf=%s\n", buf);
    return 0;
}
