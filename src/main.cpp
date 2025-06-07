#include <iostream>
#include <string>
#include <fstream>
using namespace std;

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  while (true) {
    std::cout << "$ ";
    std::string input;
    std::getline(std::cin, input);
    
    if (input == "exit 0") {
        std::exit(0);
    }
    if(input.substr(0,5) == "type "){
      string s=input.substr(5);
      if(s=="invalid_command"){
        cout<<"invalid_command: not found"<<endl;
      }
      else{
        cout<<s<<" "<<"is a shell builtin"<<endl;
      }
    }


      

    if (input.substr(0, 5) == "echo ") {
        std::cout << input.substr(5) << std::endl;
    } else {
        std::cout << input << ": command not found" << std::endl;
    }
  }
}
