# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.15

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
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.15.4/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.15.4/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/JinShang/Dropbox/15-645/P1/bustub

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/JinShang/Dropbox/15-645/P1/bustub

# Utility rule file for build-tests.

# Include the progress variables for this target.
include test/CMakeFiles/build-tests.dir/progress.make

test/CMakeFiles/build-tests:
	cd /Users/JinShang/Dropbox/15-645/P1/bustub/test && /usr/local/Cellar/cmake/3.15.4/bin/ctest --show-only

build-tests: test/CMakeFiles/build-tests
build-tests: test/CMakeFiles/build-tests.dir/build.make

.PHONY : build-tests

# Rule to build all files generated by this target.
test/CMakeFiles/build-tests.dir/build: build-tests

.PHONY : test/CMakeFiles/build-tests.dir/build

test/CMakeFiles/build-tests.dir/clean:
	cd /Users/JinShang/Dropbox/15-645/P1/bustub/test && $(CMAKE_COMMAND) -P CMakeFiles/build-tests.dir/cmake_clean.cmake
.PHONY : test/CMakeFiles/build-tests.dir/clean

test/CMakeFiles/build-tests.dir/depend:
	cd /Users/JinShang/Dropbox/15-645/P1/bustub && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/JinShang/Dropbox/15-645/P1/bustub /Users/JinShang/Dropbox/15-645/P1/bustub/test /Users/JinShang/Dropbox/15-645/P1/bustub /Users/JinShang/Dropbox/15-645/P1/bustub/test /Users/JinShang/Dropbox/15-645/P1/bustub/test/CMakeFiles/build-tests.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/CMakeFiles/build-tests.dir/depend

