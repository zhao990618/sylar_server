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
include CMakeFiles/sylar.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/sylar.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/sylar.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/sylar.dir/flags.make

CMakeFiles/sylar.dir/src/config.cpp.o: CMakeFiles/sylar.dir/flags.make
CMakeFiles/sylar.dir/src/config.cpp.o: ../src/config.cpp
CMakeFiles/sylar.dir/src/config.cpp.o: CMakeFiles/sylar.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zhaoyangfan/LinuxStudio/server/sylar/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/sylar.dir/src/config.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/sylar.dir/src/config.cpp.o -MF CMakeFiles/sylar.dir/src/config.cpp.o.d -o CMakeFiles/sylar.dir/src/config.cpp.o -c /home/zhaoyangfan/LinuxStudio/server/sylar/src/config.cpp

CMakeFiles/sylar.dir/src/config.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sylar.dir/src/config.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zhaoyangfan/LinuxStudio/server/sylar/src/config.cpp > CMakeFiles/sylar.dir/src/config.cpp.i

CMakeFiles/sylar.dir/src/config.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sylar.dir/src/config.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zhaoyangfan/LinuxStudio/server/sylar/src/config.cpp -o CMakeFiles/sylar.dir/src/config.cpp.s

CMakeFiles/sylar.dir/src/fiber.cpp.o: CMakeFiles/sylar.dir/flags.make
CMakeFiles/sylar.dir/src/fiber.cpp.o: ../src/fiber.cpp
CMakeFiles/sylar.dir/src/fiber.cpp.o: CMakeFiles/sylar.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zhaoyangfan/LinuxStudio/server/sylar/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/sylar.dir/src/fiber.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/sylar.dir/src/fiber.cpp.o -MF CMakeFiles/sylar.dir/src/fiber.cpp.o.d -o CMakeFiles/sylar.dir/src/fiber.cpp.o -c /home/zhaoyangfan/LinuxStudio/server/sylar/src/fiber.cpp

CMakeFiles/sylar.dir/src/fiber.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sylar.dir/src/fiber.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zhaoyangfan/LinuxStudio/server/sylar/src/fiber.cpp > CMakeFiles/sylar.dir/src/fiber.cpp.i

CMakeFiles/sylar.dir/src/fiber.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sylar.dir/src/fiber.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zhaoyangfan/LinuxStudio/server/sylar/src/fiber.cpp -o CMakeFiles/sylar.dir/src/fiber.cpp.s

CMakeFiles/sylar.dir/src/hook.cpp.o: CMakeFiles/sylar.dir/flags.make
CMakeFiles/sylar.dir/src/hook.cpp.o: ../src/hook.cpp
CMakeFiles/sylar.dir/src/hook.cpp.o: CMakeFiles/sylar.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zhaoyangfan/LinuxStudio/server/sylar/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/sylar.dir/src/hook.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/sylar.dir/src/hook.cpp.o -MF CMakeFiles/sylar.dir/src/hook.cpp.o.d -o CMakeFiles/sylar.dir/src/hook.cpp.o -c /home/zhaoyangfan/LinuxStudio/server/sylar/src/hook.cpp

CMakeFiles/sylar.dir/src/hook.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sylar.dir/src/hook.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zhaoyangfan/LinuxStudio/server/sylar/src/hook.cpp > CMakeFiles/sylar.dir/src/hook.cpp.i

CMakeFiles/sylar.dir/src/hook.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sylar.dir/src/hook.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zhaoyangfan/LinuxStudio/server/sylar/src/hook.cpp -o CMakeFiles/sylar.dir/src/hook.cpp.s

CMakeFiles/sylar.dir/src/iomanager.cpp.o: CMakeFiles/sylar.dir/flags.make
CMakeFiles/sylar.dir/src/iomanager.cpp.o: ../src/iomanager.cpp
CMakeFiles/sylar.dir/src/iomanager.cpp.o: CMakeFiles/sylar.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zhaoyangfan/LinuxStudio/server/sylar/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/sylar.dir/src/iomanager.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/sylar.dir/src/iomanager.cpp.o -MF CMakeFiles/sylar.dir/src/iomanager.cpp.o.d -o CMakeFiles/sylar.dir/src/iomanager.cpp.o -c /home/zhaoyangfan/LinuxStudio/server/sylar/src/iomanager.cpp

CMakeFiles/sylar.dir/src/iomanager.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sylar.dir/src/iomanager.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zhaoyangfan/LinuxStudio/server/sylar/src/iomanager.cpp > CMakeFiles/sylar.dir/src/iomanager.cpp.i

CMakeFiles/sylar.dir/src/iomanager.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sylar.dir/src/iomanager.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zhaoyangfan/LinuxStudio/server/sylar/src/iomanager.cpp -o CMakeFiles/sylar.dir/src/iomanager.cpp.s

CMakeFiles/sylar.dir/src/log.cpp.o: CMakeFiles/sylar.dir/flags.make
CMakeFiles/sylar.dir/src/log.cpp.o: ../src/log.cpp
CMakeFiles/sylar.dir/src/log.cpp.o: CMakeFiles/sylar.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zhaoyangfan/LinuxStudio/server/sylar/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/sylar.dir/src/log.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/sylar.dir/src/log.cpp.o -MF CMakeFiles/sylar.dir/src/log.cpp.o.d -o CMakeFiles/sylar.dir/src/log.cpp.o -c /home/zhaoyangfan/LinuxStudio/server/sylar/src/log.cpp

CMakeFiles/sylar.dir/src/log.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sylar.dir/src/log.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zhaoyangfan/LinuxStudio/server/sylar/src/log.cpp > CMakeFiles/sylar.dir/src/log.cpp.i

CMakeFiles/sylar.dir/src/log.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sylar.dir/src/log.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zhaoyangfan/LinuxStudio/server/sylar/src/log.cpp -o CMakeFiles/sylar.dir/src/log.cpp.s

CMakeFiles/sylar.dir/src/scheduler.cpp.o: CMakeFiles/sylar.dir/flags.make
CMakeFiles/sylar.dir/src/scheduler.cpp.o: ../src/scheduler.cpp
CMakeFiles/sylar.dir/src/scheduler.cpp.o: CMakeFiles/sylar.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zhaoyangfan/LinuxStudio/server/sylar/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/sylar.dir/src/scheduler.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/sylar.dir/src/scheduler.cpp.o -MF CMakeFiles/sylar.dir/src/scheduler.cpp.o.d -o CMakeFiles/sylar.dir/src/scheduler.cpp.o -c /home/zhaoyangfan/LinuxStudio/server/sylar/src/scheduler.cpp

CMakeFiles/sylar.dir/src/scheduler.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sylar.dir/src/scheduler.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zhaoyangfan/LinuxStudio/server/sylar/src/scheduler.cpp > CMakeFiles/sylar.dir/src/scheduler.cpp.i

CMakeFiles/sylar.dir/src/scheduler.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sylar.dir/src/scheduler.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zhaoyangfan/LinuxStudio/server/sylar/src/scheduler.cpp -o CMakeFiles/sylar.dir/src/scheduler.cpp.s

CMakeFiles/sylar.dir/src/thread.cpp.o: CMakeFiles/sylar.dir/flags.make
CMakeFiles/sylar.dir/src/thread.cpp.o: ../src/thread.cpp
CMakeFiles/sylar.dir/src/thread.cpp.o: CMakeFiles/sylar.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zhaoyangfan/LinuxStudio/server/sylar/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object CMakeFiles/sylar.dir/src/thread.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/sylar.dir/src/thread.cpp.o -MF CMakeFiles/sylar.dir/src/thread.cpp.o.d -o CMakeFiles/sylar.dir/src/thread.cpp.o -c /home/zhaoyangfan/LinuxStudio/server/sylar/src/thread.cpp

CMakeFiles/sylar.dir/src/thread.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sylar.dir/src/thread.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zhaoyangfan/LinuxStudio/server/sylar/src/thread.cpp > CMakeFiles/sylar.dir/src/thread.cpp.i

CMakeFiles/sylar.dir/src/thread.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sylar.dir/src/thread.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zhaoyangfan/LinuxStudio/server/sylar/src/thread.cpp -o CMakeFiles/sylar.dir/src/thread.cpp.s

CMakeFiles/sylar.dir/src/util.cpp.o: CMakeFiles/sylar.dir/flags.make
CMakeFiles/sylar.dir/src/util.cpp.o: ../src/util.cpp
CMakeFiles/sylar.dir/src/util.cpp.o: CMakeFiles/sylar.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zhaoyangfan/LinuxStudio/server/sylar/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building CXX object CMakeFiles/sylar.dir/src/util.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/sylar.dir/src/util.cpp.o -MF CMakeFiles/sylar.dir/src/util.cpp.o.d -o CMakeFiles/sylar.dir/src/util.cpp.o -c /home/zhaoyangfan/LinuxStudio/server/sylar/src/util.cpp

CMakeFiles/sylar.dir/src/util.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sylar.dir/src/util.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zhaoyangfan/LinuxStudio/server/sylar/src/util.cpp > CMakeFiles/sylar.dir/src/util.cpp.i

CMakeFiles/sylar.dir/src/util.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sylar.dir/src/util.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zhaoyangfan/LinuxStudio/server/sylar/src/util.cpp -o CMakeFiles/sylar.dir/src/util.cpp.s

CMakeFiles/sylar.dir/src/timer.cpp.o: CMakeFiles/sylar.dir/flags.make
CMakeFiles/sylar.dir/src/timer.cpp.o: ../src/timer.cpp
CMakeFiles/sylar.dir/src/timer.cpp.o: CMakeFiles/sylar.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/zhaoyangfan/LinuxStudio/server/sylar/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building CXX object CMakeFiles/sylar.dir/src/timer.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/sylar.dir/src/timer.cpp.o -MF CMakeFiles/sylar.dir/src/timer.cpp.o.d -o CMakeFiles/sylar.dir/src/timer.cpp.o -c /home/zhaoyangfan/LinuxStudio/server/sylar/src/timer.cpp

CMakeFiles/sylar.dir/src/timer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sylar.dir/src/timer.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/zhaoyangfan/LinuxStudio/server/sylar/src/timer.cpp > CMakeFiles/sylar.dir/src/timer.cpp.i

CMakeFiles/sylar.dir/src/timer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sylar.dir/src/timer.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/zhaoyangfan/LinuxStudio/server/sylar/src/timer.cpp -o CMakeFiles/sylar.dir/src/timer.cpp.s

# Object files for target sylar
sylar_OBJECTS = \
"CMakeFiles/sylar.dir/src/config.cpp.o" \
"CMakeFiles/sylar.dir/src/fiber.cpp.o" \
"CMakeFiles/sylar.dir/src/hook.cpp.o" \
"CMakeFiles/sylar.dir/src/iomanager.cpp.o" \
"CMakeFiles/sylar.dir/src/log.cpp.o" \
"CMakeFiles/sylar.dir/src/scheduler.cpp.o" \
"CMakeFiles/sylar.dir/src/thread.cpp.o" \
"CMakeFiles/sylar.dir/src/util.cpp.o" \
"CMakeFiles/sylar.dir/src/timer.cpp.o"

# External object files for target sylar
sylar_EXTERNAL_OBJECTS =

../lib/libsylar.so: CMakeFiles/sylar.dir/src/config.cpp.o
../lib/libsylar.so: CMakeFiles/sylar.dir/src/fiber.cpp.o
../lib/libsylar.so: CMakeFiles/sylar.dir/src/hook.cpp.o
../lib/libsylar.so: CMakeFiles/sylar.dir/src/iomanager.cpp.o
../lib/libsylar.so: CMakeFiles/sylar.dir/src/log.cpp.o
../lib/libsylar.so: CMakeFiles/sylar.dir/src/scheduler.cpp.o
../lib/libsylar.so: CMakeFiles/sylar.dir/src/thread.cpp.o
../lib/libsylar.so: CMakeFiles/sylar.dir/src/util.cpp.o
../lib/libsylar.so: CMakeFiles/sylar.dir/src/timer.cpp.o
../lib/libsylar.so: CMakeFiles/sylar.dir/build.make
../lib/libsylar.so: CMakeFiles/sylar.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/zhaoyangfan/LinuxStudio/server/sylar/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Linking CXX shared library ../lib/libsylar.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/sylar.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/sylar.dir/build: ../lib/libsylar.so
.PHONY : CMakeFiles/sylar.dir/build

CMakeFiles/sylar.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/sylar.dir/cmake_clean.cmake
.PHONY : CMakeFiles/sylar.dir/clean

CMakeFiles/sylar.dir/depend:
	cd /home/zhaoyangfan/LinuxStudio/server/sylar/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/zhaoyangfan/LinuxStudio/server/sylar /home/zhaoyangfan/LinuxStudio/server/sylar /home/zhaoyangfan/LinuxStudio/server/sylar/build /home/zhaoyangfan/LinuxStudio/server/sylar/build /home/zhaoyangfan/LinuxStudio/server/sylar/build/CMakeFiles/sylar.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/sylar.dir/depend

