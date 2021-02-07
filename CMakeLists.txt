cmake_minimum_required(VERSION 3.17)
project(inode)

set(CMAKE_CXX_STANDARD 14)

add_executable(inode main.cpp FileSystem.cpp FileSystem.cpp FileSystem.hpp structs.hpp consts.hpp INode.cpp INode.hpp MemoryIterator.cpp MemoryIterator.cpp MemoryIterator.hpp)