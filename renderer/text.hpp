#ifndef MJ_RENDERER_TEXT_HPP
#define MJ_RENDERER_TEXT_HPP

#include "texture.hpp"
#include "mesh.hpp"

class text : public texture
{
public:
    enum class game_call : unsigned char
    {
        ron = 0, tsumo = 1, riichi = 2, kong = 3, pong = 4, chow = 5
    };

public:
    static constexpr float
        DIGITS_TOP      = 0.00f,
        DIGITS_BOT      = 0.45f,
        DIGITS_WIDTH    = 0.10f,
        CALLS_TOP       = 0.45f,
        CALLS_BOT       = 1.00f,
        CALLS_WIDTH     = 0.15f;

public:
    explicit text(char const *path) : texture(path) {}

    /**
     * Get the vertices to render a number (without leading zeros) at a
     * given offset, with a given size per digit.
     *
     * @param number The number to render.
     * @param offset The position of the top left corner of the first digit.
     * @param size The size of each digit. Could be negative to control the
     * orientation of the number.
     *
     * @return The vertices to render the number as a vector of quad2ds.
     */
    std::vector<quad2d> render_num(int num, glm::vec2 offset, glm::vec2 h_sz, glm::vec2 v_sz) const;

    /**
     * Get the vertices to render a japanese character for a specified mahjong
     * call at a given offset, with a given size per character.
     *
     * @param call The call to render.
     * @param offset The position of the top left corner of the first character.
     * @param sz The size of each character. Could be negative to control the
     * orientation of the character.
     *
     * @return The vertices to render the character as a quad2d struct.
     */
    quad2d render_call(game_call call, glm::vec2 offset, glm::vec2 sz) const;

private:
    quad2d render_digit(int num, glm::vec2 offset, glm::vec2 h_sz, glm::vec2 v_sz) const;
};

#endif