# cmake_minimum_required(VERSION 3.14.0)
cmake_minimum_required(VERSION 4.0)
project(while VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
  trieste
  GIT_REPOSITORY https://github.com/microsoft/Trieste
  GIT_TAG 370ad85aa31f316f8ac39dcaaf6f4218901242e4
  )

FetchContent_MakeAvailable(trieste)

add_executable(while
src/main.cc
src/parser.cc
src/passes/expressions.cc
src/passes/statements.cc
src/passes/check_refs.cc
src/passes/zero_analysis.cc
src/utils.cc
)

target_link_libraries(while
  CLI11::CLI11
  trieste::trieste
)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
