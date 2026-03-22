cmake_minimum_required(VERSION 3.31)
project(GameOfLife)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

file(GLOB_RECURSE SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp"
)

function(add_local_or_fetch_dependency name local_dir)
	if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${local_dir}/CMakeLists.txt")
		message(STATUS "Using local dependency checkout: ${local_dir}")
		add_subdirectory("${local_dir}" "vendor/${name}")
	else()
		message(STATUS "Fetching ${name} because ${local_dir} is missing or incomplete")
		FetchContent_MakeAvailable(${name})
	endif()
endfunction()

# libs
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG fdd14e65b1c29e4e6df875fb5669ec00d6793531
	GIT_SHALLOW TRUE
)

set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
	glm
	GIT_REPOSITORY https://github.com/g-truc/glm.git
	GIT_TAG e7970a8b26732f1b0df9690f7180546f8c30e48e
	GIT_SHALLOW TRUE
)

add_local_or_fetch_dependency(glfw vendor/glfw)
add_local_or_fetch_dependency(glm vendor/glm)

add_library(glad vendor/glad/src/glad.c)

target_include_directories(glad PUBLIC vendor/glad/include)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(
				${CMAKE_PROJECT_NAME}
				PRIVATE
				${CMAKE_CURRENT_SOURCE_DIR}/src
				vendor/glad/include
)

target_link_libraries(
	${CMAKE_PROJECT_NAME}
	glfw glad glm::glm
)
