#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scull.h"

int main(int argc, char **argv) {
    int fd; 
    int quantum; 

    if (argc != 1) {
        printf("Error: no arguments expected!\n");
        return -1;
    }

    fd = open("/dev/scull0", O_RDONLY);
    printf("about to get scull_quantum\n");
    if (ioctl(fd, SCULL_IOCGQUANTUM, &quantum) == -1) {
        printf("Failed, error: %s\n", strerror(errno));
        return -1;
    }

    printf("Succeeded! quantum = %d\n", quantum);
    return 0;
}
