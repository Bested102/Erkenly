cmake_minimum_required(VERSION 3.16)

project(parking_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost CONFIG REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

add_executable(parking_server
    main.cpp
)

target_include_directories(parking_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(parking_server PRIVATE
    Boost::system
    Threads::Threads
)

if(WIN32)
    target_link_libraries(parking_server PRIVATE ws2_32)
endif()