cmake_minimum_required(VERSION 3.18)

project(sample_h_sqlite LANGUAGES C CXX)

set(CMAKE_SUPPRESS_REGENERATION ON)

set(_LIBRARY_DEP h_sqlite)

find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} ${PROJECT_NAME}.cpp)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")

target_compile_definitions(${PROJECT_NAME}
  PRIVATE $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:-DUNICODE;-D_UNICODE>
  PRIVATE $<$<COMPILE_LANG_AND_ID:C,MSVC>:-DUNICODE;-D_UNICODE>)

add_subdirectory(../ "${CMAKE_CURRENT_BINARY_DIR}/${_LIBRARY_DEP}" EXCLUDE_FROM_ALL)
message(STATUS "Using ${_LIBRARY_DEP} via add_subdirectory")

target_link_libraries(${PROJECT_NAME} PRIVATE ${_LIBRARY_DEP} Threads::Threads ${CMAKE_DL_LIBS})

include(FetchContent)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 7.0.3
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_Declare(
  sqlite3
  URL https://www.sqlite.org/2020/sqlite-amalgamation-3330000.zip
  URL_HASH SHA3_256=fb7dfd39009fb40519097b0b1a6af5e8acb17006c848f6d6b7707a4a0c3885c3
)

message(STATUS "Using fmtlib via add_subdirectory (FetchContent)")
FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
  FetchContent_Populate(fmt)
  add_subdirectory(${fmt_SOURCE_DIR} EXCLUDE_FROM_ALL)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

message(STATUS "Using sqlite3 via directory (FetchContent)")
FetchContent_GetProperties(sqlite3)
if(NOT sqlite3_POPULATED)
  FetchContent_Populate(sqlite3)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${sqlite3_SOURCE_DIR})
target_sources(${PROJECT_NAME} PRIVATE "${sqlite3_SOURCE_DIR}/sqlite3.c")
