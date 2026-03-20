// noincludeformat

#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_set>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"

static void framebuffer_size_callback(GLFWwindow* window, int height, int width);

static const char* vertex_shader_source {
	R"glsl(
#version 460 core

layout (location = 0) in vec2 a_position;

uniform vec2 u_viewport_size;

void main() {
	vec2 centered_position = a_position + vec2(0.5);
	vec2 normalized_position = centered_position / u_viewport_size;

	vec2 clip_position = vec2(
		normalized_position.x * 2.0 - 1.0,
		1.0 - normalized_position.y * 2.0
	);

	gl_Position = vec4(clip_position, 0.0, 1.0);
}

)glsl"
};

static const char* fragment_shader_source { R"glsl(
#version 460 core

out vec4 frag_color;

void main() {
	frag_color = vec4(1.0);
}


)glsl" };

// PixelManager -> add_pixel(int x, int y)
class PixelManager {
public:
	PixelManager() {
		create_shader_program();
		m_uniform_viewport_size_location = glGetUniformLocation(m_shader_program, "u_viewport_size");

		glGenVertexArrays(1, &m_vertex_array);
		glGenBuffers(1, &m_vertex_buffer);

		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		glPointSize(m_unit_size);
	}

	~PixelManager() {
		if (m_shader_program)
			glDeleteProgram(m_shader_program);

		if (m_vertex_array)
			glDeleteVertexArrays(1, &m_vertex_array);

		if (m_vertex_buffer)
			glDeleteBuffers(1, &m_vertex_buffer);
	}

	void flush_render() {
		if (m_pixels.empty())
			return;

		std::array<GLint, 4> viewport {};
		glGetIntegerv(GL_VIEWPORT, viewport.data());

		glUseProgram(m_shader_program);
		glUniform2f(m_uniform_viewport_size_location, static_cast<float>(viewport[2]), static_cast<float>(viewport[3]));

		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, m_pixels.size() * sizeof(glm::vec2), m_pixels.data(), GL_DYNAMIC_DRAW);

		glDrawArrays(GL_POINTS, 0, m_pixels.size());

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glUseProgram(0);

		m_pixels.clear();
	}

	void add_pixel(int x, int y) {
		m_pixels.push_back(glm::vec2 { x, y } * m_unit_size);
	}

private:
	void create_shader_program() {
		GLuint vertex_shader { compile_shader(GL_VERTEX_SHADER, vertex_shader_source) };
		GLuint fragment_shader { compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source) };

		m_shader_program = glCreateProgram();

		glAttachShader(m_shader_program, vertex_shader);
		glAttachShader(m_shader_program, fragment_shader);

		glLinkProgram(m_shader_program);

		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
	}

	GLuint compile_shader(GLenum type, const char* source) {
		GLuint shader { glCreateShader(type) };
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);

		return shader;
	}

private:
	std::vector<glm::vec2> m_pixels {};

	float m_unit_size { 5.0f };

	GLuint m_vertex_buffer {};
	GLuint m_vertex_array {};
	GLuint m_shader_program {};

	GLint m_uniform_viewport_size_location {};
};

struct Vec2Hash {
	size_t operator()(const glm::ivec2& v) const {
		return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
	}
};

class GameOfLife {
public:
	using GameOfLifeGeneration = std::unordered_set<glm::vec2, Vec2Hash>;

public:
	GameOfLife() {
		std::mt19937 generator {};
		std::uniform_int_distribution<> range { 0, 160 };
		for (size_t i {}; i < 160 * 80; i++) {
			m_entities.emplace(range(generator), range(generator));
		}
	}

	void inject(PixelManager& manager) {
		for (const auto& entity : m_entities)
			manager.add_pixel(entity.x, entity.y);
	};

	void update(float dt) {
		static float accumulator { 0.0f };
		accumulator += dt;

		if (accumulator <= m_update_time)
			return;

		const auto current_generation { m_entities };

		std::unordered_map<glm::vec2, std::size_t, Vec2Hash> dead_neighbours_count {};

		m_entities.clear();

		for (const auto& entity : current_generation) {
			const auto neighbour_count { get_neighbour_count(entity, current_generation) };

			if (neighbour_count == 2 || neighbour_count == 3)
				m_entities.insert(entity);

			for (const auto& dir : get_all_possible_directions()) {
				if (!current_generation.contains(entity - dir))
					dead_neighbours_count[entity - dir]++;
			}
		}

		for (const auto& [position, neighbour_count] : dead_neighbours_count) {
			if (neighbour_count == 3)
				m_entities.insert(position);
		}

		accumulator = 0;
	};

	size_t get_neighbour_count(glm::vec2 entity, const GameOfLifeGeneration& generation) {
		size_t neighbour_count {};

		for (const auto& dir : get_all_possible_directions()) {
			if (generation.contains(entity - dir)) {
				neighbour_count++;
			}
		}

		return neighbour_count;
	}

	std::vector<glm::vec2> get_all_possible_directions() {
		std::vector<glm::vec2> dirs {};

		for (int x { -1 }; x <= 1; x++) {
			for (int y { -1 }; y <= 1; y++) {
				if (x == 0 && y == 0)
					continue;
				dirs.emplace_back(x, y);
			}
		}

		return dirs;
	}

private:
	GameOfLifeGeneration m_entities;
	float m_update_time { .001f };
};

class ScreenWindow {
public:
	ScreenWindow(std::string_view title, glm::vec2 size)
	    : m_size(size)
	    , m_title(title) {
		initialize_glfw();

		m_window = glfwCreateWindow(size.x, size.y, m_title.data(), nullptr, nullptr);
		if (!m_window)
			throw std::runtime_error("failed to create a glfw window.");

		glfwMakeContextCurrent(m_window);
		glfwSetWindowUserPointer(m_window, this);

		glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);

		initialize_glad();

		int framebuffer_width {};
		int framebuffer_height {};
		glfwGetFramebufferSize(m_window, &framebuffer_width, &framebuffer_height);
		set_size({ framebuffer_width, framebuffer_height });
	}

	~ScreenWindow() {
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	void set_size(glm::vec2 new_size) {
		m_size = new_size;
		glViewport(0, 0, new_size.x, new_size.y);
	}

	bool should_close() const {
		return glfwWindowShouldClose(m_window);
	}

	void swap_buffers() const {
		glfwSwapBuffers(m_window);
	}

private:
	void initialize_glfw() {
		if (s_glfw_initialized) {
			return;
		}
		if (!glfwInit()) {
			throw std::runtime_error("failed to init glfw.");
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		s_glfw_initialized = true;
	}

	void initialize_glad() {
		if (s_glad_initialized)
			return;

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			throw std::runtime_error("glad failed to initialize.");
		}

		s_glad_initialized = true;
	}

private:
	inline static bool s_glfw_initialized { false };
	inline static bool s_glad_initialized { false };

private:
	glm::vec2 m_size;
	std::string m_title;
	GLFWwindow* m_window;
};

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	auto screen_window { static_cast<ScreenWindow*>(glfwGetWindowUserPointer(window)) };
	screen_window->set_size({ width, height });
}

int main() {
	ScreenWindow window { "Game of Life", { 100, 100 } };
	PixelManager pixel_manager {};
	GameOfLife game_of_life {};

	glClearColor(0, 0, 0, 0);

	float last_frame_time { static_cast<float>(glfwGetTime()) };
	float dt {};

	while (!window.should_close()) {
		const float current_frame_time { static_cast<float>(glfwGetTime()) };
		dt = current_frame_time - last_frame_time;
		last_frame_time = current_frame_time;

		glClear(GL_COLOR_BUFFER_BIT);

		game_of_life.inject(pixel_manager);
		pixel_manager.flush_render();

		game_of_life.update(dt);

		window.swap_buffers();
		glfwPollEvents();
	}

	return 0;
}
