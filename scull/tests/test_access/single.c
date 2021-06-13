#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void handler(int signum) {
    // Do nothing
}

int main(int argc, char **argv) {
    int fd, result;
    struct sigaction action;

    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = handler;
    sigaction(SIGINT, &action, NULL);

    fd = open("/dev/scullsingle", O_RDONLY);
    if (fd == -1) {
        printf("open failed! error: %s\n", strerror(errno));
        return -1;
    }

    printf("open /dev/scullsingle succeeded!\n");
    sleep(85400); // 1 day
    printf("\nwoken up!\n");

    result = close(fd);
    if (result == -1) {
        printf("close /dev/scullsingle failed! error: %s\n", strerror(errno));
    }
    printf("close /dev/scullsingle succeeded!\n");

    return 0;
}
