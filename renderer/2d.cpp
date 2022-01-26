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
        throw std::runtime_error("Failed to initialize renderer2d window.");

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
    delete[] buffer;
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
    glfwTerminate();
}

void renderer2d::submit(mj_hand const &hand, int relative_pos)
{
    glm::vec2 base, offset;
    switch (relative_pos)
    {
    case MJ_EAST:
        base = { PLAYFIELD_LEFT + HAND_OFFSET, PLAYFIELD_BOTTOM };
        offset = { TILE_WIDTH, 0.f };
        break;
    case MJ_SOUTH:
        base = { PLAYFIELD_RIGHT, PLAYFIELD_BOTTOM + HAND_OFFSET };
        offset = { 0.f, TILE_WIDTH };
        break;
    case MJ_WEST:
        base = { PLAYFIELD_RIGHT - HAND_OFFSET, PLAYFIELD_TOP };
        offset = { -TILE_WIDTH, 0.f };
        break;
    case MJ_NORTH:
        base = { PLAYFIELD_LEFT, PLAYFIELD_TOP - HAND_OFFSET };
        offset = { 0.f, -TILE_WIDTH };
        break;
    default: throw 0;
    }

    auto &instance = get_instance();
    for (int i = 0; i < hand.size; ++i)
        instance.submit(hand.tiles[i], relative_pos,
            base + static_cast<float>(i)*offset);
}

void renderer2d::submit(mj_meld const &melds, int relative_pos)
{
    glm::vec2 meld_br;
    glm::vec2 meld_offset;
    glm::vec2 tile_offset;
    glm::vec2 ext_offset;
    switch (relative_pos)
    {
    case MJ_EAST:
        meld_br = { PLAYFIELD_RIGHT, PLAYFIELD_BOTTOM };
        meld_offset = { 0.f, TILE_HEIGHT };
        tile_offset = { -TILE_WIDTH, 0.f };
        ext_offset = { -(TILE_HEIGHT - TILE_WIDTH), 0.f };
        break;
    case MJ_SOUTH:
        meld_br = { PLAYFIELD_RIGHT, PLAYFIELD_TOP };
        meld_offset = { -TILE_HEIGHT, 0.f };
        tile_offset = { 0.f, -TILE_WIDTH };
        ext_offset = { 0.f, -(TILE_HEIGHT - TILE_WIDTH) };
        break;
    case MJ_WEST:
        meld_br = { PLAYFIELD_LEFT, PLAYFIELD_TOP };
        meld_offset = { 0.f, -TILE_HEIGHT };
        tile_offset = { TILE_WIDTH, 0.f };
        ext_offset = { (TILE_HEIGHT - TILE_WIDTH), 0.f };
        break;
    case MJ_NORTH:
        meld_br = { PLAYFIELD_LEFT, PLAYFIELD_BOTTOM };
        meld_offset = { TILE_HEIGHT, 0.f };
        tile_offset = { 0.f, TILE_WIDTH };
        ext_offset = { 0.f, (TILE_HEIGHT - TILE_WIDTH) };
        break;
    }

    auto &instance = get_instance();
    for (auto *trip_ptr = melds.melds; trip_ptr < melds.melds + melds.size; ++trip_ptr)
    {
        if (MJ_IS_OPEN(*trip_ptr) && MJ_IS_KONG(*trip_ptr)) /* Open Kong */
        {
            mj_tile called_tile = MJ_FIRST(*trip_ptr);
            switch (MJ_TRIPLE_FROM(*trip_ptr))
            {
            case MJ_SOUTH:
                instance.submit(called_tile, MJ_NEXT_PLAYER(relative_pos),
                    meld_br);
                instance.submit(called_tile^0b01, relative_pos,
                    meld_br + 2.f*tile_offset + ext_offset);
                instance.submit(called_tile^0b10, relative_pos,
                    meld_br + 3.f*tile_offset + ext_offset);
                instance.submit(called_tile^0b11, relative_pos,
                    meld_br + 4.f*tile_offset + ext_offset);
                break;
            case MJ_WEST:
                instance.submit(called_tile&0b01, relative_pos,
                    meld_br + tile_offset);
                instance.submit(called_tile, MJ_NEXT_PLAYER(relative_pos),
                    meld_br + tile_offset);
                instance.submit(called_tile^0b10, relative_pos,
                    meld_br + 3.f*tile_offset + ext_offset);
                instance.submit(called_tile^0b11, relative_pos,
                    meld_br + 4.f*tile_offset + ext_offset);
                break;
            case MJ_NORTH:
                instance.submit(called_tile^0b01, relative_pos,
                    meld_br + tile_offset);
                instance.submit(called_tile&0b10, relative_pos,
                    meld_br + 2.f*tile_offset);
                instance.submit(called_tile^0b11, relative_pos,
                    meld_br + 3.f*tile_offset);
                instance.submit(called_tile, MJ_NEXT_PLAYER(relative_pos),
                    meld_br + 3.f*tile_offset);
                break;
            }
        }
        else if (MJ_IS_OPEN(*trip_ptr) && !MJ_IS_KONG(*trip_ptr)) /* Pong or Chow */
        {
            switch(MJ_TRIPLE_FROM(*trip_ptr))
            {
            case MJ_SOUTH:
                instance.submit(MJ_FIRST(*trip_ptr), MJ_NEXT_PLAYER(relative_pos),
                    meld_br);
                instance.submit(MJ_SECOND(*trip_ptr), relative_pos,
                    meld_br + 2.f*tile_offset + ext_offset);
                instance.submit(MJ_THIRD(*trip_ptr), relative_pos,
                    meld_br + 3.f*tile_offset + ext_offset);
                break;
            case MJ_WEST:
                instance.submit(MJ_SECOND(*trip_ptr), relative_pos,
                    meld_br + tile_offset);
                instance.submit(MJ_FIRST(*trip_ptr), MJ_NEXT_PLAYER(relative_pos),
                    meld_br + tile_offset);
                instance.submit(MJ_THIRD(*trip_ptr), relative_pos,
                    meld_br + 3.f*tile_offset + ext_offset);
                break;
            case MJ_NORTH:
                instance.submit(MJ_SECOND(*trip_ptr), relative_pos,
                    meld_br + tile_offset);
                instance.submit(MJ_THIRD(*trip_ptr), relative_pos,
                    meld_br + 2.f*tile_offset);
                instance.submit(MJ_FIRST(*trip_ptr), MJ_NEXT_PLAYER(relative_pos),
                    meld_br + 2.f*tile_offset);
                break;
            }
        }
        else /* closed kong */
        {
            instance.submit(MJ_INVALID_TILE, relative_pos,
                meld_br + tile_offset);
            instance.submit(MJ_FIRST(*trip_ptr), relative_pos,
                meld_br + 2.f * tile_offset);
            instance.submit(MJ_SECOND(*trip_ptr), relative_pos,
                meld_br + 3.f * tile_offset);
            instance.submit(MJ_INVALID_TILE, relative_pos,
                meld_br + 4.f * tile_offset);
        }

        meld_br += meld_offset;
    }
}

void renderer2d::submit(mj_tile tile, int orientation, glm::vec2 pos)
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
        q.bl.y = q.br.y = pos.y;
        q.bl.x = q.tl.x = pos.x;
        q.tl.y = q.tr.y = pos.y + TILE_HEIGHT_INTERN;
        q.tr.x = q.br.x = pos.x + TILE_WIDTH_INTERN;
        break;
    case MJ_SOUTH:
        q.bl.position = { pos.x, pos.y };
        q.br.position = { pos.x, pos.y + TILE_WIDTH_INTERN };
        q.tl.position = { pos.x - TILE_HEIGHT_INTERN, pos.y };
        q.tr.position = { pos.x - TILE_HEIGHT_INTERN, pos.y + TILE_WIDTH_INTERN };
        break;
    case MJ_WEST:
        q.bl.position = { pos.x, pos.y };
        q.br.position = { pos.x - TILE_WIDTH_INTERN, pos.y };
        q.tl.position = { pos.x, pos.y - TILE_HEIGHT_INTERN };
        q.tr.position = { pos.x - TILE_WIDTH_INTERN, pos.y - TILE_HEIGHT_INTERN };
        break;
    case MJ_NORTH:
        q.bl.position = { pos.x, pos.y };
        q.br.position = { pos.x, pos.y - TILE_WIDTH_INTERN };
        q.tl.position = { pos.x + TILE_HEIGHT_INTERN, pos.y };
        q.tr.position = { pos.x + TILE_HEIGHT_INTERN, pos.y - TILE_WIDTH_INTERN };
        break;
    default: throw 0;
    }

    q.tr.tex_index = q.br.tex_index = q.tl.tex_index = q.bl.tex_index = TILES_TEX_SLOT;

    *buffer_ptr = q;
    buffer_ptr++;
    num_quads++;
}

void renderer2d::submit(mj_tile tile, glm::vec2 pos, int relative_pos, bool after_riichi)
{
    constexpr float RIICHI_OFFSET = TILE_HEIGHT - TILE_WIDTH;
    glm::vec2 base;
    switch (relative_pos)
    {
    case MJ_EAST:
    {
        base = glm::vec2{ PLAYFIELD_LEFT + DISCARD_PILE_OFFSET.x,
            PLAYFIELD_BOTTOM + DISCARD_PILE_OFFSET.y } +
            glm::vec2{ pos.x * TILE_WIDTH, -pos.y * TILE_HEIGHT };

        if (after_riichi)
            base.x += RIICHI_OFFSET;

        break;
    }
    case MJ_SOUTH:
    {
        base = glm::vec2{ PLAYFIELD_RIGHT - DISCARD_PILE_OFFSET.y,
            PLAYFIELD_BOTTOM + DISCARD_PILE_OFFSET.x }
            + glm::vec2{ pos.y * TILE_HEIGHT, pos.x * TILE_WIDTH };

        if (after_riichi)
            base.y += RIICHI_OFFSET;

        break;
    }
    case MJ_WEST:
    {
        base = glm::vec2{ PLAYFIELD_RIGHT - DISCARD_PILE_OFFSET.x,
            PLAYFIELD_TOP - DISCARD_PILE_OFFSET.y } +
            glm::vec2{ -pos.x * TILE_WIDTH, pos.y * TILE_HEIGHT };

        if (after_riichi)
            base.x -= RIICHI_OFFSET;

        break;
    }
    case MJ_NORTH:
    {
        base = glm::vec2{ PLAYFIELD_LEFT + DISCARD_PILE_OFFSET.y,
            PLAYFIELD_TOP - DISCARD_PILE_OFFSET.x } +
            glm::vec2{ -pos.y * TILE_HEIGHT, -pos.x * TILE_WIDTH };

        if (after_riichi)
            base.y -= RIICHI_OFFSET;

        break;
    }
    default: throw 0;
    }

    submit(tile, relative_pos, base);
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
