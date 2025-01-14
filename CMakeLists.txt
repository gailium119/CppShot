cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(CppShot VERSION 0.6.0)

IF( MSVC )
	SET ( CMAKE_EXE_LINKER_FLAGS /MANIFEST:NO )
ENDIF()

add_definitions(-DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0500 -D_WIN32_IE=0x0300)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	IF( MSVC )
		add_link_options(/OPT:REF /OPT:ICF /LTCG /INCREMENTAL:NO /SUBSYSTEM:WINDOWS)
	ELSE()
		add_compile_options(-O2 -s -mwindows)
	ENDIF()
endif()

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/res/resources.rc.in
	${CMAKE_CURRENT_BINARY_DIR}/res/resources.rc
	@ONLY)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
	${CMAKE_CURRENT_BINARY_DIR}/src/version.h
	@ONLY)

file(GLOB_RECURSE SOURCES
	src/*.cpp
	res/*.rc
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/res/resources.rc)
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src)

IF( DEFINED LINK_UNICOWS )
	target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/lib/libunicows.a)	
ENDIF()

target_link_libraries(${PROJECT_NAME} gdiplus advapi32 gdi32 user32 kernel32 comctl32 shell32)

set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

IF( MSVC )
	SET ( CMAKE_EXE_LINKER_FLAGS /MANIFEST:NO )
ELSE()
	target_link_libraries(${PROJECT_NAME} -static-libgcc -static-libstdc++)
ENDIF()
