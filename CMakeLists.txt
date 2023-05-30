cmake_minimum_required(VERSION 3.0)
project(SYLAR)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Wno-unused-function")
# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Wno-unused-function")
# set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11")
set(CMAKE_BUILD_TYPE Debug)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)
message(STATUS "This is BINARY dir" ${CMAKE_BINARY_DIR})
message(STATUS "This is SOURCE dir" ${CMAKE_SOURCE_DIR})

# 添加头文件地址
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(/usr/local/include)
include_directories(/usr/include)

# 连接路径
link_directories(/usr/local/lib)

set(LIB_SRC
    src/address.cpp
    src/config.cpp
    src/fd_manager.cpp
    src/fiber.cpp
    src/hook.cpp
    src/iomanager.cpp
    src/log.cpp  
    src/socket.cpp
    src/scheduler.cpp 
    src/thread.cpp
    src/util.cpp
    src/timer.cpp 
)

# add_library(sylar SHARED ${CMAKE_SOURCE_DIR}/src/log.cpp)
add_library(sylar SHARED ${CMAKE_SOURCE_DIR}/${LIB_SRC})
#add_library(sylar_static STATIC $(LIB_SRC))

find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)

set(LIBS
    sylar
    dl
    pthread
    yaml-cpp
)

add_executable(main_test tests/test.cpp)
add_dependencies(main_test sylar)
target_link_libraries(main_test ${LIBS})

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config sylar)
target_link_libraries(test_config ${LIBS})

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread sylar)
target_link_libraries(test_thread ${LIBS})

add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util sylar)
target_link_libraries(test_util ${LIBS})

add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber sylar)
target_link_libraries(test_fiber ${LIBS})

add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler sylar)
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_iomanager tests/test_iomanager.cpp)
add_dependencies(test_iomanager sylar)
target_link_libraries(test_iomanager ${LIBS})

add_executable(test_hook tests/test_hook.cpp)
add_dependencies(test_hook sylar)
target_link_libraries(test_hook ${LIBS})

add_executable(test_address tests/test_address.cpp)
add_dependencies(test_address sylar)
target_link_libraries(test_address ${LIBS})

add_executable(test_socket tests/test_socket.cpp)
add_dependencies(test_socket sylar)
target_link_libraries(test_socket ${LIBS})

