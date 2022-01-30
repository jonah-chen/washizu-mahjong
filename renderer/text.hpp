#ifndef MJ_RENDERER_TEXT_HPP
#define MJ_RENDERER_TEXT_HPP

#include "texture.hpp"
#include "mesh.hpp"

class text : public texture
{
    enum class game_call : unsigned char
    {
        ron, tsumo, riichi, kong, pong, chow
    };
public:
    static std::array<quad2d, 10> DIGITS;
    text(const char *path, const char *cfg) : texture(path) { configure(cfg); }

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
    std::vector<quad2d> render_num(int num, glm::vec2 offset, glm::vec2 sz) const;

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
    void configure(const char *cfg);
};

#endif