cmake_minimum_required(VERSION 3.16)

project(parking_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(HIREDIS REQUIRED hiredis)

add_executable(parking_server
    main.cpp
)

target_include_directories(parking_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HIREDIS_INCLUDE_DIRS}
)

target_link_directories(parking_server PRIVATE
    ${HIREDIS_LIBRARY_DIRS}
)

target_compile_options(parking_server PRIVATE
    ${HIREDIS_CFLAGS_OTHER}
)

target_link_libraries(parking_server PRIVATE
    Boost::system
    Threads::Threads
    ${HIREDIS_LIBRARIES}
)

if(WIN32)
    target_link_libraries(parking_server PRIVATE ws2_32)
endif()