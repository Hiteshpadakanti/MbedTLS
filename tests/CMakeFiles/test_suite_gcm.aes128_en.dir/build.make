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
include tests/CMakeFiles/test_suite_gcm.aes128_en.dir/depend.make

# Include the progress variables for this target.
include tests/CMakeFiles/test_suite_gcm.aes128_en.dir/progress.make

# Include the compile flags for this target's objects.
include tests/CMakeFiles/test_suite_gcm.aes128_en.dir/flags.make

tests/test_suite_gcm.aes128_en.c: tests/scripts/generate_code.pl
tests/test_suite_gcm.aes128_en.c: library/libmbedtls.so
tests/test_suite_gcm.aes128_en.c: tests/suites/helpers.function
tests/test_suite_gcm.aes128_en.c: tests/suites/main_test.function
tests/test_suite_gcm.aes128_en.c: tests/suites/test_suite_gcm.function
tests/test_suite_gcm.aes128_en.c: tests/suites/test_suite_gcm.aes128_en.data
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/jordan/mbedtls/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating test_suite_gcm.aes128_en.c"
	cd /home/jordan/mbedtls/tests && /usr/bin/perl /home/jordan/mbedtls/tests/scripts/generate_code.pl /home/jordan/mbedtls/tests/suites test_suite_gcm test_suite_gcm.aes128_en

tests/CMakeFiles/test_suite_gcm.aes128_en.dir/test_suite_gcm.aes128_en.c.o: tests/CMakeFiles/test_suite_gcm.aes128_en.dir/flags.make
tests/CMakeFiles/test_suite_gcm.aes128_en.dir/test_suite_gcm.aes128_en.c.o: tests/test_suite_gcm.aes128_en.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jordan/mbedtls/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object tests/CMakeFiles/test_suite_gcm.aes128_en.dir/test_suite_gcm.aes128_en.c.o"
	cd /home/jordan/mbedtls/tests && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/test_suite_gcm.aes128_en.dir/test_suite_gcm.aes128_en.c.o   -c /home/jordan/mbedtls/tests/test_suite_gcm.aes128_en.c

tests/CMakeFiles/test_suite_gcm.aes128_en.dir/test_suite_gcm.aes128_en.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/test_suite_gcm.aes128_en.dir/test_suite_gcm.aes128_en.c.i"
	cd /home/jordan/mbedtls/tests && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jordan/mbedtls/tests/test_suite_gcm.aes128_en.c > CMakeFiles/test_suite_gcm.aes128_en.dir/test_suite_gcm.aes128_en.c.i

tests/CMakeFiles/test_suite_gcm.aes128_en.dir/test_suite_gcm.aes128_en.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/test_suite_gcm.aes128_en.dir/test_suite_gcm.aes128_en.c.s"
	cd /home/jordan/mbedtls/tests && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jordan/mbedtls/tests/test_suite_gcm.aes128_en.c -o CMakeFiles/test_suite_gcm.aes128_en.dir/test_suite_gcm.aes128_en.c.s

# Object files for target test_suite_gcm.aes128_en
test_suite_gcm_aes128_en_OBJECTS = \
"CMakeFiles/test_suite_gcm.aes128_en.dir/test_suite_gcm.aes128_en.c.o"

# External object files for target test_suite_gcm.aes128_en
test_suite_gcm_aes128_en_EXTERNAL_OBJECTS =

tests/test_suite_gcm.aes128_en: tests/CMakeFiles/test_suite_gcm.aes128_en.dir/test_suite_gcm.aes128_en.c.o
tests/test_suite_gcm.aes128_en: tests/CMakeFiles/test_suite_gcm.aes128_en.dir/build.make
tests/test_suite_gcm.aes128_en: library/libmbedtls.so.2.4.2
tests/test_suite_gcm.aes128_en: library/libmbedx509.so.2.4.2
tests/test_suite_gcm.aes128_en: library/libmbedcrypto.so.2.4.2
tests/test_suite_gcm.aes128_en: tests/CMakeFiles/test_suite_gcm.aes128_en.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/jordan/mbedtls/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable test_suite_gcm.aes128_en"
	cd /home/jordan/mbedtls/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_suite_gcm.aes128_en.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tests/CMakeFiles/test_suite_gcm.aes128_en.dir/build: tests/test_suite_gcm.aes128_en

.PHONY : tests/CMakeFiles/test_suite_gcm.aes128_en.dir/build

tests/CMakeFiles/test_suite_gcm.aes128_en.dir/clean:
	cd /home/jordan/mbedtls/tests && $(CMAKE_COMMAND) -P CMakeFiles/test_suite_gcm.aes128_en.dir/cmake_clean.cmake
.PHONY : tests/CMakeFiles/test_suite_gcm.aes128_en.dir/clean

tests/CMakeFiles/test_suite_gcm.aes128_en.dir/depend: tests/test_suite_gcm.aes128_en.c
	cd /home/jordan/mbedtls && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/jordan/mbedtls /home/jordan/mbedtls/tests /home/jordan/mbedtls /home/jordan/mbedtls/tests /home/jordan/mbedtls/tests/CMakeFiles/test_suite_gcm.aes128_en.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/CMakeFiles/test_suite_gcm.aes128_en.dir/depend

