#include "shader.hpp"

#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <vector>

#ifndef NDEBUG
#include <stdexcept>
#endif

shader::shader(char const *vertex_src, char const *fragment_src)
{
    from_src(vertex_src, fragment_src);
}

void shader::from_src(char const *vertex_src, char const *fragment_src)
{
    program_id = glCreateProgram();

    unsigned int vertex_shader = compile(GL_VERTEX_SHADER, vertex_src);
    unsigned int fragment_shader = compile(GL_FRAGMENT_SHADER, fragment_src);

    glLinkProgram(program_id);
    glValidateProgram(program_id);

    glDeleteShader(vertex_shader);void fill_tex_slots(char const *name, unsigned int num_slots=32);
    glDeleteShader(fragment_shader);
}

void shader::from_file(char const *vertex_path, char const *fragment_path)
{
    std::ifstream vertex_file(vertex_path);
    std::ifstream fragment_file(fragment_path);

    if (!vertex_file.is_open())
    {
        throw std::runtime_error("Failed to open vertex shader file");
    }

    if (!fragment_file.is_open())
    {
        throw std::runtime_error("Failed to open fragment shader file");
    }

    std::string vertex_src((std::istreambuf_iterator<char>(vertex_file)), std::istreambuf_iterator<char>());
    std::string fragment_src((std::istreambuf_iterator<char>(fragment_file)), std::istreambuf_iterator<char>());

    from_src(vertex_src.c_str(), fragment_src.c_str());
}

unsigned int shader::compile(unsigned int type, char const *src)
{
    unsigned int shader_id = glCreateShader(type);
    glShaderSource(shader_id, 1, &src, nullptr);
    glCompileShader(shader_id);
    glAttachShader(program_id, shader_id);
    return shader_id;
}

shader::~shader() noexcept
{
    if (program_id) glDeleteProgram(program_id);
}

void shader::bind() noexcept
{
    glUseProgram(program_id);
}

void shader::unbind() noexcept
{
    glUseProgram(0);
}

int shader::locate_uniform(char const *name)
{
    auto it = uniform_cache.find(name);
    if (it != uniform_cache.end())
        return it->second;

    int location = glGetUniformLocation(program_id, name);

#ifndef NDEBUG
    if (location == -1)
        throw std::runtime_error("uniform not found: " + std::string(name));
#endif

    uniform_cache.insert({name, location});
    return location;
}

void shader::fill_tex_slots(char const *name, unsigned int num_slots)
{
    std::vector<int> idxs;
    idxs.reserve(num_slots);
    for (unsigned int i = 0; i < num_slots; ++i)
        idxs.push_back(i);
    glUniform1iv(locate_uniform(name), num_slots, idxs.data());
}

void inline shader::uniform(char const *name, float value)
{
    glUniform1f(locate_uniform(name), value);
}

void inline shader::uniform(char const *name, glm::vec2 const &value)
{
    glUniform2fv(locate_uniform(name), 1, glm::value_ptr(value));
}

void inline shader::uniform(char const *name, glm::vec3 const &value)
{
    glUniform3fv(locate_uniform(name), 1, glm::value_ptr(value));
}

void inline shader::uniform(char const *name, glm::vec4 const &value)
{
    glUniform4fv(locate_uniform(name), 1, glm::value_ptr(value));
}

void shader::uniform(char const *name, glm::mat4 const &value, bool transpose)
{
    glUniformMatrix4fv(locate_uniform(name), 1, transpose, glm::value_ptr(value));
}
