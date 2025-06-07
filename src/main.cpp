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
    
    if (input.substr(0, 5) == "type ") {
        std::string command = input.substr(5);
        if (command == "echo" || command == "exit" || command == "type") {
            std::cout << command << " is a shell builtin" << std::endl;
        } else {
            std::cout << command << ": not found" << std::endl;
        }
    } else if (input.substr(0, 5) == "echo ") {
        std::cout << input.substr(5) << std::endl;
    } else {
        std::cout << input << ": command not found" << std::endl;
    }
  }
}
