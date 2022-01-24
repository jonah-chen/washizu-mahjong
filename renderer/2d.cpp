#include "2d.hpp"

#define GLEW_STATIC
#include <GL/glew.h>

#ifndef NDEBUG

void APIENTRY
debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
			  GLsizei length, const GLchar *message, const void *userParam)
{
    switch(severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        std::cerr << "High severity: " << id << ": " << message << std::endl;
        throw std::runtime_error("OpenGL error");
    case GL_DEBUG_SEVERITY_MEDIUM:
        std::cerr << "Medium severity: " << id << ": " << message << std::endl;
        throw std::runtime_error("OpenGL error");
    case GL_DEBUG_SEVERITY_LOW:
        std::cerr << "Low severity: " << id << ": " << message << std::endl;
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        std::cerr << "Notification: " << id << ": " << message << std::endl;
        break;
    }
}

#endif

renderer2d::renderer2d()
{
    buffer = buffer_ptr = new quad2d[MAX_QUADS];
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_QUADS * sizeof(vertex2d), nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(index_type),
        indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex2d),
        (const void*)offsetof(vertex2d, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex2d),
        (const void*)offsetof(vertex2d, tex_coord));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(vertex2d),
        (const void*)offsetof(vertex2d, tex_index));
    glEnableVertexAttribArray(2);

    tex.bind(0);
    program.bind();
    program.fill_tex_slots("tex_array");
    program.uniform("projection", glm::mat4(1.0f));
}

renderer2d::~renderer2d() noexcept
{
    delete[] buffer;
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}

void renderer2d::submit(mj_tile tile, int relative_pos)
{
    quad2d q;

    if (MJ_IS_OPAQUE(tile) == MJ_TRUE)
    {
        q.tl.v = q.tr.v = 0.0f;
        q.bl.v = q.br.v = 0.5f;
    }
    else
    {
        q.tl.v = q.tr.v = 0.5f;
        q.bl.v = q.br.v = 1.0f;
    }

    float idx34;
    if (MJ_SUIT(tile) == MJ_DRAGON)
        idx34 = 31 + MJ_NUMBER(tile);
    else
        idx34 = MJ_NUMBER(tile) + MJ_SUIT(tile) * 9;

    q.bl.u = q.tl.u = idx34 / MJ_UNIQUE_TILES;
    q.br.u = q.tr.u = (idx34 + 1) / MJ_UNIQUE_TILES;

    q.bl.x = -0.5f;
    q.bl.y = -0.5f;
    q.tl.x = -0.5f;
    q.tl.y = 0.5f;
    q.tr.x = 0.5f;
    q.tr.y = 0.5f;
    q.br.x = 0.5f;
    q.br.y = -0.5f;

    q.tr.tex_index = q.br.tex_index = q.tl.tex_index = q.bl.tex_index = 0;

    *buffer_ptr = q;
    buffer_ptr++;
    num_quads++;
}

void renderer2d::flush()
{
    if (buffer_ptr == buffer)
        return;

    glBufferSubData(GL_ARRAY_BUFFER, 0, num_quads * sizeof(quad2d), buffer);
    glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_INT, nullptr);
}

void renderer2d::clear()
{
    buffer_ptr = buffer;
    num_quads = 0;
}
