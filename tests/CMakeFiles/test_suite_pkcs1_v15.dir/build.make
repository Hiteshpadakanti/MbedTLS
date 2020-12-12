# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/jordan/mbedtls

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/jordan/mbedtls

# Include any dependencies generated for this target.
include tests/CMakeFiles/test_suite_pkcs1_v15.dir/depend.make

# Include the progress variables for this target.
include tests/CMakeFiles/test_suite_pkcs1_v15.dir/progress.make

# Include the compile flags for this target's objects.
include tests/CMakeFiles/test_suite_pkcs1_v15.dir/flags.make

tests/test_suite_pkcs1_v15.c: tests/scripts/generate_code.pl
tests/test_suite_pkcs1_v15.c: library/libmbedtls.so
tests/test_suite_pkcs1_v15.c: tests/suites/helpers.function
tests/test_suite_pkcs1_v15.c: tests/suites/main_test.function
tests/test_suite_pkcs1_v15.c: tests/suites/test_suite_pkcs1_v15.function
tests/test_suite_pkcs1_v15.c: tests/suites/test_suite_pkcs1_v15.data
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/jordan/mbedtls/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating test_suite_pkcs1_v15.c"
	cd /home/jordan/mbedtls/tests && /usr/bin/perl /home/jordan/mbedtls/tests/scripts/generate_code.pl /home/jordan/mbedtls/tests/suites test_suite_pkcs1_v15 test_suite_pkcs1_v15

tests/CMakeFiles/test_suite_pkcs1_v15.dir/test_suite_pkcs1_v15.c.o: tests/CMakeFiles/test_suite_pkcs1_v15.dir/flags.make
tests/CMakeFiles/test_suite_pkcs1_v15.dir/test_suite_pkcs1_v15.c.o: tests/test_suite_pkcs1_v15.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jordan/mbedtls/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object tests/CMakeFiles/test_suite_pkcs1_v15.dir/test_suite_pkcs1_v15.c.o"
	cd /home/jordan/mbedtls/tests && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/test_suite_pkcs1_v15.dir/test_suite_pkcs1_v15.c.o   -c /home/jordan/mbedtls/tests/test_suite_pkcs1_v15.c

tests/CMakeFiles/test_suite_pkcs1_v15.dir/test_suite_pkcs1_v15.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/test_suite_pkcs1_v15.dir/test_suite_pkcs1_v15.c.i"
	cd /home/jordan/mbedtls/tests && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jordan/mbedtls/tests/test_suite_pkcs1_v15.c > CMakeFiles/test_suite_pkcs1_v15.dir/test_suite_pkcs1_v15.c.i

tests/CMakeFiles/test_suite_pkcs1_v15.dir/test_suite_pkcs1_v15.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/test_suite_pkcs1_v15.dir/test_suite_pkcs1_v15.c.s"
	cd /home/jordan/mbedtls/tests && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jordan/mbedtls/tests/test_suite_pkcs1_v15.c -o CMakeFiles/test_suite_pkcs1_v15.dir/test_suite_pkcs1_v15.c.s

# Object files for target test_suite_pkcs1_v15
test_suite_pkcs1_v15_OBJECTS = \
"CMakeFiles/test_suite_pkcs1_v15.dir/test_suite_pkcs1_v15.c.o"

# External object files for target test_suite_pkcs1_v15
test_suite_pkcs1_v15_EXTERNAL_OBJECTS =

tests/test_suite_pkcs1_v15: tests/CMakeFiles/test_suite_pkcs1_v15.dir/test_suite_pkcs1_v15.c.o
tests/test_suite_pkcs1_v15: tests/CMakeFiles/test_suite_pkcs1_v15.dir/build.make
tests/test_suite_pkcs1_v15: library/libmbedtls.so.2.4.2
tests/test_suite_pkcs1_v15: library/libmbedx509.so.2.4.2
tests/test_suite_pkcs1_v15: library/libmbedcrypto.so.2.4.2
tests/test_suite_pkcs1_v15: tests/CMakeFiles/test_suite_pkcs1_v15.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/jordan/mbedtls/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable test_suite_pkcs1_v15"
	cd /home/jordan/mbedtls/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_suite_pkcs1_v15.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tests/CMakeFiles/test_suite_pkcs1_v15.dir/build: tests/test_suite_pkcs1_v15

.PHONY : tests/CMakeFiles/test_suite_pkcs1_v15.dir/build

tests/CMakeFiles/test_suite_pkcs1_v15.dir/clean:
	cd /home/jordan/mbedtls/tests && $(CMAKE_COMMAND) -P CMakeFiles/test_suite_pkcs1_v15.dir/cmake_clean.cmake
.PHONY : tests/CMakeFiles/test_suite_pkcs1_v15.dir/clean

tests/CMakeFiles/test_suite_pkcs1_v15.dir/depend: tests/test_suite_pkcs1_v15.c
	cd /home/jordan/mbedtls && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/jordan/mbedtls /home/jordan/mbedtls/tests /home/jordan/mbedtls /home/jordan/mbedtls/tests /home/jordan/mbedtls/tests/CMakeFiles/test_suite_pkcs1_v15.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/CMakeFiles/test_suite_pkcs1_v15.dir/depend

