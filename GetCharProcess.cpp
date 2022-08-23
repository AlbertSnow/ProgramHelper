
#include <unistd.h>
#include <stdio.h>  
#include <iostream> 
using namespace std;


// get user input event not press enter key
void deliverInput(int inputFD) {
  cout << "++++++++++ User start handle ++++++++++" << endl;
  cout << "++++++++++ input q will exit ++++++++++" << endl;

  // Set terminal to raw mode 
  system("stty raw");

  while (1) {
    // Wait for single character
    char input = getchar();
    write(inputFD, &input, 1);
    if (input == 'q') {
        break;
    }
  }
  // Reset terminal to normal "cooked" mode
  system("stty cooked");
  exit(1);
}

void startProcess(int inputFD) {
  pid_t pid = fork();
  if (pid == 0)
  {
    deliverInput(inputFD);
  }
}