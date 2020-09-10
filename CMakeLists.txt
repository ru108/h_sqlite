cmake_minimum_required(VERSION 3.18)

project(h_sqlite VERSION 0.1.0.0 DESCRIPTION "sqlite c++ wrapper components" HOMEPAGE_URL https://github.com/ru108/h_sqlite LANGUAGES CXX)
string(TOUPPER ${PROJECT_NAME} PROJECT_NAMESPACE)
set(PROJECT_CONFIG ${PROJECT_NAME}_config)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

set(CMAKE_CXX_EXTENSIONS OFF)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

add_library("${PROJECT_NAMESPACE}::${PROJECT_NAME}" ALIAS ${PROJECT_NAME})

install(DIRECTORY "include/${PROJECT_NAME}" DESTINATION include)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_CONFIG})
install(EXPORT ${PROJECT_CONFIG} NAMESPACE "${PROJECT_NAMESPACE}::" DESTINATION "share/${PROJECT_NAMESPACE}/cmake")

include(CMakePackageConfigHelpers)
write_basic_package_version_file("${PROJECT_BINARY_DIR}/${PROJECT_CONFIG}_version.cmake"
  VERSION
    ${PROJECT_VERSION}
  COMPATIBILITY
    AnyNewerVersion)
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_CONFIG}_version.cmake" DESTINATION "share/${PROJECT_NAMESPACE}/cmake")
