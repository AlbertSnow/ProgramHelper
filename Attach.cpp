#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

#define APP_PACKAGE_NAME "com.sankuai.meituan.meituanwaimaibusiness"
#define APP_LAUNCH_ACTIVITY "com.sankuai.meituan.meituanwaimaibusiness.modules.main.MainActivity"

/// return: can read fd, -1 mean error;
int pollReadableFD(int* fdArray, int count, int timeout) {
    pollfd pollFDList[count];
    for (int i = 0; i<count; i++) {
        struct pollfd pullParentInput;
        pullParentInput.fd = fdArray[i];
        pullParentInput.events = POLLIN;
        pollFDList[i] = pullParentInput;
    }

    if (poll(pollFDList, count, timeout)) {
        for (int j = 0; j<count; j++) {
            if (pollFDList[j].revents != 0 && (pollFDList[j].revents & POLLIN)) {
                return j;
            }
        }
    } else {
        perror("poll");
        return -1;
    }
}

void deliverParentInputToChild(int* childInputFD, char* buffer, int bufferCount) {
    ssize_t count = read(STDIN_FILENO, buffer, bufferCount);
    if (count == -1) {
        if (errno != EINTR) {
            perror("read");
        }
    } else if (count != 0) {
        // printf("GET-------: %.*s \n", (int)count, buffer);
        write(childInputFD[1], buffer, count);
    }
}


/// parse child process (flutter) output, to execute new process
void parseChildLog2RunProgram(char* buffer, int count) {
    if (strncmp("Target file", buffer, 11) == 0) {
        printf("-------- This is not the root directory of the flutter project. ----- \n");
    }

    if (strncmp("Waiting for a connection from", buffer, 25) == 0) {
        printf("-------- Start adb program ----- \n");

        pid_t pid = fork();
        if (pid == 0)
        {
            printf("AdbProcess: I am child, pid=%d,ppid=%d\n", getpid(), getppid());
            // execl(PROGRAM_ADB_PATH, "adb", "devices", (char*)NULL);
            char* programName = "adb";
            char* args[] = { programName, "devices", NULL };
            execvp(programName, args);
            perror("execl");
            exit(1);
        }
        wait(NULL);

        pid_t stopAppPid = fork();
        if (stopAppPid == 0) {
            printf("StopAppProcess: I am child, pid=%d,ppid=%d\n", getpid(), getppid());
            // execl(PROGRAM_ADB_PATH, "adb", "shell", "am", "force-stop", "com.sankuai.meituan.meituanwaimaibusiness", (char*)NULL);
            char* programName = "adb";
            char* args[] = { programName, "shell", "am", "force-stop", APP_PACKAGE_NAME, NULL };
            execvp(programName, args);
            perror("execl");
            exit(1);
        }
        wait(NULL);
        printf("AdbProcess: stopApp finish \n");

        pid_t startAppPid = fork();
        if (startAppPid == 0) {
            printf("StartAppProcess: I am child, pid=%d,ppid=%d\n", getpid(), getppid());
            sleep(2);
            // execl(PROGRAM_ADB_PATH, "adb", "shell", "am", "start", "com.sankuai.meituan.meituanwaimaibusiness/com.sankuai.meituan.meituanwaimaibusiness.modules.main.MainActivity", (char*)NULL);
            char* programName = "adb";
            char* args[] = { programName, "shell", "am", "start", (APP_PACKAGE_NAME + "/" + APP_LAUNCH_ACTIVITY), NULL };
            execvp(programName, args);
            perror("execl");
            exit(1);
        }
        wait(NULL);
        printf("AdbProcess: startApp finish \n");
    }
}

/// parse child process output, and input some chars.
void parseChildLogToInputChars(int* childInputFD, char* displayBuffer) {
    if (strncmp("The Flutter DevTools debugger and profiler on", displayBuffer, 25) == 0) {
        printf("Start input R \n");
        write(childInputFD[1], "R", 1);
    }
}

//  count: log length
//  childInputFD: child Process input fd
//  buffer: child process log output 
void parseChildLog(int* childInputFD, int* childOutputFD, char* buffer, int readCount) {
    ssize_t count = read(childOutputFD[0], buffer, readCount);

    if (count == -1) {
        if (errno == EINTR) {
            return;
        }
        else {
            perror("read");
            exit(1);
        }
    }
    else if (count != 0) {
        // printf("ParentProcessCatch: %.*s \n", (int)count, buffer);
        if (buffer[count - 1] == '\n') {
            printf("%.*s", (int)count, buffer);
        }
        else {
            printf("%.*s \n", (int)count, buffer);
        }

        parseChildLog2RunProgram(buffer, count);
        parseChildLogToInputChars(childInputFD, buffer);
    }
}


/// infinit loop, to read child process output
void parentProcess(int* childInputFD, int* childOutputFD)
{
    // Close the entrance of the pipe within the parent process
    printf("Parent process, pid=%d,ppid=%d\n", getpid(), getppid());
    close(childInputFD[0]);
    close(childOutputFD[1]);

    int count = 2;
    int pollFDs[]  = {STDIN_FILENO, childOutputFD[0]};
    int infinitTimeout = -1;
    char buffer[4096];
    while (1)
    {
        int fdIndex = pollReadableFD(pollFDs, count, infinitTimeout);
        if (fdIndex == 0) { // parent process input something
            deliverParentInputToChild(childInputFD, buffer, sizeof(buffer));
        } else if (fdIndex == 1) { // child process output log
            parseChildLog(childInputFD, childOutputFD, buffer, sizeof(buffer));
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
    printf("Child Process, pid=%d,ppid=%d\n", getpid(), getppid());

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

    // execl(PROGRAM_FLUTTER_PATH, "flutter", "attach", (char*)NULL);
    char* programName = "flutter";
    char* args[] = { programName, "attach", NULL };
    execvp(programName, args);
    perror("execl");
    _exit(1);
}

int main(int argc, char* argv[])
{
    if (argc == 3) {
        printf("argc1: %s, argc2: %s", argv[0], argv[1]);
    }

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
