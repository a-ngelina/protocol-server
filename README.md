# Protocol Server

A simple protocol server written in C++ that handles file operations and basic server commands.

## Features

- **Multi-threaded**: Handles multiple client connections concurrently
- **File Operations**: Read, write, and list directory contents
- **Status Monitoring**: Check server status and gracefully disconnect
- **File Locking**: Uses Linux file locks to avoid race conditions
- **Debug Mode**: Optional debug output for request/response monitoring

## Supported Commands

The server supports the following protocol requests:

- `STATUS` - Check if the server is running
- `QUIT` - Disconnect from the server
- `LIST path/to/directory` - List contents of a directory
- `GET path/to/file` - Retrieve file contents
- `POST path/to/file\ncontents` - Create or overwrite a file with given contents

## Requirements

- **C++ Standard**: C++20 or later
- **Operating System**: Linux (uses Linux-specific file locks and system calls)
- **Compiler**: GCC with C++20 support

## Building

### Basic Build

```bash
make
```

### Build and Run

```bash
make run
```

### Debug Build

For additional debug information during runtime:

```bash
make CXXFLAGS="-std=c++20 -DDEBUG"
```

### Clean Build Files

```bash
make clean
```

## Testing

- Test files are located in the `tests/` directory
- `tests/client_functions.cpp/h` - Common client functionality

## Project Structure

```
.
├── Makefile                                    # Build configuration
├── README.md                                   # This file
├── server_main.cpp                            # Main server logic
├── network.cpp/h                              # Network communication
├── file_manipulation.cpp/h                    # File operations
├── request_parsing_response_preparation.cpp/h # Protocol handling
├── string_utilities.cpp/h                     # String utilities
└── tests/                                     # Test files
    ├── client_functions.cpp/h                 # Test utilities
    └── client_*.cpp                           # Individual test cases
```

## Protocol Details

- **Port**: 8080
- **Protocol**: TCP
- **Response Format**: length-prefixed, HTTP-style status codes (200, 400, 403, 500) and status messages
- **Threading**: Each client connection is handled in a separate thread
- **File Safety**: Uses file locking to prevent concurrent access issues
