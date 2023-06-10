add_executable(simple_client src/simple_client.cpp)
add_executable(simple_server src/simple_server.cpp)

target_include_directories(simple_client PRIVATE include)
target_include_directories(simple_server PRIVATE include)
