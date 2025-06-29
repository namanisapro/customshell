#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cstring>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <fstream>
#include <map>

using namespace std;

// Builtin commands for autocompletion
const vector<string> BUILTIN_COMMANDS = {"echo", "exit", "type", "pwd", "cd", "history"};

// Forward declarations
void executeCommand(const std::vector<std::string>& args);

// Autocompletion function for readline
char* builtin_completion(const char* text, int state) {
    static size_t list_index, len;
    static vector<string> all_commands;
    const char* name;
    
    if (!state) {
        list_index = 0;
        len = strlen(text);
        all_commands.clear();
        
        // Add builtin commands
        for (const auto& cmd : BUILTIN_COMMANDS) {
            if (strncmp(cmd.c_str(), text, len) == 0) {
                all_commands.push_back(cmd);
            }
        }
        
        // Add external executables from PATH
        char* path = getenv("PATH");
        if (path != nullptr) {
            string pathStr(path);
            istringstream pathStream(pathStr);
            string dir;
            
            while (getline(pathStream, dir, ':')) {
                DIR* dirp = opendir(dir.c_str());
                if (dirp != nullptr) {
                    struct dirent* entry;
                    while ((entry = readdir(dirp)) != nullptr) {
                        string filename(entry->d_name);
                        if (strncmp(filename.c_str(), text, len) == 0) {
                            string fullPath = dir + "/" + filename;
                            if (access(fullPath.c_str(), X_OK) == 0) {
                                // Check if this executable is not already in the list
                                bool found = false;
                                for (const auto& existing : all_commands) {
                                    if (existing == filename) {
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found) {
                                    all_commands.push_back(filename);
                                }
                            }
                        }
                    }
                    closedir(dirp);
                }
            }
        }
    }
    
    if (list_index < all_commands.size()) {
        name = all_commands[list_index].c_str();
        list_index++;
        
        // Return a copy of the completed command without extra space
        char* result = new char[strlen(name) + 1]; // +1 for null terminator
        strcpy(result, name);
        return result;
    }
    
    return nullptr;
}

// Function to generate completions
char** builtin_completion_generator(const char* text, int start, int end) {
    char** matches = nullptr;
    
    if (start == 0) {
        matches = rl_completion_matches(text, builtin_completion);
    }
    
    return matches;
}

struct ParsedCommand {
    std::vector<std::string> args;
    std::string output_file;
    std::string error_file;
    bool has_redirection;
    bool has_stderr_redirection;
    bool has_append_redirection;
    bool has_stderr_append_redirection;
    bool is_pipeline;
    std::vector<std::vector<std::string>> pipeline_commands;
};

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

ParsedCommand parseCommandWithRedirection(const std::string& input) {
    ParsedCommand result;
    result.has_redirection = false;
    result.has_stderr_redirection = false;
    result.has_append_redirection = false;
    result.has_stderr_append_redirection = false;
    result.is_pipeline = false;
    
    std::vector<std::string> args = parseArgs(input);
    
    // Check for pipeline first
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "|") {
            result.is_pipeline = true;
            
            // Split into pipeline commands
            std::vector<std::string> current_command;
            for (size_t j = 0; j < i; ++j) {
                current_command.push_back(args[j]);
            }
            if (!current_command.empty()) {
                result.pipeline_commands.push_back(current_command);
            }
            
            current_command.clear();
            for (size_t j = i + 1; j < args.size(); ++j) {
                if (args[j] == "|") {
                    if (!current_command.empty()) {
                        result.pipeline_commands.push_back(current_command);
                        current_command.clear();
                    }
                } else {
                    current_command.push_back(args[j]);
                }
            }
            if (!current_command.empty()) {
                result.pipeline_commands.push_back(current_command);
            }
            
            return result;
        }
    }
    
    // No pipeline, check for redirection
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == ">" || args[i] == "1>") {
            if (i + 1 < args.size()) {
                result.output_file = args[i + 1];
                result.has_redirection = true;
                // Add all arguments before the redirection operator
                for (size_t j = 0; j < i; ++j) {
                    result.args.push_back(args[j]);
                }
                return result;
            }
        } else if (args[i] == "2>" || args[i] == "2>&1") {
            if (i + 1 < args.size()) {
                result.error_file = args[i + 1];
                result.has_stderr_redirection = true;
                // Add all arguments before the redirection operator
                for (size_t j = 0; j < i; ++j) {
                    result.args.push_back(args[j]);
                }
                return result;
            }
        } else if (args[i] == "2>>") {
            if (i + 1 < args.size()) {
                result.error_file = args[i + 1];
                result.has_stderr_append_redirection = true;
                // Add all arguments before the redirection operator
                for (size_t j = 0; j < i; ++j) {
                    result.args.push_back(args[j]);
                }
                return result;
            }
        } else if (args[i] == ">>" || args[i] == "1>>") {
            if (i + 1 < args.size()) {
                result.output_file = args[i + 1];
                result.has_append_redirection = true;
                // Add all arguments before the redirection operator
                for (size_t j = 0; j < i; ++j) {
                    result.args.push_back(args[j]);
                }
                return result;
            }
        }
    }
    
    // No redirection found, return all arguments
    result.args = args;
    return result;
}

void handleRedirection(const ParsedCommand& command) {
    if (command.has_redirection) {
        int fd = open(command.output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            cerr << "Error opening file for redirection: " << command.output_file << endl;
            exit(1);
        }
        // Redirect stdout to the file
        if (dup2(fd, STDOUT_FILENO) == -1) {
            cerr << "Error redirecting stdout" << endl;
            exit(1);
        }
        close(fd);
    }
    
    if (command.has_append_redirection) {
        int fd = open(command.output_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            cerr << "Error opening file for append redirection: " << command.output_file << endl;
            exit(1);
        }
        // Redirect stdout to the file
        if (dup2(fd, STDOUT_FILENO) == -1) {
            cerr << "Error redirecting stdout" << endl;
            exit(1);
        }
        close(fd);
    }
    
    if (command.has_stderr_redirection) {
        int fd = open(command.error_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            cerr << "Error opening file for stderr redirection: " << command.error_file << endl;
            exit(1);
        }
        // Redirect stderr to the file
        if (dup2(fd, STDERR_FILENO) == -1) {
            cerr << "Error redirecting stderr" << endl;
            exit(1);
        }
        close(fd);
    }
    
    if (command.has_stderr_append_redirection) {
        int fd = open(command.error_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            cerr << "Error opening file for stderr append redirection: " << command.error_file << endl;
            exit(1);
        }
        // Redirect stderr to the file
        if (dup2(fd, STDERR_FILENO) == -1) {
            cerr << "Error redirecting stderr" << endl;
            exit(1);
        }
        close(fd);
    }
}

struct RedirectionState {
    int original_stdout;
    int original_stderr;
    bool has_stdout_redirection;
    bool has_stderr_redirection;
    bool has_append_redirection;
    bool has_stderr_append_redirection;
    
    RedirectionState() : original_stdout(-1), original_stderr(-1), 
                        has_stdout_redirection(false), has_stderr_redirection(false),
                        has_append_redirection(false), has_stderr_append_redirection(false) {}
};

RedirectionState handleBuiltinRedirection(const ParsedCommand& command) {
    RedirectionState state;
    
    if (command.has_redirection) {
        state.has_stdout_redirection = true;
        // Save original stdout
        state.original_stdout = dup(STDOUT_FILENO);
        if (state.original_stdout == -1) {
            cerr << "Error saving stdout" << endl;
            return state;
        }
        
        int fd = open(command.output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            cerr << "Error opening file for redirection: " << command.output_file << endl;
            close(state.original_stdout);
            state.original_stdout = -1;
            return state;
        }
        // Redirect stdout to the file
        if (dup2(fd, STDOUT_FILENO) == -1) {
            cerr << "Error redirecting stdout" << endl;
            close(fd);
            close(state.original_stdout);
            state.original_stdout = -1;
            return state;
        }
        close(fd);
    }
    
    if (command.has_append_redirection) {
        state.has_append_redirection = true;
        // Save original stdout
        state.original_stdout = dup(STDOUT_FILENO);
        if (state.original_stdout == -1) {
            cerr << "Error saving stdout" << endl;
            return state;
        }
        
        int fd = open(command.output_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            cerr << "Error opening file for append redirection: " << command.output_file << endl;
            close(state.original_stdout);
            state.original_stdout = -1;
            return state;
        }
        // Redirect stdout to the file
        if (dup2(fd, STDOUT_FILENO) == -1) {
            cerr << "Error redirecting stdout" << endl;
            close(fd);
            close(state.original_stdout);
            state.original_stdout = -1;
            return state;
        }
        close(fd);
    }
    
    if (command.has_stderr_redirection) {
        state.has_stderr_redirection = true;
        // Save original stderr
        state.original_stderr = dup(STDERR_FILENO);
        if (state.original_stderr == -1) {
            cerr << "Error saving stderr" << endl;
            if (state.original_stdout != -1) {
                close(state.original_stdout);
                state.original_stdout = -1;
            }
            return state;
        }
        
        int fd = open(command.error_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            cerr << "Error opening file for stderr redirection: " << command.error_file << endl;
            if (state.original_stdout != -1) {
                close(state.original_stdout);
                state.original_stdout = -1;
            }
            close(state.original_stderr);
            state.original_stderr = -1;
            return state;
        }
        // Redirect stderr to the file
        if (dup2(fd, STDERR_FILENO) == -1) {
            cerr << "Error redirecting stderr" << endl;
            close(fd);
            if (state.original_stdout != -1) {
                close(state.original_stdout);
                state.original_stdout = -1;
            }
            close(state.original_stderr);
            state.original_stderr = -1;
            return state;
        }
        close(fd);
    }
    
    if (command.has_stderr_append_redirection) {
        state.has_stderr_append_redirection = true;
        // Save original stderr
        state.original_stderr = dup(STDERR_FILENO);
        if (state.original_stderr == -1) {
            cerr << "Error saving stderr" << endl;
            if (state.original_stdout != -1) {
                close(state.original_stdout);
                state.original_stdout = -1;
            }
            return state;
        }
        
        int fd = open(command.error_file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            cerr << "Error opening file for stderr append redirection: " << command.error_file << endl;
            if (state.original_stdout != -1) {
                close(state.original_stdout);
                state.original_stdout = -1;
            }
            close(state.original_stderr);
            state.original_stderr = -1;
            return state;
        }
        // Redirect stderr to the file
        if (dup2(fd, STDERR_FILENO) == -1) {
            cerr << "Error redirecting stderr" << endl;
            close(fd);
            if (state.original_stdout != -1) {
                close(state.original_stdout);
                state.original_stdout = -1;
            }
            close(state.original_stderr);
            state.original_stderr = -1;
            return state;
        }
        close(fd);
    }
    
    return state;
}

void restoreRedirection(const RedirectionState& state) {
    if ((state.has_stdout_redirection || state.has_append_redirection) && state.original_stdout != -1) {
        dup2(state.original_stdout, STDOUT_FILENO);
        close(state.original_stdout);
    }
    if ((state.has_stderr_redirection || state.has_stderr_append_redirection) && state.original_stderr != -1) {
        dup2(state.original_stderr, STDERR_FILENO);
        close(state.original_stderr);
    }
}

void executePipeline(const ParsedCommand& command) {
    int n = command.pipeline_commands.size();
    if (n < 2) {
        cerr << "Pipeline must have at least 2 commands" << endl;
        return;
    }
    
    // Create pipes for n-1 connections
    std::vector<std::array<int, 2>> pipes(n - 1);
    for (int i = 0; i < n - 1; ++i) {
        if (pipe(pipes[i].data()) == -1) {
            cerr << "Error creating pipe" << endl;
            return;
        }
    }
    
    std::vector<pid_t> pids;
    
    // Fork a process for each command
    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            
            // Set up stdin (read from previous pipe or shell stdin)
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    cerr << "Error redirecting stdin in pipeline" << endl;
                    exit(1);
                }
            }
            
            // Set up stdout (write to next pipe or shell stdout)
            if (i < n - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    cerr << "Error redirecting stdout in pipeline" << endl;
                    exit(1);
                }
            }
            
            // Close all pipe file descriptors in child
            for (int j = 0; j < n - 1; ++j) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute the command
            executeCommand(command.pipeline_commands[i]);
            exit(1); // Should not reach here
        } else if (pid < 0) {
            cerr << "Error forking process " << i << endl;
            // Clean up pipes and kill any already forked processes
            for (int j = 0; j < n - 1; ++j) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            for (pid_t child_pid : pids) {
                kill(child_pid, SIGTERM);
                waitpid(child_pid, nullptr, 0);
            }
            return;
        } else {
            pids.push_back(pid);
        }
    }
    
    // Parent process - close all pipe file descriptors
    for (int i = 0; i < n - 1; ++i) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all children to complete
    for (pid_t pid : pids) {
        int status;
        waitpid(pid, &status, 0);
    }
}

void executeCommand(const std::vector<std::string>& args) {
    if (args.empty()) {
        return;
    }
    
    string command_str = args[0];
    
    // Check if it's a builtin command
    if (command_str == "echo") {
        for (size_t i = 1; i < args.size(); ++i) {
            cout << args[i];
            if (i < args.size() - 1) {
                cout << " ";
            }
        }
        cout << endl;
        exit(0);
    } else if (command_str == "type") {
        if (args.size() < 2) {
            cout << "type: missing argument" << endl;
            exit(1);
        }
        string target = args[1];
        if (target == "echo" || target == "exit" || target == "type" || target == "pwd" || target == "history") {
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
        exit(0);
    } else if (command_str == "pwd") {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            cout << cwd << endl;
        } else {
            cout << "pwd: error getting current directory" << endl;
        }
        exit(0);
    } else if (command_str == "history") {
        exit(0);
    }
    
    // Try to execute as external command
    char* path = getenv("PATH");
    if (path == nullptr) {
        cerr << command_str << ": command not found" << endl;
        exit(1);
    }

    string pathStr(path);
    istringstream pathStream(pathStr);
    string dir;
    bool found = false;

    while (getline(pathStream, dir, ':')) {
        string fullPath = dir + "/" + command_str;
        if (access(fullPath.c_str(), X_OK) == 0) {
            found = true;
            
            vector<char*> execArgs;
            for (size_t i = 0; i < args.size(); ++i) {
                execArgs.push_back(const_cast<char*>(args[i].c_str()));
            }
            execArgs.push_back(nullptr);
            
            execv(fullPath.c_str(), execArgs.data());
            cerr << "Error executing " << command_str << endl;
            exit(1);
        }
    }

    if (!found) {
        cerr << command_str << ": command not found" << endl;
        exit(1);
    }
}

int main() {
    // Set up readline autocompletion
    rl_attempted_completion_function = builtin_completion_generator;
    
    // Load history from HISTFILE environment variable if set
    char* histfile = getenv("HISTFILE");
    if (histfile != nullptr) {
        read_history(histfile);
    }
    
    while (true) {
        char* input_line = readline("$ ");
        
        if (input_line == nullptr) {
            // EOF or Ctrl+D
            break;
        }
        
        string input(input_line);
        free(input_line); // Free the memory allocated by readline
        
        if (input.empty()) {
            continue;
        }
        
        // Add to history if not empty
        add_history(input.c_str());
        
        if (input == "exit 0") {
            // Save history to HISTFILE before exiting
            char* histfile = getenv("HISTFILE");
            if (histfile != nullptr) {
                write_history(histfile);
            }
            exit(0);
        }

        ParsedCommand command = parseCommandWithRedirection(input);

        if (command.args.empty() && command.pipeline_commands.empty()) {
            continue;
        }

        // Handle pipelines
        if (command.is_pipeline) {
            executePipeline(command);
            continue;
        }

        string command_str = command.args[0];

        if (command_str == "echo") {
            // Handle redirection for built-in commands
            RedirectionState state = handleBuiltinRedirection(command);
            // Check if there was an error during redirection setup
            if ((command.has_redirection && state.original_stdout == -1) || 
                (command.has_append_redirection && state.original_stdout == -1) ||
                (command.has_stderr_redirection && state.original_stderr == -1) ||
                (command.has_stderr_append_redirection && state.original_stderr == -1)) {
                continue;
            }
            
            for (size_t i = 1; i < command.args.size(); ++i) {
                cout << command.args[i];
                if (i < command.args.size() - 1) {
                    cout << " ";
                }
            }
            cout << endl;
            restoreRedirection(state);
        } else if (command_str == "type") {
            // Handle redirection for built-in commands
            RedirectionState state = handleBuiltinRedirection(command);
            // Check if there was an error during redirection setup
            if ((command.has_redirection && state.original_stdout == -1) || 
                (command.has_append_redirection && state.original_stdout == -1) ||
                (command.has_stderr_redirection && state.original_stderr == -1) ||
                (command.has_stderr_append_redirection && state.original_stderr == -1)) {
                continue;
            }
            
            if (command.args.size() < 2) {
                cout << "type: missing argument" << endl;
                restoreRedirection(state);
                continue;
            }
            string target = command.args[1];
            if (target == "echo" || target == "exit" || target == "type" || target == "pwd" || target == "history") {
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
            restoreRedirection(state);
        } else if (command_str == "pwd") {
            // Handle redirection for built-in commands
            RedirectionState state = handleBuiltinRedirection(command);
            // Check if there was an error during redirection setup
            if ((command.has_redirection && state.original_stdout == -1) || 
                (command.has_append_redirection && state.original_stdout == -1) ||
                (command.has_stderr_redirection && state.original_stderr == -1) ||
                (command.has_stderr_append_redirection && state.original_stderr == -1)) {
                continue;
            }
            
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != nullptr) {
                cout << cwd << endl;
            } else {
                cout << "pwd: error getting current directory" << endl;
            }
            restoreRedirection(state);
        } else if (command_str == "history") {
            // Handle redirection for built-in commands
            RedirectionState state = handleBuiltinRedirection(command);
            // Check if there was an error during redirection setup
            if ((command.has_redirection && state.original_stdout == -1) || 
                (command.has_append_redirection && state.original_stdout == -1) ||
                (command.has_stderr_redirection && state.original_stderr == -1) ||
                (command.has_stderr_append_redirection && state.original_stderr == -1)) {
                continue;
            }
            
            // Static variable to track last appended history index
            static std::map<std::string, int> last_appended_index;
            
            // Check for history -a <file> command
            if (command.args.size() > 2 && command.args[1] == "-a") {
                string filename = command.args[2];
                ofstream file(filename, ios::app);
                if (file.is_open()) {
                    HIST_ENTRY **the_list = history_list();
                    int start = 0;
                    if (last_appended_index.count(filename)) {
                        start = last_appended_index[filename];
                    }
                    int total_entries = 0;
                    while (the_list && the_list[total_entries]) {
                        total_entries++;
                    }
                    for (int i = start; i < total_entries; ++i) {
                        file << the_list[i]->line << endl;
                    }
                    file.close();
                    last_appended_index[filename] = total_entries;
                }
                restoreRedirection(state);
                continue;
            }
            
            // Check for history -r <file> command
            if (command.args.size() > 2 && command.args[1] == "-r") {
                string filename = command.args[2];
                ifstream file(filename);
                if (file.is_open()) {
                    string line;
                    while (getline(file, line)) {
                        // Skip empty lines
                        if (!line.empty()) {
                            add_history(line.c_str());
                        }
                    }
                    file.close();
                }
                restoreRedirection(state);
                continue;
            }
            
            // Check for history -w <file> command
            if (command.args.size() > 2 && command.args[1] == "-w") {
                string filename = command.args[2];
                ofstream file(filename);
                if (file.is_open()) {
                    HIST_ENTRY **the_list = history_list();
                    if (the_list) {
                        for (int i = 0; the_list[i]; ++i) {
                            file << the_list[i]->line << endl;
                        }
                    }
                    file.close();
                }
                restoreRedirection(state);
                continue;
            }
            
            HIST_ENTRY **the_list = history_list();
            if (the_list) {
                int total_entries = 0;
                while (the_list[total_entries]) {
                    total_entries++;
                }
                
                // Check if a number argument is provided
                int limit = total_entries; // Default to showing all entries
                if (command.args.size() > 1) {
                    try {
                        limit = stoi(command.args[1]);
                        if (limit < 0) {
                            limit = total_entries; // If negative, show all
                        }
                    } catch (const std::exception&) {
                        limit = total_entries; // If invalid number, show all
                    }
                }
                
                // Calculate starting index to show last 'limit' entries
                int start_index = total_entries - limit;
                if (start_index < 0) start_index = 0;
                
                for (int i = start_index; i < total_entries; ++i) {
                    cout << "    " << (i + 1) << "  " << the_list[i]->line << endl;
                }
            }
            restoreRedirection(state);
        } else if (command_str == "cd") {
            if (command.args.size() < 2) {
                // No argument provided, do nothing for now (future stages may handle this)
                continue;
            }
            const std::string& dir = command.args[1];
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
                cout << command_str << ": command not found" << endl;
                continue;
            }

            string pathStr(path);
            istringstream pathStream(pathStr);
            string dir;
            bool found = false;

            while (getline(pathStream, dir, ':')) {
                string fullPath = dir + "/" + command_str;
                if (access(fullPath.c_str(), X_OK) == 0) {
                    found = true;
                    pid_t pid = fork();
                    if (pid == 0) {
                        // Child process
                        
                        // Handle output redirection if specified
                        handleRedirection(command);
                        
                        vector<char*> execArgs;
                        execArgs.push_back(const_cast<char*>(command_str.c_str()));  // Use command name instead of full path
                        for (size_t i = 1; i < command.args.size(); ++i) {
                            execArgs.push_back(const_cast<char*>(command.args[i].c_str()));
                        }
                        execArgs.push_back(nullptr);
                        execv(fullPath.c_str(), execArgs.data());
                        cerr << "Error executing " << command_str << endl;
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
                cout << command_str << ": command not found" << endl;
            }
        }
    }

    return 0;
}
