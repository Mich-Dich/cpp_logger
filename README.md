# Logger System

A simple and flexible logging system implemented in C++ that allows for customizable log message formats and severity levels. This project includes a demonstration of the logging system in action.

## Features

- **Log Levels**: Supports multiple log levels including Trace, Debug, Info, Warn, Error, and Fatal.
- **Customizable Format**: Users can define their own log message formats using various tags.
- **Thread-Safe**: The logging system is designed to be thread-safe, allowing for concurrent logging from multiple threads.
- **Validation and Assertions**: Built-in macros for validation and assertions to help with debugging.

## Files

- `main.cpp`: The main application demonstrating the logging system.
- `logger.h`: Header file defining the logging system's interface and data structures.
- `logger.cpp`: Implementation of the logging system.
- `util.h / util.cpp`: Utility functions used within the logger.

## Getting Started

### Prerequisites

- A C++ compiler like GCC (C++11 or later recommended).

### Building the Project

1. Clone the repository:
  ```bash
  git clone git@github.com:Mich-Dich/cpp_logger.git
  cd cpp_logger
  ```

  Compile the project with GCC:
  ```bash
  g++ -o logging_test main.cpp logger.cpp util.cpp
  ```

# Usage
### Run the compiled application:
  ```bash
  ./logging_test
  ```
The application will generate log messages of various severity levels and demonstrate the validation and assertion macros.

### Customizing Log Format
You can customize the log format by modifying the logger::init function call in main.cpp. The format string can include various tags such as:

### Log Format Tags
The following tags can be used to customize the log message format:

| Tag | Description                                   | Example                      |
|-----|-----------------------------------------------|------------------------------|
| `$T` | Time                                         | hh:mm:ss                     |
| `$H` | Hour                                         | hh                           |
| `$M` | Minute                                       | mm                           |
| `$S` | Second                                       | ss                           |
| `$J` | Milliseconds                                  | jjj                          |
| `$N` | Date                                         | yyyy:mm:dd                   |
| `$Y` | Year                                         | yyyy                         |
| `$O` | Month                                        | mm                           |
| `$D` | Day                                          | dd                           |
| `$F` | Function name                                 | application::main, math::foo |
| `$P` | Only function name                            | main, foo                   |
| `$A` | File name                                    | C:/project/main.cpp, ~/project/main.cpp |
| `$K` | Shortened file name                           | project/main.cpp            |
| `$I` | Only file name                               | main.cpp                     |
| `$G` | Line number                                   | 1, 42                        |
| `$L` | Log level                                    | [TRACE], [DEBUG], ... [FATAL] |
| `$X` | Alignment                                     | Adds space for "INFO" & "WARN" |
| `$B` | Color begin                                  | From here the color begins   |
| `$E` | Color end                                    | From here the color will be reset |
| `$C` | Text                                         | The message the user wants to print |
| `$Z` | New line                                     | Add a new line in the message format |

For example:
  
  ```cpp
  logger::init("[$B$T:$J$E - $B$A $F:$G$E] $C$Z");
  ```
### Logging Levels
You can control which log levels are enabled by modifying the `LOG_LEVEL_ENABLED` define in logger.h. The levels are:

- 0: Fatal + Error
- 1: Fatal + Error + Warn
- 2: Fatal + Error + Warn + Info
- 3: Fatal + Error + Warn + Info + Debug
- 4: Fatal + Error + Warn + Info + Debug + Trace

# Contributing
Contributions are welcome! Please feel free to submit a pull request or open an issue for any suggestions or improvements.

# License
This project is licensed under the MIT License. See the LICENSE file for more details.

