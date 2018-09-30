cmake_minimum_required(VERSION 3.8)
project(statorange VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(make_clang_warnings ENABLED_WARNINGS_LIST DISABLED_WARNINGS_LIST CLANG_WARNING_FLAGS_LIST)
    foreach(WARNING ${${ENABLED_WARNINGS_LIST}})
        list(APPEND WARNING_FLAGS "-W${WARNING}")
    endforeach(WARNING)
    foreach(WARNING ${${DISABLED_WARNINGS_LIST}})
        list(APPEND WARNING_FLAGS "-Wno-${WARNING}")
    endforeach(WARNING)
    set(${CLANG_WARNING_FLAGS_LIST} ${WARNING_FLAGS} PARENT_SCOPE)
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
    disabled-macro-expansion
)
make_clang_warnings(ENABLED_WARNINGS DISABLED_WARNINGS CLANG_WARNING_FLAGS)

# Add system include paths.

# The source files.
file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE STATORANGE src/statorange.cpp)
list(REMOVE_ITEM SOURCES ${STATORANGE})
file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)

set(LIBS asound ssl crypto c++)

# The main executable
add_executable(statorange ${SOURCES} ${STATORANGE})
target_include_directories(statorange PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_options(statorange PRIVATE -stdlib=libc++ ${CLANG_WARNING_FLAGS})
target_compile_features(statorange PRIVATE cxx_std_20)
target_link_libraries(statorange ${LIBS})

add_executable(statorange_test ${SOURCES} ${TEST_SOURCES})
target_include_directories(statorange_test SYSTEM PRIVATE bandit)
target_include_directories(statorange_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_options(statorange_test PRIVATE -stdlib=libc++ ${CLANG_WARNING_FLAGS})
target_compile_features(statorange_test PRIVATE cxx_std_20)
target_link_libraries(statorange_test ${LIBS})

# # The test executable(s).
# add_executable(test_suite tests/main.cpp ${SOURCES})
# target_link_libraries(test_suite ${LIBS})
#
# # Add the test execution to cmake.
# enable_testing()
# add_test(test_suite test_suite)

# Install the binary.
install(TARGETS statorange DESTINATION /usr/local/bin)

# Install the fonts.
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/res/misc
    DESTINATION /usr/local/lib/statorange/misc
)
