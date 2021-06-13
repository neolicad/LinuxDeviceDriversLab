#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "scull.h"

void main() {
    int fd; 
    int quantum; 

    fd = open("/dev/scull0", O_RDONLY);
    quantum = ioctl(fd, SCULL_IOCQQUANTUM, 0);
    printf("quantum = %d\n", quantum);
}
