#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <poll.h>
#include "ConfigConstants.h"

#include <iostream> 
using namespace std;  

// Read user input, though not press enter key;
void deliverInput(int inputFD) {
//   cout << "++++++++++ User start handle ++++++++++" << endl;
  cout << "++++++++++ 输入 q 退出交互程序 ++++++++++" << endl;

  // Set terminal to raw mode 
  system("stty raw");

  while (1) {
    // Wait for single character
    char input = getchar();
    write(inputFD, &input, 1);
    if (input == 'q' || input == 'Q') {
        cout << "++++++++++ 输入 Ctrl+C 后将完全退出程序 ++++++++++" << endl;
        break;
    }
  }
  // Reset terminal to normal "cooked" mode
  system("stty cooked");
  exit(1);
}

// start new process to handle user input
// redirect to flutter process as standard input
void startDeliverInputProcess(int inputFD) {
  pid_t pid = fork();
  if (pid == 0)
  {
    deliverInput(inputFD);
  }
}


// third process handle user input to other child process (flutter)
void onUserStartControll(int childInputFD) {
    startDeliverInputProcess(childInputFD);
}

// return: can read fd, -1 mean error;
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

// read parent process std input deliver to child process
void deliverParentInputToChild(int* childInputPipeFDs, char* buffer, int bufferCount) {
    ssize_t count = read(STDIN_FILENO, buffer, bufferCount);
    if (count == -1) {
        if (errno != EINTR) {
            perror("read");
        }
    } else if (count != 0) {
        // printf("GET-------: %.*s \n", (int)count, buffer);
        write(childInputPipeFDs[1], buffer, count);
    }
}


// parse child process (flutter) output, to execute new process
void parseChildLog2RunProgram(char* buffer, int count) {
    if (strncmp("Target file", buffer, 11) == 0) {
        printf("-------- This is not the root directory of the flutter project. -------- \n");
    }

    if (strncmp("Waiting for a connection from", buffer, 25) == 0) {
        printf("\n-------- Start adb program -------- \n");

        // pid_t pid = fork();
        // if (pid == 0)
        // {
        //     printf("AdbProcess: I am child, pid=%d,ppid=%d\n", getpid(), getppid());
        //     // execl(PROGRAM_ADB_PATH, "adb", "devices", (char*)NULL);
        //     char* programName = "adb";
        //     char* args[] = { programName, "devices", NULL };
        //     execvp(programName, args);
        //     perror("execl");
        //     exit(1);
        // }
        // wait(NULL);

        pid_t stopAppPid = fork();
        if (stopAppPid == 0) {
            printf("StopAppProcess: I am child, pid=%d,ppid=%d\n", getpid(), getppid());
            // execl(PROGRAM_ADB_PATH, "adb", "shell", "am", "force-stop",  (char *) APP_PACKAGE_NAME, (char*)NULL);
            char* programName = "adb";
            char* args[] = { programName, "shell", "am", "force-stop", (char *) APP_PACKAGE_NAME, NULL };
            execvp(programName, args);
            perror("execl");
            exit(1);
        }
        wait(NULL);

        pid_t startAppPid = fork();
        if (startAppPid == 0) {
            printf("StartAppProcess: I am child, pid=%d,ppid=%d\n", getpid(), getppid());
            sleep(2);
            // execl(PROGRAM_ADB_PATH, "adb", "shell", "am", "start", (char *)APP_LAUNCH_ACTIVITY, (char*)NULL);
            char* programName = "adb";
            char* args[] = { programName, "shell", "am", "start", (char *)APP_LAUNCH_ACTIVITY, NULL };
            execvp(programName, args);
            perror("execl");
            exit(1);
        }
        wait(NULL);
        printf("-------- Finish adb program -------- \n\n");
        printf("-------- 第一次Sync会比较耗时，请耐心等待 -------- \n");
    }
}

// parse child process output, and input some chars.
void parseChildLogToInputChars(int* childInputPipeFDs, char* displayBuffer) {
    if (strncmp("The Flutter DevTools debugger and profiler on", displayBuffer, 25) == 0) {
        printf("Start input R \n");
        write(childInputPipeFDs[1], "R", 1);
        onUserStartControll(childInputPipeFDs[1]);
    }
}

//  count: log length
//  childInputPipeFDs: child Process input fd
//  buffer: child process log output 
void parseChildLog(int* childInputPipeFDs, int* childOutputPipeFDs, char* buffer, int readCount) {
    ssize_t count = read(childOutputPipeFDs[0], buffer, readCount);

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
            printf("%.*s\r\n", (int)(count-1), buffer);
        }
        else {
            printf("%.*s\r\n", (int)count, buffer);
        }

        parseChildLog2RunProgram(buffer, count);
        parseChildLogToInputChars(childInputPipeFDs, buffer);
    }
}


// infinit loop, 
// 1. read child process output, and auto trigger some program;
// 2. redirect parent process input to child process;
void parentProcess(int* childInputPipeFDs, int* childOutputPipeFDs)
{
    // Close the entrance of the pipe within the parent process
    printf("Parent process, pid=%d,ppid=%d\n", getpid(), getppid());
    close(childInputPipeFDs[0]);
    close(childOutputPipeFDs[1]);

    int count = 2;
    int pollFDs[]  = {STDIN_FILENO, childOutputPipeFDs[0]};
    int infinitTimeout = -1;
    char buffer[4096];
    while (1)
    {
        parseChildLog(childInputPipeFDs, childOutputPipeFDs, buffer, sizeof(buffer));

        // int fdIndex = pollReadableFD(pollFDs, count, infinitTimeout);
        // if (fdIndex == 0) { // parent process input something
        //     deliverParentInputToChild(childInputPipeFDs, buffer, sizeof(buffer));
        // } else if (fdIndex == 1) { // child process output log
        //     parseChildLog(childInputPipeFDs, childOutputPipeFDs, buffer, sizeof(buffer));
        // }
    }
    close(childOutputPipeFDs[0]);
    wait(0);
    // sleep(10);
}

// Child process for "flutter attach" program
// 1. deliver child process output to parent process by pipe
void childProcess(int* childInputPipeFDs, int* childOutputPipeFDs) {
    // Close the exit from the pipe within the child process
    close(childInputPipeFDs[1]);
    close(childOutputPipeFDs[0]);
    printf("Child Process, pid=%d,ppid=%d\n\n", getpid(), getppid());

    while ((dup2(childInputPipeFDs[0], STDIN_FILENO) == -1) && (errno == EINTR))
    {
        printf("dup2 function error childInputPipeFDs");
    }

    while ((dup2(childOutputPipeFDs[1], STDOUT_FILENO) == -1) && (errno == EINTR))
    {
        printf("dup2 function error");
    }
    while ((dup2(childOutputPipeFDs[1], STDERR_FILENO) == -1) && (errno == EINTR))
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
    int childOutputPipeFDs[2];
    if (pipe(childOutputPipeFDs) == -1)
    {
        perror("pipe");
        exit(1);
    }

    int childInputPipeFDs[2];
    if (pipe(childInputPipeFDs) == -1)
    {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        childProcess(childInputPipeFDs, childOutputPipeFDs);
    }
    else if (pid > 0)
    {
        parentProcess(childInputPipeFDs, childOutputPipeFDs);
    }
    if (pid == -1)
    {
        perror("fork");
        exit(1);
    }
    return 1;
}
