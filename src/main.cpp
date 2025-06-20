#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cstring>

using namespace std;

std::vector<std::string> parseArgs(const std::string& input) {
    std::vector<std::string> args;
    std::string current;
    bool in_single_quote = false;
    bool in_double_quote = false;
    bool escaped = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        // Special handling for backslash inside double quotes
        if (in_double_quote && c == '\\') {
            if (i + 1 < input.size() && (input[i + 1] == '\\' || input[i + 1] == '$' || input[i + 1] == '"' || input[i + 1] == '\n')) {
                current += input[i + 1];
                ++i;
            } else {
                current += '\\';
            }
            continue;
        }
        
        if (escaped) {
            current += c;
            escaped = false;
            continue;
        }

        if (c == '\\') {
            if (in_single_quote) {
                // In single quotes, backslash is treated as a literal character
                current += c;
            } else if (in_double_quote) {
                // Already handled above
                // (This branch will not be reached)
            } else {
                // In unquoted context, escape next character
                escaped = true;
            }
        } else if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
        } else if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
        } else if (std::isspace(c) && !in_single_quote && !in_double_quote) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        args.push_back(current);
    }

    return args;
}

int main() {
    while (true) {
        cout << "$ ";
        string input;
        getline(cin, input);

        if (input == "exit 0") {
            exit(0);
        }

        vector<string> args = parseArgs(input);

        if (args.empty()) {
            continue;
        }

        string command = args[0];

        if (command == "echo") {
            for (size_t i = 1; i < args.size(); ++i) {
                cout << args[i];
                if (i < args.size() - 1) {
                    cout << " ";
                }
            }
            cout << endl;
        } else if (command == "type") {
            if (args.size() < 2) {
                cout << "type: missing argument" << endl;
                continue;
            }
            string target = args[1];
            if (target == "echo" || target == "exit" || target == "type" || target == "pwd") {
                cout << target << " is a shell builtin" << endl;
            } else {
                bool found = false;
                char* path = getenv("PATH");
                if (path != nullptr) {
                    string pathStr(path);
                    istringstream pathStream(pathStr);
                    string dir;
                    while (getline(pathStream, dir, ':')) {
                        string fullPath = dir + "/" + target;
                        if (access(fullPath.c_str(), X_OK) == 0) {
                            cout << target << " is " << fullPath << endl;
                            found = true;
                            break;
                        }
                    }
                }
                if (!found) {
                    cout << target << ": not found" << endl;
                }
            }
        } else if (command == "pwd") {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != nullptr) {
                cout << cwd << endl;
            } else {
                cout << "pwd: error getting current directory" << endl;
            }
        } else if (command == "cd") {
            if (args.size() < 2) {
                // No argument provided, do nothing for now (future stages may handle this)
                continue;
            }
            const std::string& dir = args[1];
            if (dir == "~") {
                char* home = getenv("HOME");
                if (home == nullptr) {
                    std::cout << "cd: HOME environment variable not set" << std::endl;
                    continue;
                }
                if (chdir(home) != 0) {
                    std::cout << "cd: " << home << ": No such file or directory" << std::endl;
                }
            } else {
                if (chdir(dir.c_str()) != 0) {
                    std::cout << "cd: " << dir << ": No such file or directory" << std::endl;
                }
            }
        } else {
            // Try to execute as external command
            char* path = getenv("PATH");
            if (path == nullptr) {
                cout << command << ": command not found" << endl;
                continue;
            }

            string pathStr(path);
            istringstream pathStream(pathStr);
            string dir;
            bool found = false;

            while (getline(pathStream, dir, ':')) {
                string fullPath = dir + "/" + command;
                if (access(fullPath.c_str(), X_OK) == 0) {
                    found = true;
                    pid_t pid = fork();
                    if (pid == 0) {
                        // Child process
                        vector<char*> execArgs;
                        execArgs.push_back(const_cast<char*>(command.c_str()));  // Use command name instead of full path
                        for (size_t i = 1; i < args.size(); ++i) {
                            execArgs.push_back(const_cast<char*>(args[i].c_str()));
                        }
                        execArgs.push_back(nullptr);
                        execv(fullPath.c_str(), execArgs.data());
                        cerr << "Error executing " << command << endl;
                        exit(1);
                    } else if (pid > 0) {
                        // Parent process
                        int status;
                        waitpid(pid, &status, 0);
                    }
                    break;
                }
            }

            if (!found) {
                cout << command << ": command not found" << endl;
            }
        }
    }

    return 0;
}
