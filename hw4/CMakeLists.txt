cmake_minimum_required(VERSION 3.20)
project(checker CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")

option(ENABLE_ASAN OFF)
option(ENABLE_TSAN OFF)
option(ENABLE_UBSAN OFF)
option(ENABLE_VALGRIND OFF)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(app
    src/main.cpp
    src/key.cpp
    src/worker.cpp
    src/client.cpp
    src/protocol.cpp
)

target_include_directories(app PRIVATE src)
target_link_libraries(app PRIVATE Threads::Threads OpenSSL::Crypto)

add_executable(server
    src/server_main.cpp
    src/server.cpp
    src/key.cpp
    src/protocol.cpp
)

target_include_directories(server PRIVATE src)
target_link_libraries(server PRIVATE Threads::Threads OpenSSL::Crypto)

if(ENABLE_ASAN)
    target_compile_options(app PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(app PRIVATE -fsanitize=address)
endif()

if(ENABLE_TSAN)
    target_compile_options(app PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
    target_link_options(app PRIVATE -fsanitize=thread)
endif()

if(ENABLE_UBSAN)
    target_compile_options(app PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
    target_link_options(app PRIVATE -fsanitize=undefined)
endif()

if(ENABLE_VALGRIND)
    target_compile_options(app PRIVATE -O0 -g3 -fno-inline)
endif()

message(STATUS "Build type     : ${CMAKE_BUILD_TYPE}")
message(STATUS "ASAN           : ${ENABLE_ASAN}")
message(STATUS "TSAN           : ${ENABLE_TSAN}")
message(STATUS "UBSAN          : ${ENABLE_UBSAN}")
message(STATUS "Valgrind mode  : ${ENABLE_VALGRIND}")
