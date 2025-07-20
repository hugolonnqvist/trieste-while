cmake_minimum_required(VERSION 3.14.0)
project(while VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
  trieste
  GIT_REPOSITORY https://github.com/microsoft/Trieste
  GIT_TAG main
  )

FetchContent_MakeAvailable(trieste)

FetchContent_Declare(
  vbc
  GIT_REPOSITORY https://github.com/sylvanc/verona-bc
  GIT_TAG 19944ae71191f2f242b52cf0eb5a9f745536e313
  )

FetchContent_MakeAvailable(vbc)

add_library(ffi::ffi UNKNOWN IMPORTED)
set_target_properties(ffi::ffi PROPERTIES
  IMPORTED_LOCATION "/opt/homebrew/Cellar/libffi/3.4.8/lib"
  INTERFACE_INCLUDE_DIRECTORIES "/opt/homebrew/Cellar/libffi/3.4.8/include")

add_library(while_lib SHARED
src/lib.cc
)

add_executable(while
src/while.cc
src/parser.cc
src/reader.cc
src/optimization_analysis.cc
src/compiler.cc

src/utils.cc
src/control_flow.cc

src/passes/generate_mermaid.cc

src/passes/functions.cc
src/passes/expressions.cc
src/passes/statements.cc
src/passes/check_refs.cc

src/passes/unique_variables.cc
src/passes/gather_stats.cc
src/passes/normalization.cc
src/passes/gather_control_flow.cc
src/passes/zero_analysis.cc
src/passes/constant_folding.cc
src/passes/dead_code_elimination.cc

src/passes/to3addr.cc
src/passes/gather_vars.cc
src/passes/blockify.cc
src/passes/compile.cc
)

add_executable(while_trieste
src/while_trieste.cc
src/parser.cc
src/reader.cc

src/passes/generate_mermaid.cc
src/utils.cc
src/control_flow.cc

src/passes/functions.cc
src/passes/expressions.cc
src/passes/statements.cc
src/passes/check_refs.cc
src/passes/unique_variables.cc
src/passes/gather_stats.cc
src/passes/normalization.cc

src/passes/to3addr.cc
src/passes/gather_vars.cc
src/passes/blockify.cc
src/passes/compile.cc
)

target_link_libraries(while
  CLI11::CLI11
  trieste::trieste
  vbc::include
)

target_link_libraries(while_trieste
  CLI11::CLI11
  trieste::trieste
  vbc::include
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(TRIESTE_BUILD_SAMPLES OFF)
set(CMAKE_CXX_FLAGS "-g")
