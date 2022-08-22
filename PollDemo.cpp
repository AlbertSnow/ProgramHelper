#include <poll.h>
#include <stdio.h>
#include <unistd.h>

#define TIMEOUT 6000000 // 100 minutes
#define BUF_SIZE 512

int fd_can_read(int fd, int timeout) {
    struct pollfd pfd;

    pfd.fd = fd;
    pfd.events = POLLIN;

    if (poll(&pfd, 1, timeout)) {
        if (pfd.revents & POLLIN) {
            return 1;
        }
    }

    return 0;
}

int main(int argv, char **argc) {
    int fd;
    size_t bytes_read;
    char buffer[BUF_SIZE];

    fd = STDIN_FILENO;

    while (1) {
        if (fd_can_read(fd, TIMEOUT)) {
            printf("Can read\n");
            bytes_read = read(fd, buffer, sizeof(buffer));

            printf("Bytes read: %zu\n", bytes_read);
        }
        else {
            printf("Can't read\n");
        }
    }
}