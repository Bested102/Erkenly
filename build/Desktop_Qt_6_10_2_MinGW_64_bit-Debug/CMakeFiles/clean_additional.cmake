# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\Erkenly_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Erkenly_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\erkenly_domain_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\erkenly_domain_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\erkenly_networking_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\erkenly_networking_autogen.dir\\ParseCache.txt"
  "Erkenly_autogen"
  "erkenly_domain_autogen"
  "erkenly_networking_autogen"
  )
endif()
