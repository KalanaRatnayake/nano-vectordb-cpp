# Minimum CMake version
cmake_minimum_required(VERSION 3.14)

# Project name and language
project(nano_vectordb_cpp LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Eigen3 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(SQLite3 REQUIRED)

# Add include directories
include_directories(
  include
  ${EIGEN3_INCLUDE_DIR}
)

# Add the test executable
add_executable(test_db 
  src/test_db.cpp 
)

# Link libraries
target_link_libraries(test_db 
  PRIVATE 
  nlohmann_json::nlohmann_json 
  ssl 
  crypto 
  Eigen3::Eigen
  SQLite::SQLite3
)