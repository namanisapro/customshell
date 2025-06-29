# 🐚 CustomShell - A Feature-Rich POSIX Shell Implementation

A comprehensive, production-ready shell implementation written in C++ that demonstrates advanced systems programming concepts and provides a full-featured command-line interface.

## ✨ Features

### 🚀 Core Shell Functionality
- **Interactive REPL** with customizable prompt
- **Command Execution** for both built-in and external programs
- **Process Management** with proper signal handling
- **Environment Variable** support and manipulation
- **Working Directory** management

### 🛠️ Built-in Commands
- `echo` - Print arguments with proper escaping
- `exit` - Clean shell termination
- `type` - Command type detection (builtin/executable)
- `pwd` - Print current working directory
- `cd` - Directory navigation (absolute, relative, home directory)
- `history` - Command history management

### 🔧 Advanced Features

#### **Command Parsing & Quoting**
- **Single Quotes** - Literal string handling
- **Double Quotes** - Variable expansion support
- **Backslash Escaping** - Character escaping in all contexts
- **Complex Argument Parsing** - Handles spaces, special characters, and nested quotes

#### **I/O Redirection**
- **Standard Output**: `>`, `>>` (overwrite/append)
- **Standard Error**: `2>`, `2>>` (overwrite/append)
- **File Descriptor Management** - Proper cleanup and error handling
- **Built-in Command Redirection** - All built-ins support redirection

#### **Pipeline Support**
- **Multi-command Pipelines** - Unlimited command chaining
- **Process Coordination** - Proper pipe creation and cleanup
- **Built-in Pipeline Support** - Built-ins work seamlessly in pipelines
- **Error Handling** - Graceful failure recovery

#### **Command History**
- **Interactive Navigation** - Up/down arrow key support
- **History Persistence** - Automatic save/load via `HISTFILE`
- **History Commands**:
  - `history` - List all commands
  - `history <n>` - Show last n commands
  - `history -r <file>` - Load history from file
  - `history -w <file>` - Write history to file
  - `history -a <file>` - Append new commands to file

#### **Autocompletion**
- **Built-in Commands** - Tab completion for all built-ins
- **Executable Discovery** - PATH-based executable completion
- **Partial Matching** - Intelligent completion suggestions
- **Duplicate Prevention** - Smart deduplication of suggestions

## 🏗️ Architecture

### **Core Components**
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Command Line  │───▶│   Parser        │───▶│   Executor      │
│   Interface     │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Readline      │    │   Redirection   │    │   Process       │
│   Integration   │    │   Handler       │    │   Manager       │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### **Key Design Decisions**
- **Modular Architecture** - Clean separation of concerns
- **Error Handling** - Comprehensive error recovery
- **Memory Management** - Proper resource cleanup
- **POSIX Compliance** - Standards-compliant implementation

## 🛠️ Technical Implementation

### **Systems Programming Concepts**
- **Process Creation** - `fork()`, `exec()`, `wait()`
- **File Descriptors** - `dup2()`, `pipe()`, `open()`
- **Signal Handling** - Process management and cleanup
- **Memory Management** - Dynamic allocation and cleanup

### **Advanced C++ Features**
- **STL Containers** - `std::vector`, `std::map`, `std::string`
- **Smart Pointers** - Modern C++ memory management
- **Exception Handling** - Robust error management
- **Template Usage** - Generic programming where appropriate

### **External Libraries**
- **GNU Readline** - Professional command-line interface
- **POSIX APIs** - Standard system calls and interfaces

## 📦 Building & Installation

### **Prerequisites**
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libreadline-dev

# macOS
brew install cmake readline

# CentOS/RHEL
sudo yum install gcc-c++ cmake readline-devel
```

### **Build Instructions**
```bash
# Clone the repository
git clone https://github.com/namanisapro/customshell.git
cd customshell

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Run the shell
./shell
```

## 🎯 Usage Examples

### **Basic Commands**
```bash
$ echo "Hello, World!"
Hello, World!

$ pwd
/home/user/projects

$ cd /tmp && pwd
/tmp
```

### **Advanced Features**
```bash
# Pipeline with multiple commands
$ ls -la | grep "\.txt" | wc -l
5

# Redirection with built-ins
$ echo "Error message" 2> error.log
$ history 5 > recent_commands.txt

# Complex quoting
$ echo 'Single "quotes" and \backslashes'
Single "quotes" and \backslashes

# History management
$ history -r commands.txt
$ history -a session.log
```

## 🧪 Testing

The shell includes comprehensive test coverage for all features:
- **Unit Tests** - Individual component testing
- **Integration Tests** - End-to-end functionality
- **Edge Case Testing** - Error conditions and boundary cases

## 🔍 Code Quality

- **Clean Code Principles** - Readable, maintainable code
- **Error Handling** - Comprehensive error management
- **Documentation** - Inline comments and clear structure
- **Performance** - Optimized for efficiency

## 🚀 Performance Characteristics

- **Fast Startup** - Minimal initialization overhead
- **Efficient Memory Usage** - Optimized for resource usage
- **Responsive Interface** - Real-time command processing
- **Scalable Architecture** - Handles complex command chains

## 🎓 Learning Outcomes

This project demonstrates mastery of:
- **Systems Programming** - Low-level OS interactions
- **C++ Development** - Modern C++ best practices
- **Software Architecture** - Clean, modular design
- **Problem Solving** - Complex feature implementation
- **Testing & Debugging** - Comprehensive validation

## 📈 Future Enhancements

Potential areas for expansion:
- **Job Control** - Background process management
- **Aliases** - Command aliasing support
- **Scripting** - Shell script execution
- **Customization** - Configuration file support
- **Plugin System** - Extensible architecture

## 🤝 Contributing

This is a personal project demonstrating systems programming skills. For educational purposes, feel free to fork and experiment!

## 📄 License

This project is for educational and portfolio purposes.

---

**Built with ❤️ and lots of coffee ☕**

*This shell implementation showcases advanced systems programming concepts and demonstrates the ability to build complex, production-ready software from the ground up.* 