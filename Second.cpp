#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>

char* PROGRAM_FLUTTER_PATH = "/Users/zhaojialiang/.flutter_sdk/bin/flutter";
char* PROGRAM_ADB_PATH = "/Users/zhaojialiang/Library/Android/sdk/platform-tools/adb";


int main()
{
    printf("test before sleep \n");
    sleep(9000);
    printf("test after sleep");
}
