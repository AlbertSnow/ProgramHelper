#include <iostream> 
#include <stdio.h>  
using namespace std;  

// get user input event not press enter key
int main() { 
  // Output prompt 
  cout << "Press any key to continue..." << endl; 

  // Set terminal to raw mode 
  system("stty raw"); 

  while(1) {
    // Wait for single character
    char input = getchar();

    // Echo input:
    cout << "--" << input << "--";
    if (input == 'q') {
        break;
    }
  }
  // Reset terminal to normal "cooked" mode
  system("stty cooked"); 

  // And we're out of here 
  return 0; 
}