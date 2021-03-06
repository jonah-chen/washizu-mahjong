#ifndef MJ_RENDERER_2D_HPP
#define MJ_RENDERER_2D_HPP
#define MJ_RENDERER

#define GLEW_STATIC

#include "mesh.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "text.hpp"
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

    static constexpr int
        OPENGL_VERSION      = 4,
        OPENGL_SUBVERSION   = 5,
        OPENGL_PROFILE      = GLFW_OPENGL_CORE_PROFILE,
        WINDOW_WIDTH        = 1024,
        WINDOW_HEIGHT       = 1024;

    static constexpr float
        PLAYFIELD_LEFT      = 0.0f,
        PLAYFIELD_RIGHT     = 75.0f,
        PLAYFIELD_BOTTOM    = 0.0f,
        PLAYFIELD_TOP       = 75.0f,
        TILE_WIDTH          = 3.0f,
        TILE_HEIGHT         = 4.0f,
        HAND_OFFSET         = 16.0f;

    static constexpr unsigned char
        RON_FLAG            = 0x80,
        TSUMO_FLAG          = 0x40,
        RIICHI_FLAG         = 0x20,
        KONG_FLAG           = 0x10,
        PONG_FLAG           = 0x08,
        CHOW_FLAG           = 0x04;

    static constexpr glm::vec2
        DISCARD_PILE_OFFSET     = { 28.0f, 24.0f },
        CALL_TEXT_OFFSET        = { 46.6f, 21.0f },
        CALL_TEXT_SIZE          = { 3.2f, 3.0f };

    static constexpr mj_tile TSUMOGIRI_FLAG = (1 << 14);

    static constexpr glm::vec4
        DEFAULT_TINT    = { 0.94f, 0.94f, 0.94f, 0.96f },
        TSUMOGIRI_TINT  = { 0.90f, 1.00f, 1.00f, 0.74f },
        HIGHLIGHT_TINT  = { 1.00f, 0.70f, 0.60f, 1.00f },
        UNAVAILABLE_CALL= { 0.50f, 0.50f, 0.50f, 0.80f },
        AVAILABLE_CALL  = { 1.00f, 0.30f, 0.30f, 1.00f };

private:
    static constexpr int
        TILES_TEX_SLOT      = 0,
        TEXT_TEX_SLOT       = 1,
        DISCARDS_PER_LINE   = 6;
    static constexpr float
        TILE_WIDTH_INTERN   = 2.77f,
        TILE_HEIGHT_INTERN  = 3.91f;
    static constexpr
        glm::vec2 CALL_TEXT_SIZE_INTERN = { 3.0f, -3.0f };

public:
    static unsigned char call_flags;

public:
    ~renderer2d() noexcept;

    renderer2d(renderer2d const &) = delete;
    renderer2d &operator=(renderer2d const &) = delete;

    static renderer2d &get_instance();

    static void submit(int number, glm::vec2 topleft, int relative_pos);

    static void submit_calls();

    static void submit(mj_hand const &hand, int relative_pos);

    template<typename Allocator>
    static void submit(std::vector<mj_tile, Allocator> const &discards, int relative_pos, int riichi_turn=0x7fff)
    {
        auto &instance = get_instance();
        for (int i = 0; i < std::min(18ul, discards.size()); ++i)
            instance.submit(discards[i], {i%DISCARDS_PER_LINE, i/DISCARDS_PER_LINE},
                relative_pos, i, riichi_turn);

        for (int i = 18; i < discards.size(); ++i)
            instance.submit(discards[i], {i-12, 2}, relative_pos, i, riichi_turn);
    }

    static void submit(mj_meld const &meld, int relative_pos);

    static void flush();
    static void clear();

    static inline GLFWwindow *window_ptr() { return get_instance().window; }

private:
    GLFWwindow *init_window();
    renderer2d();
    GLFWwindow *window;

    unsigned int vao, vbo, ebo;

    quad2d *buffer, *buffer_ptr;

    std::size_t num_quads {};

    static constexpr quad_indices<MAX_QUADS> indices {};

    shader program {
"#version 450 core\n"
"layout (location = 0) in vec2 position;\n"
"layout (location = 1) in vec4 tint;\n"
"layout (location = 2) in vec2 tex_coord;\n"
"layout (location = 3) in float tex_idx;\n"
"out vec2 tex_coord_out;\n"
"out float tex_idx_out;\n"
"out vec4 tint_out;\n"
"uniform mat4 projection;\n"
"void main() {\n"
"    tex_coord_out = tex_coord;\n"
"    tex_idx_out = tex_idx;\n"
"    tint_out = tint;\n"
"    gl_Position = projection * vec4(position, 0.0, 1.0);\n"
"}\n",
"#version 450 core\n"
"in vec2 tex_coord_out;\n"
"in float tex_idx_out;\n"
"in vec4 tint_out;\n"
"uniform sampler2D tex_array[8];\n"
"out vec4 color;\n"
"void main() {\n"
"    color = texture(tex_array[int(tex_idx_out)], tex_coord_out) * tint_out;\n"
"}\n"
    };

    texture tile_tex {"/home/john/CLionProjects/washizu-mahjong/assets/texture/tiles.png"};
    text text_tex {"/home/john/CLionProjects/washizu-mahjong/assets/texture/text.png"};

private:
    void flush_impl();
    void clear_impl();
    void submit(mj_tile tile, int orientation, glm::vec2 pos, glm::vec4 tint=DEFAULT_TINT);
    void submit(mj_tile tile, glm::vec2 pos, int relative_pos, int turn, int riichi_turn);
    void submit(text::game_call call, glm::vec2 topleft, bool active);
};

#endif