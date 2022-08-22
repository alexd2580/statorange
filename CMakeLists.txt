cmake_minimum_required(VERSION 3.12)
project(
  statorange
  VERSION 2.0.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_custom_target(
  copy-compile-commands ALL
  ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_CURRENT_LIST_DIR})

function(make_clang_warnings ENABLED_WARNINGS_LIST DISABLED_WARNINGS_LIST
         CLANG_WARNING_FLAGS_LIST)
  foreach(WARNING ${${ENABLED_WARNINGS_LIST}})
    list(APPEND WARNING_FLAGS "-W${WARNING}")
  endforeach(WARNING)
  foreach(WARNING ${${DISABLED_WARNINGS_LIST}})
    list(APPEND WARNING_FLAGS "-Wno-${WARNING}")
  endforeach(WARNING)
  set(${CLANG_WARNING_FLAGS_LIST}
      ${WARNING_FLAGS}
      PARENT_SCOPE)
endfunction()

set(ENABLED_WARNINGS everything)
set(DISABLED_WARNINGS
    padded
    c++98-compat
    c++98-compat-pedantic
    old-style-cast
    missing-prototypes
    reserved-id-macro
    weak-vtables
    global-constructors
    exit-time-destructors
    disabled-macro-expansion)
make_clang_warnings(ENABLED_WARNINGS DISABLED_WARNINGS CLANG_WARNING_FLAGS)

add_subdirectory(fmt)

# The source files.
file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE STATORANGE src/statorange.cpp)
list(REMOVE_ITEM SOURCES ${STATORANGE})
file(GLOB_RECURSE UNIT_TEST_SOURCES tests/*.cpp)

set(LIBS
    asound
    ssl
    crypto
    fmt
    pulse
    pthread
    curlpp
    curl)

# The main executable
add_executable(statorange ${SOURCES} ${STATORANGE})
target_include_directories(statorange SYSTEM PRIVATE fmt/include)
target_include_directories(statorange PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_options(statorange PRIVATE ${CLANG_WARNING_FLAGS})
target_link_libraries(statorange ${LIBS})

add_executable(statorange_unit_tests ${SOURCES} ${UNIT_TEST_SOURCES})
target_include_directories(
  statorange_unit_tests SYSTEM
  PRIVATE bandit
  PRIVATE fmt/include)
target_include_directories(statorange_unit_tests
                           PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_options(statorange_unit_tests PRIVATE ${CLANG_WARNING_FLAGS})
target_link_libraries(statorange_unit_tests ${LIBS})

# Add the test execution to cmake.
enable_testing()
add_test(statorange_unit_tests statorange_unit_tests)

# Install the binary.
install(TARGETS statorange DESTINATION /usr/local/bin)
