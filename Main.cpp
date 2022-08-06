#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>

void handleChildProcessOutput(char* buffer, int count) {
    if (strncmp("Target file", buffer, 11) == 0) {
        printf("-------- I got you ----- \n");
    }

    if (strncmp("Waiting for a connection from", buffer, 30) == 0) {
        pid_t pid = fork();
        if (pid == 0)
        {
            printf("RestartProcess: I am child, pid=%d,ppid=%d\n", getpid(), getppid());
            execl("/Users/zhaojialiang/MyScript/Restart", "Restart", (char*)NULL);
            perror("execl");
            printf("RestartProcess: end");
        }
    }
}

void parentProcess(int* childInputFD, int* childOutputFD)
{
    // Close the entrance of the pipe within the parent process
    printf("I am parent, pid=%d,ppid=%d\n", getpid(), getppid());
    close(childInputFD[0]);
    close(childOutputFD[1]);

    char buffer[4096];
    while (1)
    {
        ssize_t count = read(childOutputFD[0], buffer, sizeof(buffer));
        if (count == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                perror("read");
                exit(1);
            }
        }
        else if (count == 0)
        {
            break;
        }
        else
        {
            printf("ParentPrcessOutput: %.*s \n", (int)count, buffer);
            // handle_child_process_output(buffer, count);
            // if (buffer ) {
            // }
            handleChildProcessOutput(buffer, count);
        }
    }
    close(childOutputFD[0]);
    wait(0);
    // sleep(10);
}

void childProcess(int* childInputFD, int* childOutputFD) {
    /// Close the exit from the pipe within the child process
    close(childInputFD[1]);
    close(childOutputFD[0]);
    printf("I am child, pid=%d,ppid=%d\n", getpid(), getppid());

    while ((dup2(childInputFD[0], STDIN_FILENO) == -1) && (errno == EINTR))
    {
        printf("dup2 function error childInputFD");
    }

    while ((dup2(childOutputFD[1], STDOUT_FILENO) == -1) && (errno == EINTR))
    {
        printf("dup2 function error");
    }
    while ((dup2(childOutputFD[1], STDERR_FILENO) == -1) && (errno == EINTR))
    {
        printf("dup2 function error");
    }

    printf("Change FD: I am child, pid=%d,ppid=%d\n", getpid(), getppid());
    execl("/Users/zhaojialiang/.flutter_sdk/bin/flutter", "flutter", "attach", (char*)NULL);
    perror("execl");
    _exit(1);
}

int main()
{
    int childOutputFD[2];
    if (pipe(childOutputFD) == -1)
    {
        perror("pipe");
        exit(1);
    }

    int childInputFD[2];
    if (pipe(childInputFD) == -1)
    {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        childProcess(childInputFD, childOutputFD);
    }
    else if (pid > 0)
    {
        parentProcess(childInputFD, childOutputFD);
    }
    if (pid == -1)
    {
        perror("fork");
        exit(1);
    }
    return 1;
}
