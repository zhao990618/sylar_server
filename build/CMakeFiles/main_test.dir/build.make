# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Produce verbose output by default.
VERBOSE = 1

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/zhaoyangfan/LinuxStudio/server/sylar

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/zhaoyangfan/LinuxStudio/server/sylar/build

# Include any dependencies generated for this target.
include CMakeFiles/main_test.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/main_test.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/main_test.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/main_test.dir/flags.make

CMakeFiles/main_test.dir/tests/test.cpp.o: CMakeFiles/main_test.dir/flags.make
CMakeFiles/main_test.dir/tests/test.cpp.o: ../tests/test.cpp
CMakeFiles/main_test.dir/tests/test.cpp.o: CMakeFiles/main_test.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zhaoyangfan/LinuxStudio/server/sylar/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/main_test.dir/tests/test.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/main_test.dir/tests/test.cpp.o -MF CMakeFiles/main_test.dir/tests/test.cpp.o.d -o CMakeFiles/main_test.dir/tests/test.cpp.o -c /home/zhaoyangfan/LinuxStudio/server/sylar/tests/test.cpp

CMakeFiles/main_test.dir/tests/test.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/main_test.dir/tests/test.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zhaoyangfan/LinuxStudio/server/sylar/tests/test.cpp > CMakeFiles/main_test.dir/tests/test.cpp.i

CMakeFiles/main_test.dir/tests/test.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/main_test.dir/tests/test.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zhaoyangfan/LinuxStudio/server/sylar/tests/test.cpp -o CMakeFiles/main_test.dir/tests/test.cpp.s

# Object files for target main_test
main_test_OBJECTS = \
"CMakeFiles/main_test.dir/tests/test.cpp.o"

# External object files for target main_test
main_test_EXTERNAL_OBJECTS =

../bin/main_test: CMakeFiles/main_test.dir/tests/test.cpp.o
../bin/main_test: CMakeFiles/main_test.dir/build.make
../bin/main_test: ../lib/libsylar.so
../bin/main_test: CMakeFiles/main_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/zhaoyangfan/LinuxStudio/server/sylar/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../bin/main_test"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/main_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/main_test.dir/build: ../bin/main_test
.PHONY : CMakeFiles/main_test.dir/build

CMakeFiles/main_test.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/main_test.dir/cmake_clean.cmake
.PHONY : CMakeFiles/main_test.dir/clean

CMakeFiles/main_test.dir/depend:
	cd /home/zhaoyangfan/LinuxStudio/server/sylar/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zhaoyangfan/LinuxStudio/server/sylar /home/zhaoyangfan/LinuxStudio/server/sylar /home/zhaoyangfan/LinuxStudio/server/sylar/build /home/zhaoyangfan/LinuxStudio/server/sylar/build /home/zhaoyangfan/LinuxStudio/server/sylar/build/CMakeFiles/main_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/main_test.dir/depend

