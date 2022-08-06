#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>

char* PROGRAM_FLUTTER_PATH = "/Users/zhaojialiang/.flutter_sdk/bin/flutter";
char* PROGRAM_ADB_PATH = "/Users/zhaojialiang/Library/Android/sdk/platform-tools/adb";

void handleChildProcessOutput(char* buffer, int count) {
    if (strncmp("Target file", buffer, 11) == 0) {
        printf("-------- I got you ----- \n");
    }

    if (strncmp("Waiting for a connection from", buffer, 25) == 0) {
        printf("-------- Start restart your app ----- \n");

        pid_t pid = fork();
        if (pid == 0)
        {
            printf("AdbProcess: I am child, pid=%d,ppid=%d\n", getpid(), getppid());
            execl(PROGRAM_ADB_PATH, "adb", "devices", (char*)NULL);
            perror("execl");
            exit(1);
        }
        wait(NULL);

        pid_t stopAppPid = fork();
        if (stopAppPid == 0) {
            printf("StopAppProcess: I am child, pid=%d,ppid=%d\n", getpid(), getppid());
            execl(PROGRAM_ADB_PATH, "adb", "shell", "am", "force-stop", "com.sankuai.meituan.meituanwaimaibusiness", (char*)NULL);
            perror("execl");
            exit(1);
        }
        wait(NULL);
        printf("AdbProcess: stopApp finish \n");

        pid_t startAppPid = fork();
        if (startAppPid == 0) {
            printf("StartAppProcess: I am child, pid=%d,ppid=%d\n", getpid(), getppid());
            sleep(2);
            execl(PROGRAM_ADB_PATH, "adb", "shell", "am", "start", "com.sankuai.meituan.meituanwaimaibusiness/com.sankuai.meituan.meituanwaimaibusiness.modules.main.MainActivity", (char*)NULL);
            perror("execl");
            exit(1);
        }
        wait(NULL);
        printf("AdbProcess: startApp finish \n");

        printf("AdbProcess: end \n");
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
            handleChildProcessOutput(buffer, count);
            if (strncmp("The Flutter DevTools debugger and profiler on", buffer, 25) == 0) {
                printf("Start input R \n");
                write(childInputFD[1], "R", 1);
            }
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
    execl(PROGRAM_FLUTTER_PATH, "flutter", "attach", (char*)NULL);
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
