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
include benchmark/strings/CMakeFiles/str_cat_benchmark.dir/depend.make

# Include the progress variables for this target.
include benchmark/strings/CMakeFiles/str_cat_benchmark.dir/progress.make

# Include the compile flags for this target's objects.
include benchmark/strings/CMakeFiles/str_cat_benchmark.dir/flags.make

benchmark/strings/CMakeFiles/str_cat_benchmark.dir/str_cat_benchmark.cc.o: benchmark/strings/CMakeFiles/str_cat_benchmark.dir/flags.make
benchmark/strings/CMakeFiles/str_cat_benchmark.dir/str_cat_benchmark.cc.o: ../benchmark/strings/str_cat_benchmark.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/liyinbin/github/gottingen/abel/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object benchmark/strings/CMakeFiles/str_cat_benchmark.dir/str_cat_benchmark.cc.o"
	cd /Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/strings && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/str_cat_benchmark.dir/str_cat_benchmark.cc.o -c /Users/liyinbin/github/gottingen/abel/benchmark/strings/str_cat_benchmark.cc

benchmark/strings/CMakeFiles/str_cat_benchmark.dir/str_cat_benchmark.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/str_cat_benchmark.dir/str_cat_benchmark.cc.i"
	cd /Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/strings && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/liyinbin/github/gottingen/abel/benchmark/strings/str_cat_benchmark.cc > CMakeFiles/str_cat_benchmark.dir/str_cat_benchmark.cc.i

benchmark/strings/CMakeFiles/str_cat_benchmark.dir/str_cat_benchmark.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/str_cat_benchmark.dir/str_cat_benchmark.cc.s"
	cd /Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/strings && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/liyinbin/github/gottingen/abel/benchmark/strings/str_cat_benchmark.cc -o CMakeFiles/str_cat_benchmark.dir/str_cat_benchmark.cc.s

# Object files for target str_cat_benchmark
str_cat_benchmark_OBJECTS = \
"CMakeFiles/str_cat_benchmark.dir/str_cat_benchmark.cc.o"

# External object files for target str_cat_benchmark
str_cat_benchmark_EXTERNAL_OBJECTS =

bin/str_cat_benchmark: benchmark/strings/CMakeFiles/str_cat_benchmark.dir/str_cat_benchmark.cc.o
bin/str_cat_benchmark: benchmark/strings/CMakeFiles/str_cat_benchmark.dir/build.make
bin/str_cat_benchmark: lib/libbenchmark.a
bin/str_cat_benchmark: lib/libbenchmark_main.a
bin/str_cat_benchmark: lib/libabel_static.a
bin/str_cat_benchmark: lib/libbenchmark.a
bin/str_cat_benchmark: benchmark/strings/CMakeFiles/str_cat_benchmark.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/liyinbin/github/gottingen/abel/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../bin/str_cat_benchmark"
	cd /Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/strings && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/str_cat_benchmark.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
benchmark/strings/CMakeFiles/str_cat_benchmark.dir/build: bin/str_cat_benchmark

.PHONY : benchmark/strings/CMakeFiles/str_cat_benchmark.dir/build

benchmark/strings/CMakeFiles/str_cat_benchmark.dir/clean:
	cd /Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/strings && $(CMAKE_COMMAND) -P CMakeFiles/str_cat_benchmark.dir/cmake_clean.cmake
.PHONY : benchmark/strings/CMakeFiles/str_cat_benchmark.dir/clean

benchmark/strings/CMakeFiles/str_cat_benchmark.dir/depend:
	cd /Users/liyinbin/github/gottingen/abel/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/liyinbin/github/gottingen/abel /Users/liyinbin/github/gottingen/abel/benchmark/strings /Users/liyinbin/github/gottingen/abel/cmake-build-debug /Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/strings /Users/liyinbin/github/gottingen/abel/cmake-build-debug/benchmark/strings/CMakeFiles/str_cat_benchmark.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : benchmark/strings/CMakeFiles/str_cat_benchmark.dir/depend

