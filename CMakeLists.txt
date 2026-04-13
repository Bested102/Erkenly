cmake_minimum_required(VERSION 3.16)

project(Erkenly VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

add_library(erkenly_domain STATIC
    domain/Gate.hpp
    domain/Gate.cpp
    domain/ParkingLot.hpp
    domain/ParkingLot.cpp
    domain/ParkingModel.hpp
    domain/ParkingModel.cpp
    domain/ParkingSpot.hpp
    domain/ParkingSpot.cpp
)

target_include_directories(erkenly_domain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/domain
)

add_library(erkenly_networking STATIC
    networking/ParkingClient.h
    networking/ParkingClient.cpp
)

target_include_directories(erkenly_networking PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/networking
)

target_link_libraries(erkenly_networking PUBLIC
    Qt${QT_VERSION_MAJOR}::Network
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Erkenly
        MANUAL_FINALIZATION

        ui/main.cpp
        ui/mainwindow.h
        ui/mainwindow.cpp
        ui/mainwindow.ui
        ui/lotoverview.h
        ui/lotoverview.cpp
        ui/lotOverview.ui
        ui/mapview.h
        ui/mapview.cpp
        ui/mapview.ui

        app/AppController.h
        app/AppController.cpp
    )
else()
    add_executable(Erkenly
        ui/main.cpp
        ui/mainwindow.h
        ui/mainwindow.cpp
        ui/mainwindow.ui
        ui/lotoverview.h
        ui/lotoverview.cpp
        ui/lotOverview.ui
        ui/mapview.h
        ui/mapview.cpp
        ui/mapview.ui

        app/AppController.h
        app/AppController.cpp
    )
endif()

target_include_directories(Erkenly PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/domain
    ${CMAKE_CURRENT_SOURCE_DIR}/networking
)

target_link_libraries(Erkenly PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    erkenly_domain
    erkenly_networking
)

set_target_properties(Erkenly PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Erkenly)
endif()