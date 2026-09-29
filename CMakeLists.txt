cmake_minimum_required(VERSION 3.16)
project(embedded_sensor_stack VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

foreach(daemon sensord procd metricsd)
  add_executable(${daemon} src/${daemon}/main.cpp)
  target_include_directories(${daemon} PRIVATE include)
  install(TARGETS ${daemon} RUNTIME DESTINATION bin)
endforeach()
