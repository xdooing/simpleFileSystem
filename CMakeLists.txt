cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)

project(SimpleFS)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")

# include dir
include_directories(${CMAKE_SOURCE_DIR}/src/include)

# lib source file
set(SFS_LIB_SOURCES
    src/library/disk.cpp
    src/library/fs.cpp
)

# shell source file
set(SFS_SHELL_SOURCES
    src/shell/sfssh.cpp
)

# output dir 
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/release")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
# shared lib
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
# static lib
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# static lib
add_library(sfs STATIC ${SFS_LIB_SOURCES})

# shell executable
add_executable(sfssh ${SFS_SHELL_SOURCES})
target_link_libraries(sfssh sfs)