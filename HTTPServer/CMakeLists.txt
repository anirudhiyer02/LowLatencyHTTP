cmake_minimum_required(VERSION 3.10)
project(HTTPServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Boost
find_package(Boost REQUIRED COMPONENTS system thread)

# Add nlohmann/json
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

# Add server executable
add_executable(server src/main.cpp)

# Add client executable
add_executable(client src/client.cpp)

# Link libraries
target_link_libraries(server PRIVATE Boost::system Boost::thread nlohmann_json::nlohmann_json)
target_link_libraries(client PRIVATE Boost::system Boost::thread) 