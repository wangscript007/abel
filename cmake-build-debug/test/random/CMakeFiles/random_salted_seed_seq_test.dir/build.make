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
CMAKE_COMMAND = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake

# The command to remove a file.
RM = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/liyinbin/github/gottingen/abel

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/liyinbin/github/gottingen/abel/cmake-build-debug

# Include any dependencies generated for this target.
include test/random/CMakeFiles/random_salted_seed_seq_test.dir/depend.make

# Include the progress variables for this target.
include test/random/CMakeFiles/random_salted_seed_seq_test.dir/progress.make

# Include the compile flags for this target's objects.
include test/random/CMakeFiles/random_salted_seed_seq_test.dir/flags.make

test/random/CMakeFiles/random_salted_seed_seq_test.dir/salted_seed_seq_test.cc.o: test/random/CMakeFiles/random_salted_seed_seq_test.dir/flags.make
test/random/CMakeFiles/random_salted_seed_seq_test.dir/salted_seed_seq_test.cc.o: ../test/random/salted_seed_seq_test.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/liyinbin/github/gottingen/abel/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object test/random/CMakeFiles/random_salted_seed_seq_test.dir/salted_seed_seq_test.cc.o"
	cd /Users/liyinbin/github/gottingen/abel/cmake-build-debug/test/random && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/random_salted_seed_seq_test.dir/salted_seed_seq_test.cc.o -c /Users/liyinbin/github/gottingen/abel/test/random/salted_seed_seq_test.cc

test/random/CMakeFiles/random_salted_seed_seq_test.dir/salted_seed_seq_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/random_salted_seed_seq_test.dir/salted_seed_seq_test.cc.i"
	cd /Users/liyinbin/github/gottingen/abel/cmake-build-debug/test/random && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/liyinbin/github/gottingen/abel/test/random/salted_seed_seq_test.cc > CMakeFiles/random_salted_seed_seq_test.dir/salted_seed_seq_test.cc.i

test/random/CMakeFiles/random_salted_seed_seq_test.dir/salted_seed_seq_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/random_salted_seed_seq_test.dir/salted_seed_seq_test.cc.s"
	cd /Users/liyinbin/github/gottingen/abel/cmake-build-debug/test/random && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/liyinbin/github/gottingen/abel/test/random/salted_seed_seq_test.cc -o CMakeFiles/random_salted_seed_seq_test.dir/salted_seed_seq_test.cc.s

# Object files for target random_salted_seed_seq_test
random_salted_seed_seq_test_OBJECTS = \
"CMakeFiles/random_salted_seed_seq_test.dir/salted_seed_seq_test.cc.o"

# External object files for target random_salted_seed_seq_test
random_salted_seed_seq_test_EXTERNAL_OBJECTS =

bin/random_salted_seed_seq_test: test/random/CMakeFiles/random_salted_seed_seq_test.dir/salted_seed_seq_test.cc.o
bin/random_salted_seed_seq_test: test/random/CMakeFiles/random_salted_seed_seq_test.dir/build.make
bin/random_salted_seed_seq_test: lib/libgtestd.a
bin/random_salted_seed_seq_test: lib/libgmockd.a
bin/random_salted_seed_seq_test: lib/libgtest_maind.a
bin/random_salted_seed_seq_test: lib/libtesting_static.a
bin/random_salted_seed_seq_test: lib/libabel_static.a
bin/random_salted_seed_seq_test: lib/libgmockd.a
bin/random_salted_seed_seq_test: lib/libgtestd.a
bin/random_salted_seed_seq_test: test/random/CMakeFiles/random_salted_seed_seq_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/liyinbin/github/gottingen/abel/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../bin/random_salted_seed_seq_test"
	cd /Users/liyinbin/github/gottingen/abel/cmake-build-debug/test/random && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/random_salted_seed_seq_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
test/random/CMakeFiles/random_salted_seed_seq_test.dir/build: bin/random_salted_seed_seq_test

.PHONY : test/random/CMakeFiles/random_salted_seed_seq_test.dir/build

test/random/CMakeFiles/random_salted_seed_seq_test.dir/clean:
	cd /Users/liyinbin/github/gottingen/abel/cmake-build-debug/test/random && $(CMAKE_COMMAND) -P CMakeFiles/random_salted_seed_seq_test.dir/cmake_clean.cmake
.PHONY : test/random/CMakeFiles/random_salted_seed_seq_test.dir/clean

test/random/CMakeFiles/random_salted_seed_seq_test.dir/depend:
	cd /Users/liyinbin/github/gottingen/abel/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/liyinbin/github/gottingen/abel /Users/liyinbin/github/gottingen/abel/test/random /Users/liyinbin/github/gottingen/abel/cmake-build-debug /Users/liyinbin/github/gottingen/abel/cmake-build-debug/test/random /Users/liyinbin/github/gottingen/abel/cmake-build-debug/test/random/CMakeFiles/random_salted_seed_seq_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/random/CMakeFiles/random_salted_seed_seq_test.dir/depend

