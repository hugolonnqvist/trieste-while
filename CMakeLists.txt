cmake_minimum_required(VERSION 3.14.0)
# cmake_minimum_required(VERSION 4.0)
project(while VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
  trieste
  GIT_REPOSITORY https://github.com/hugolonnqvist/Trieste
  GIT_BRANCH feature/conditional-pass 
  GIT_TAG 937c97852c4faf72a11d46b3f537a23ee8e8b8d5
  
  # GIT_REPOSITORY https://github.com/microsoft/Trieste
  # GIT_TAG 370ad85aa31f316f8ac39dcaaf6f4218901242e4
  )

FetchContent_MakeAvailable(trieste)

add_executable(while
src/while.cc
src/parser.cc
src/reader.cc
src/interpreter.cc
src/optimization_analysis.cc

src/utils.cc
src/control_flow.cc

src/passes/expressions.cc
src/passes/statements.cc
src/passes/check_refs.cc
src/passes/eval.cc
src/passes/normalization.cc
src/passes/gather_control_flow.cc
src/passes/zero_analysis.cc
src/passes/constant_folding.cc
src/passes/dead_code_elimination.cc
)

add_executable(while_trieste
src/while_trieste.cc
src/parser.cc
src/reader.cc

src/utils.cc
src/control_flow.cc

src/passes/expressions.cc
src/passes/statements.cc
src/passes/check_refs.cc
src/passes/normalization.cc
)

target_link_libraries(while
  CLI11::CLI11
  trieste::trieste
)

target_link_libraries(while_trieste
  CLI11::CLI11
  trieste::trieste
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(TRIESTE_BUILD_SAMPLES OFF)
