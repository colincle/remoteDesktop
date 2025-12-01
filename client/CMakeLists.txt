cmake_minimum_required(VERSION 3.16)
project(Client)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent WebSockets)

# Grab all .cpp and .hpp files recursively
file(GLOB_RECURSE SOURCES
	src/*.cpp
	include/*.hpp
	include/*.h
	src/**/*.hpp
)

add_executable(Client ${SOURCES})

# Apply compiler warnings globally, but only to your source files
target_compile_options(Client PRIVATE
	-Wall -Wextra
)

target_include_directories(Client PRIVATE
	include
	src
)

target_link_libraries(Client PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	Qt6::Network
	Qt6::WebSockets
)
