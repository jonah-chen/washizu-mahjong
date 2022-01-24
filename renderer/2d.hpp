#ifndef MJ_RENDERER_2D_HPP
#define MJ_RENDERER_2D_HPP

#define GLEW_STATIC

#include "mesh.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "mahjong/mahjong.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifndef NDEBUG

#include <iostream>

void APIENTRY
debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
			  GLsizei length, const GLchar *message, const void *userParam);
#endif

class renderer2d
{
public:
    static constexpr std::size_t MAX_QUADS = 256;
public:
    renderer2d();
    ~renderer2d() noexcept;

    renderer2d(renderer2d const &) = delete;
    renderer2d(renderer2d &&) = delete;
    renderer2d &operator=(renderer2d const &) = delete;
    renderer2d &operator=(renderer2d &&) = delete;

    void submit(mj_tile tile, int relative_pos);
    void submit(mj_meld meld, int relative_pos);

    void flush();
    void clear();

private:
    unsigned int vao, vbo, ebo;

    quad2d *buffer, *buffer_ptr;

    std::size_t num_quads {};

    static constexpr quad_indices<MAX_QUADS> indices {};

    shader program {
"#version 450 core\n"
"layout (location = 0) in vec2 position;\n"
"layout (location = 1) in vec2 tex_coord;\n"
"layout (location = 2) in float tex_idx;\n"
"out vec2 tex_coord_out;\n"
"out float tex_idx_out;\n"
"uniform mat4 projection;\n"
"void main() {\n"
"    tex_coord_out = tex_coord;\n"
"    tex_idx_out = tex_idx;\n"
"    gl_Position = projection * vec4(position, 0.0, 1.0);\n"
"}\n",
"#version 450 core\n"
"in vec2 tex_coord_out;\n"
"in float tex_idx_out;\n"
"uniform sampler2D tex_array[8];\n"
"out vec4 color;\n"
"void main() {\n"
"    color = texture(tex_array[int(tex_idx_out)], tex_coord_out);\n"
"}\n"
    };

    texture tex {"assets/texture/tiles.png"};
};

#endif