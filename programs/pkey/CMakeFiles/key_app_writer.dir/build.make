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
include programs/pkey/CMakeFiles/key_app_writer.dir/depend.make

# Include the progress variables for this target.
include programs/pkey/CMakeFiles/key_app_writer.dir/progress.make

# Include the compile flags for this target's objects.
include programs/pkey/CMakeFiles/key_app_writer.dir/flags.make

programs/pkey/CMakeFiles/key_app_writer.dir/key_app_writer.c.o: programs/pkey/CMakeFiles/key_app_writer.dir/flags.make
programs/pkey/CMakeFiles/key_app_writer.dir/key_app_writer.c.o: programs/pkey/key_app_writer.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/jordan/mbedtls/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object programs/pkey/CMakeFiles/key_app_writer.dir/key_app_writer.c.o"
	cd /home/jordan/mbedtls/programs/pkey && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/key_app_writer.dir/key_app_writer.c.o   -c /home/jordan/mbedtls/programs/pkey/key_app_writer.c

programs/pkey/CMakeFiles/key_app_writer.dir/key_app_writer.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/key_app_writer.dir/key_app_writer.c.i"
	cd /home/jordan/mbedtls/programs/pkey && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/jordan/mbedtls/programs/pkey/key_app_writer.c > CMakeFiles/key_app_writer.dir/key_app_writer.c.i

programs/pkey/CMakeFiles/key_app_writer.dir/key_app_writer.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/key_app_writer.dir/key_app_writer.c.s"
	cd /home/jordan/mbedtls/programs/pkey && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/jordan/mbedtls/programs/pkey/key_app_writer.c -o CMakeFiles/key_app_writer.dir/key_app_writer.c.s

# Object files for target key_app_writer
key_app_writer_OBJECTS = \
"CMakeFiles/key_app_writer.dir/key_app_writer.c.o"

# External object files for target key_app_writer
key_app_writer_EXTERNAL_OBJECTS =

programs/pkey/key_app_writer: programs/pkey/CMakeFiles/key_app_writer.dir/key_app_writer.c.o
programs/pkey/key_app_writer: programs/pkey/CMakeFiles/key_app_writer.dir/build.make
programs/pkey/key_app_writer: library/libmbedtls.so.2.4.2
programs/pkey/key_app_writer: library/libmbedx509.so.2.4.2
programs/pkey/key_app_writer: library/libmbedcrypto.so.2.4.2
programs/pkey/key_app_writer: programs/pkey/CMakeFiles/key_app_writer.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/jordan/mbedtls/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable key_app_writer"
	cd /home/jordan/mbedtls/programs/pkey && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/key_app_writer.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
programs/pkey/CMakeFiles/key_app_writer.dir/build: programs/pkey/key_app_writer

.PHONY : programs/pkey/CMakeFiles/key_app_writer.dir/build

programs/pkey/CMakeFiles/key_app_writer.dir/clean:
	cd /home/jordan/mbedtls/programs/pkey && $(CMAKE_COMMAND) -P CMakeFiles/key_app_writer.dir/cmake_clean.cmake
.PHONY : programs/pkey/CMakeFiles/key_app_writer.dir/clean

programs/pkey/CMakeFiles/key_app_writer.dir/depend:
	cd /home/jordan/mbedtls && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/jordan/mbedtls /home/jordan/mbedtls/programs/pkey /home/jordan/mbedtls /home/jordan/mbedtls/programs/pkey /home/jordan/mbedtls/programs/pkey/CMakeFiles/key_app_writer.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : programs/pkey/CMakeFiles/key_app_writer.dir/depend

