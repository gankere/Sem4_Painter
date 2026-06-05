# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\PainterQt_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\PainterQt_autogen.dir\\ParseCache.txt"
  "PainterQt_autogen"
  )
endif()
