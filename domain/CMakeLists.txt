add_library(erkenly_domain STATIC
    Gate.hpp
    Gate.cpp
    ParkingLot.hpp
    ParkingLot.cpp
    ParkingModel.hpp
    ParkingModel.cpp
    ParkingSpot.hpp
    ParkingSpot.cpp
)

target_include_directories(erkenly_domain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)