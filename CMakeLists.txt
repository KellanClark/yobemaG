cmake_minimum_required(VERSION 3.10)
project(yoBemaG)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(SDL2 REQUIRED)

include_directories(
	${PROJECT_SOURCE_DIR}/include/
	${SDL2_INCLUDE_DIRS}
	modules/imgui/
	modules/imgui/examples/libs/gl3w/
)

add_executable(yobemag
	src/apu.cpp
	src/cartridge.cpp
	src/cpu.cpp
	src/gameboy.cpp
	src/main.cpp
	src/ppu.cpp
	src/ram.cpp
	src/sm83core.cpp
	src/timer.cpp

	modules/imgui/imgui_draw.cpp
	modules/imgui/imgui_demo.cpp
	modules/imgui/imgui_tables.cpp
	modules/imgui/imgui_widgets.cpp
	modules/imgui/imgui.cpp
	modules/imgui/backends/imgui_impl_sdl.cpp
	modules/imgui/backends/imgui_impl_opengl3.cpp
	modules/imgui/examples/libs/gl3w/GL/gl3w.c
)

target_compile_options(yobemag PRIVATE
	-MMD
	-MP
	-Wall
#[[	-g
	-fsanitize=address
	-fsanitize=leak
	-fsanitize=undefined
	-fsanitize=pointer-compare
	-fsanitize=pointer-subtract
	-fstack-protector-all]]
)

target_link_libraries(yobemag PRIVATE
	${SDL2_LIBRARIES}
	-lstdc++
	-lSDL2
	-ldl
	-lGL
	-lm
#[[	-fsanitize=address
	-fsanitize=leak
	-fsanitize=undefined
	-fsanitize=pointer-compare
	-fsanitize=pointer-subtract
	-fstack-protector-all]]
)