cmake_minimum_required(VERSION 3.27.0)  # for policy CMP0148

find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(FILE2STRING_PATH "${CMAKE_CURRENT_SOURCE_DIR}/script.py" CACHE INTERNAL "File2String utility")
