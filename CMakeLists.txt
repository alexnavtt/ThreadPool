cmake_minimum_required(VERSION 3.0.2)
project(threadpool)

add_compile_options(-std=c++14)

## Specify additional locations of header files
include_directories(
  include
)

## Declare a C++ library
add_library(${PROJECT_NAME} SHARED src/threadpool.cpp)
target_link_libraries(${PROJECT_NAME} pthread)

add_executable(example src/test.cpp)
target_link_libraries(example ${PROJECT_NAME})

#############
## Install ##
#############

## Mark libraries for installation
install(
  TARGETS ${PROJECT_NAME}  
  EXPORT ${PROJECT_NAME}Config
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

## Install the config file associated with the cpp_timer lib
install(
  EXPORT ${PROJECT_NAME}Config
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/${PROJECT_NAME}
  NAMESPACE ${PROJECT_NAME}::
)

## Mark header files for installation
install(
  FILES include/${PROJECT_NAME}/threadpool.hpp
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}
)
