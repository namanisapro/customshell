#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <fstream>

std::vector<std::string> split_path(const std::string& path) {
    std::vector<std::string> directories;
    std::stringstream ss(path);
    std::string dir;
    while (std::getline(ss, dir, ':')) {
        directories.push_back(dir);
    }
    return directories;
}

bool is_executable(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

std::string find_command_in_path(const std::string& command) {
    const char* path_env = std::getenv("PATH");
    if (!path_env) return "";

    std::vector<std::string> path_dirs = split_path(path_env);
    for (const auto& dir : path_dirs) {
        std::string full_path = dir + "/" + command;
        if (is_executable(full_path)) {
            return full_path;
        }
    }
    return "";
}

int main() {
    // Flush after every std::cout / std:cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::string input;
    while (true) {
        std::cout << "$ ";
        std::getline(std::cin, input);

        if (input == "exit 0") {
            std::exit(0);
        } else if (input.substr(0, 5) == "echo ") {
            std::cout << input.substr(5) << std::endl;
        } else if (input.substr(0, 5) == "type ") {
            std::string command = input.substr(5);
            if (command == "echo" || command == "exit" || command == "type") {
                std::cout << command << " is a shell builtin" << std::endl;
            } else {
                std::string path = find_command_in_path(command);
                if (!path.empty()) {
                    std::cout << command << " is " << path << std::endl;
                } else {
                    std::cout << command << ": not found" << std::endl;
                }
            }
        } else {
            std::cout << input << ": command not found" << std::endl;
        }
    }
    return 0;
}
