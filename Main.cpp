#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>

void parentProcess(int *filedes)
{
    // Close the entrance of the pipe within the parent process
    printf("I am parent, pid=%d,ppid=%d\n", getpid(), getppid());
    close(filedes[1]);
    char buffer[4096];
    while (1)
    {
        ssize_t count = read(filedes[0], buffer, sizeof(buffer));
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
            printf("Parent outpu: %s", buffer);
            // handle_child_process_output(buffer, count);
        }
    }
    close(filedes[0]);
    wait(0);
}

int main()
{
    int filedes[2];
    if (pipe(filedes) == -1)
    {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        printf("I am child, pid=%d,ppid=%d\n", getpid(), getppid());

        while ((dup2(filedes[1], STDOUT_FILENO) == -1) && (errno == EINTR))
        {
            printf("dup2 function error");
        }

        /// Close the exit from the pipe within the child process
        close(filedes[0]);

        execl("/Users/zhaojialiang/.flutter_sdk/bin/flutter", "flutter", "attach", (char *)NULL);
        perror("execl");
        _exit(1);
    }
    else if (pid > 0)
    {
        parentProcess(filedes);
    }
    if (pid == -1)
    {
        perror("fork");
        exit(1);
    }
    return 0;
}
