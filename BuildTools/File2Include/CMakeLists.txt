cmake_minimum_required(VERSION 3.27.0)  # for policy CMP0148

find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(PYTHONINTERP_FOUND ${Python3_Interpreter_FOUND} CACHE INTERNAL "Python Interpreter found")
set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE} CACHE INTERNAL "Python Executable")
set(FILE2STRING_PATH "${CMAKE_CURRENT_SOURCE_DIR}/script.py" CACHE INTERNAL "File2String utility")
