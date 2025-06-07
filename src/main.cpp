#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <fstream>
#include <windows.h>

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

string build_command_line(const vector<string>& args) {
    string cmd_line;
    for (const auto& arg : args) {
        if (cmd_line.empty()) {
            cmd_line = arg;
        } else {
            cmd_line += " " + arg;
        }
    }
    return cmd_line;
}

int main() {
    // Flush after every std::cout / std:cerr
    cout << unitbuf;
    cerr << unitbuf;

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

            // Create process
            STARTUPINFO si = {sizeof(si)};
            PROCESS_INFORMATION pi;
            string cmd_line = build_command_line(args);

            if (!CreateProcess(
                NULL,                   // No module name (use command line)
                (LPSTR)cmd_line.c_str(), // Command line
                NULL,                   // Process handle not inheritable
                NULL,                   // Thread handle not inheritable
                FALSE,                  // Set handle inheritance to FALSE
                0,                      // No creation flags
                NULL,                   // Use parent's environment block
                NULL,                   // Use parent's starting directory
                &si,                    // Pointer to STARTUPINFO structure
                &pi                     // Pointer to PROCESS_INFORMATION structure
            )) {
                cerr << "Failed to create process" << endl;
                continue;
            }

            // Wait for process to finish
            WaitForSingleObject(pi.hProcess, INFINITE);

            // Close process and thread handles
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
    return 0;
}
