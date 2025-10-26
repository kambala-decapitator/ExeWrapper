cmake_minimum_required(VERSION 3.19)
project(ExeWrapper LANGUAGES CXX)
if(NOT WIN32)
	message(FATAL_ERROR "only for Windows")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(UNICODE _UNICODE)
add_executable(exewrapper main.cpp)
install(TARGETS exewrapper
	RUNTIME DESTINATION .
)
