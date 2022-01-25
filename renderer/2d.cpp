#include "2d.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

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
        //std::cerr << "Notification: " << id << ": " << message << std::endl;
        break;
    default:
        break;
    }
}

#endif

renderer2d &renderer2d::get_instance()
{
    static renderer2d instance;
    return instance;
}

GLFWwindow *renderer2d::init_window()
{
    if (!glfwInit())
        return nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_VERSION);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_SUBVERSION);
    glfwWindowHint(GLFW_OPENGL_PROFILE, OPENGL_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    GLFWwindow *new_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Washizu Mahjong", nullptr, nullptr);

    if (!new_window)
        return nullptr;

    glfwMakeContextCurrent(new_window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK)
        return nullptr;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debugCallback, nullptr);
#endif

    return new_window;
}

renderer2d::renderer2d() : window(init_window())
{
    if (!window)
        throw std::runtime_error("Failed to initialize renderer2d");

    buffer = buffer_ptr = new quad2d[MAX_QUADS];
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_QUADS * sizeof(quad2d), nullptr, GL_DYNAMIC_DRAW);

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
    program.uniform("projection", glm::ortho(PLAYFIELD_LEFT, PLAYFIELD_RIGHT, PLAYFIELD_BOTTOM, PLAYFIELD_TOP));
}

renderer2d::~renderer2d() noexcept
{
    // delete[] buffer;
    // glDeleteBuffers(1, &vbo);
    // glDeleteBuffers(1, &ebo);
    // glDeleteVertexArrays(1, &vao);
    // glfwTerminate();
}

void renderer2d::submit(mj_hand const &hand, int relative_pos)
{
    float x_base, y_base, x_offset, y_offset;
    switch (relative_pos)
    {
    case MJ_EAST:
        x_base = PLAYFIELD_LEFT + HAND_OFFSET;
        y_base = PLAYFIELD_BOTTOM;
        x_offset = TILE_WIDTH;
        y_offset = 0.f;
        break;
    case MJ_SOUTH:
        x_base = PLAYFIELD_RIGHT;
        y_base = PLAYFIELD_BOTTOM + HAND_OFFSET;
        x_offset = 0.f;
        y_offset = TILE_WIDTH;
        break;
    case MJ_WEST:
        x_base = PLAYFIELD_RIGHT - HAND_OFFSET;
        y_base = PLAYFIELD_TOP;
        x_offset = -TILE_WIDTH;
        y_offset = 0.f;
        break;
    case MJ_NORTH:
        x_base = PLAYFIELD_LEFT;
        y_base = PLAYFIELD_TOP - HAND_OFFSET;
        x_offset = 0.f;
        y_offset = -TILE_WIDTH;
        break;
    default: throw 0;
    }

    auto &instance = get_instance();
    for (int i = 0; i < hand.size; ++i)
        instance.submit(hand.tiles[i], relative_pos,
            x_base + i*x_offset, y_base + i*y_offset);
}

void renderer2d::submit(mj_tile tile, int orientation, float x, float y)
{
    quad2d q;

    if (tile == MJ_INVALID_TILE)
    {
        tile = MJ_TILE(MJ_DRAGON, MJ_WHITE, 0);
    } // temporarily

    if (MJ_IS_OPAQUE(tile) == MJ_TRUE)
    {
        q.tl.v = q.tr.v = 0.5f;
        q.bl.v = q.br.v = 1.0f;
    }
    else
    {
        q.tl.v = q.tr.v = 0.0f;
        q.bl.v = q.br.v = 0.5f;
    }

    float idx34;
    if (MJ_SUIT(tile) == MJ_DRAGON)
        idx34 = 31 + MJ_NUMBER(tile);
    else
        idx34 = MJ_NUMBER(tile) + MJ_SUIT(tile) * 9;

    q.bl.u = q.tl.u = idx34 / MJ_UNIQUE_TILES;
    q.br.u = q.tr.u = (idx34 + 1) / MJ_UNIQUE_TILES;

    switch (orientation)
    {
    case MJ_EAST:
        q.bl.y = q.br.y = y;
        q.bl.x = q.tl.x = x;
        q.tl.y = q.tr.y = y + TILE_HEIGHT_INTERN;
        q.tr.x = q.br.x = x + TILE_WIDTH_INTERN;
        break;
    case MJ_SOUTH:
        q.bl.position = { x, y };
        q.br.position = { x, y + TILE_WIDTH_INTERN };
        q.tl.position = { x - TILE_HEIGHT_INTERN, y };
        q.tr.position = { x - TILE_HEIGHT_INTERN, y + TILE_WIDTH_INTERN };
        break;
    case MJ_WEST:
        q.bl.position = { x, y };
        q.br.position = { x - TILE_WIDTH_INTERN, y };
        q.tl.position = { x, y - TILE_HEIGHT_INTERN };
        q.tr.position = { x - TILE_WIDTH_INTERN, y - TILE_HEIGHT_INTERN };
        break;
    case MJ_NORTH:
        q.bl.position = { x, y };
        q.br.position = { x, y - TILE_WIDTH_INTERN };
        q.tl.position = { x + TILE_HEIGHT_INTERN, y };
        q.tr.position = { x + TILE_HEIGHT_INTERN, y - TILE_WIDTH_INTERN };
        break;
    default: throw 0;
    }

    q.tr.tex_index = q.br.tex_index = q.tl.tex_index = q.bl.tex_index = TILES_TEX_SLOT;

    *buffer_ptr = q;
    buffer_ptr++;
    num_quads++;
}

void renderer2d::submit(mj_tile tile, float x, float y, int relative_pos, bool after_riichi)
{
    switch (relative_pos)
    {
    case MJ_EAST:
    {
        float const x_base = PLAYFIELD_LEFT + DISCARD_PILE_OFFSET.x;
        float const y_base = PLAYFIELD_BOTTOM + DISCARD_PILE_OFFSET.y;

        if (after_riichi)
            submit(tile, relative_pos, x_base + (x-1) * TILE_WIDTH + TILE_HEIGHT, y_base - y * TILE_HEIGHT);
        else
            submit(tile, relative_pos, x_base + x * TILE_WIDTH, y_base - y * TILE_HEIGHT);
        return;
    }
    case MJ_SOUTH:
    {
        float const x_base = PLAYFIELD_RIGHT - DISCARD_PILE_OFFSET.y;
        float const y_base = PLAYFIELD_BOTTOM + DISCARD_PILE_OFFSET.x;
        if (after_riichi)
            submit(tile, relative_pos, x_base + y * TILE_HEIGHT, y_base + (x-1) * TILE_WIDTH + TILE_HEIGHT);
        else
            submit(tile, relative_pos, x_base + y * TILE_HEIGHT, y_base + x * TILE_WIDTH);
        return;
    }
    case MJ_WEST:
    {
        float const x_base = PLAYFIELD_RIGHT - DISCARD_PILE_OFFSET.x;
        float const y_base = PLAYFIELD_TOP - DISCARD_PILE_OFFSET.y;

        if (after_riichi)
            submit(tile, relative_pos, x_base - (x-1) * TILE_WIDTH - TILE_HEIGHT, y_base + y * TILE_HEIGHT);
        else
            submit(tile, relative_pos, x_base - x * TILE_WIDTH, y_base + y * TILE_HEIGHT);
        return;
    }
    case MJ_NORTH:
    {
        float const x_base = PLAYFIELD_LEFT + DISCARD_PILE_OFFSET.y;
        float const y_base = PLAYFIELD_TOP - DISCARD_PILE_OFFSET.x;

        if (after_riichi)
            submit(tile, relative_pos, x_base - y * TILE_HEIGHT, y_base - (x-1) * TILE_WIDTH - TILE_HEIGHT);
        else
            submit(tile, relative_pos, x_base - y * TILE_HEIGHT, y_base - x * TILE_WIDTH);
        return;
    }
    default: throw 0;
    }
}

void renderer2d::flush()
{
    get_instance().flush_impl();
}

void renderer2d::flush_impl()
{
    if (buffer_ptr != buffer)
        glBufferSubData(GL_ARRAY_BUFFER, 0, num_quads * sizeof(quad2d), buffer);
    glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_INT, nullptr);
}

void renderer2d::clear()
{
    get_instance().clear_impl();
}

void renderer2d::clear_impl()
{
    buffer_ptr = buffer;
    num_quads = 0;
}
