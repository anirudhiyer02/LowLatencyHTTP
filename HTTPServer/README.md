# Low-Latency HTTP Server

A simple, low-latency HTTP server implemented in C++ using Boost.Asio.

## Prerequisites

- C++17 compatible compiler
- CMake (version 3.10 or higher)
- Boost library (version 1.71 or higher)

## Building the Project

1. Create a build directory:
```bash
mkdir build
cd build
```

2. Configure and build the project:
```bash
cmake ..
make
```

## Running the Server

After building, you can run the server from the build directory:
```bash
./server
```

The server will start listening on port 8080. You can test it using curl or a web browser:
```bash
curl http://localhost:8080
```

## Features

- Asynchronous I/O using Boost.Asio
- Multi-threaded request handling
- Basic HTTP/1.1 support
- Low-latency design

## Performance Considerations

This implementation uses:
- Asynchronous I/O to handle multiple connections efficiently
- Thread-per-connection model for request handling
- Zero-copy operations where possible
- Modern C++ features for better performance

## Future Improvements

- Add proper HTTP request parsing
- Implement keep-alive connections
- Add support for different HTTP methods
- Implement proper error handling
- Add configuration options
- Add support for HTTPS 