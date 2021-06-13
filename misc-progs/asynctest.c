/*
 * asynctest.c: use async notification to read stdin
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

int gotdata=0;
void sighandler(int signo)
{
    if (signo==SIGIO)
        gotdata++;
    return;
}

char buffer[4096];

int main(int argc, char **argv)
{
    int count, fd, result;
    struct sigaction action;
    fd_set fds;

    memset(&action, 0, sizeof(action));
    action.sa_handler = sighandler;
    action.sa_flags = 0;

    fd = open("/dev/scullpipe", O_RDONLY);
    if (fd == -1) {
        printf("open /dev/scullpipe failed. error: %s", strerror(errno));
        return -1;
    }

    sigaction(SIGIO, &action, NULL);

    /** subscribe to /dev/scullpipe */
    fcntl(fd, F_SETOWN, getpid());
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | FASYNC);

    /** subscribe to stdin */
    fcntl(STDIN_FILENO, F_SETOWN, getpid());
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | FASYNC);

    while(1) {
        /* this only returns if a signal arrives */
        sleep(86400); /* one day */
        if (!gotdata)
            continue;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        FD_SET(STDIN_FILENO, &fds);

        result = select(FD_SETSIZE, &fds, NULL, NULL, NULL);
        if (result == -1) {
            printf("select failed. error: %s\n", strerror(errno));
            return -1;
        }
        printf("select returns: %d, FD_ISSET(fd) = %d, "
                "FD_ISSET(STDIN_FILENO)=%d\n",
                result, FD_ISSET(fd, &fds), FD_ISSET(STDIN_FILENO, &fds));
        if (FD_ISSET(fd, &fds)) {
            count=read(fd, buffer, 4096);
        }
        if (FD_ISSET(STDIN_FILENO, &fds)) {
            count=read(STDIN_FILENO, buffer, 4096);
        }
        /* buggy: if avail data is more than 4kbytes... */
        write(1,buffer,count);
        gotdata=0;
    }
}
