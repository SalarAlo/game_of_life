cmake_minimum_required(VERSION 3.31)
project(GameOfLife)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp"
)

# libs
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/glm)

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
	glfw glad glm
)
