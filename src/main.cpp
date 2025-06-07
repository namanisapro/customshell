#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

vector<string> split_path(const string& path) {
    vector<string> directories;
    stringstream ss(path);
    string dir;
    while (getline(ss, dir, ':')) {
        directories.push_back(dir);
    }
    return directories;
}

bool is_executable(const string& path) {
    ifstream file(path);
    return file.good();
}

string find_command_in_path(const string& command) {
    const char* path_env = getenv("PATH");
    if (!path_env) return "";

    vector<string> path_dirs = split_path(path_env);
    for (const auto& dir : path_dirs) {
        string full_path = dir + "/" + command;
        if (is_executable(full_path)) {
            return full_path;
        }
    }
    return "";
}

vector<string> split_command(const string& input) {
    vector<string> args;
    stringstream ss(input);
    string arg;
    while (ss >> arg) {
        args.push_back(arg);
    }
    return args;
}

int main() {
    string input;
    while (true) {
        cout << "$ ";
        getline(cin, input);

        if (input == "exit 0") {
            exit(0);
        } else if (input.substr(0, 5) == "echo ") {
            cout << input.substr(5) << endl;
        } else if (input.substr(0, 5) == "type ") {
            string command = input.substr(5);
            if (command == "echo" || command == "exit" || command == "type") {
                cout << command << " is a shell builtin" << endl;
            } else {
                string path = find_command_in_path(command);
                if (!path.empty()) {
                    cout << command << " is " << path << endl;
                } else {
                    cout << command << ": not found" << endl;
                }
            }
        } else {
            // Handle external commands
            vector<string> args = split_command(input);
            if (args.empty()) continue;

            string command_path = find_command_in_path(args[0]);
            if (command_path.empty()) {
                cout << args[0] << ": command not found" << endl;
                continue;
            }

            // Convert args to C-style array for execv
            vector<char*> c_args;
            for (const auto& arg : args) {
                c_args.push_back(const_cast<char*>(arg.c_str()));
            }
            c_args.push_back(nullptr);

            pid_t pid = fork();
            if (pid < 0) {
                cerr << "Failed to fork" << endl;
            } else if (pid == 0) {
                // Child process
                execv(command_path.c_str(), c_args.data());
                exit(1);  // Only reached if execv fails
            } else {
                // Parent process
                int status;
                waitpid(pid, &status, 0);
            }
        }
    }
    return 0;
}
