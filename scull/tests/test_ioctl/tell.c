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

    if (argc != 2) {
        printf("Error: one and only one argument expected!");
        return -1;
    }

    quantum = atoi(argv[1]);

    fd = open("/dev/scull0", O_RDONLY);
    printf("about to tell scull_quantum to %d\n", quantum);
    if (ioctl(fd, SCULL_IOCTQUANTUM, quantum) == -1) {
        printf("Failed, error: %s\n", strerror(errno));
        return -1;
    }
    printf("Succeeded!\n");
    return 0;
}
