cmake_minimum_required(VERSION 3.8)
project(statorange CXX)

SET(CMAKE_CXX_COMPILER clang++)

function(make_clang_warnings enabled_warnings_list disabled_warnings_list clang_warning_flags_list)
    foreach(warning ${${enabled_warnings_list}})
        list(APPEND warning_flags "-W${warning}")
    endforeach(warning)
    foreach(warning ${${disabled_warnings_list}})
        list(APPEND warning_flags "-Wno-${warning}")
    endforeach(warning)
    set(${clang_warning_flags_list} ${warning_flags} PARENT_SCOPE)
endfunction()

set(enabled_warnings everything)
set(disabled_warnings
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
make_clang_warnings(enabled_warnings disabled_warnings clang_warning_flags)

# Add system include paths.
include_directories(SYSTEM bandit)

# The source files.
file(GLOB_RECURSE SRC src/*.cpp)
file(GLOB_RECURSE STATORANGE src/statorange.cpp)
list(REMOVE_ITEM SRC ${STATORANGE})

# add_compile_options(-std=c++2a -stdlib=libc++ ${clang_warning_flags})
add_compile_options(-std=c++2a ${clang_warning_flags})
set(LIBS c++ asound ssl crypto)

# The main executable
add_executable(statorange ${STATORANGE} ${SRC})
target_link_libraries(statorange ${LIBS})

# # The test executable(s).
# add_executable(test_suite tests/main.cpp ${SRC})
# target_link_libraries(test_suite ${LIBS})
#
# # Add the test execution to cmake.
# enable_testing()
# add_test(test_suite test_suite)

# Install the binary and the launcher.
install(TARGETS statorange DESTINATION /usr/local/bin)
install(PROGRAMS statorange_launcher DESTINATION /usr/local/bin)

# Install the fonts.
file(GLOB MISC_FONTS res/misc/*)
install(
    FILES ${MISC_FONTS}
    DESTINATION /usr/local/lib/statorange/misc
)
