#include "text.hpp"

#ifndef NDEBUG
#include <stdexcept>
#endif

std::vector<quad2d> text::render_num(int num, glm::vec2 offset, glm::vec2 h_sz, glm::vec2 v_sz) const
{
    std::vector<quad2d> quads;

    // count number of digits
    int digits = 0;
    int n = num;
    while (n) {
        n /= 10;
        ++digits;
    }

    offset += h_sz * static_cast<float>(digits - 1);

    while (num)
    {
        int digit = num % 10;
        num /= 10;
        auto quad = render_digit(digit, offset, h_sz, v_sz);
        offset -= h_sz;
        quads.push_back(quad);
    }

    return quads;
}

quad2d text::render_call(game_call call, glm::vec2 offset, glm::vec2 sz) const
{
    quad2d quad;

    quad.tex_index(bound_slot);
    quad.tint(glm::vec4(1.0f));

    quad.tl.position = offset;
    quad.br.position = offset + sz;
    quad.tr.position = { quad.br.x, quad.tl.y };
    quad.bl.position = { quad.tl.x, quad.br.y };

    quad.tl.u = quad.bl.u = static_cast<float>(call) * CALLS_WIDTH;
    quad.tr.u = quad.br.u = (static_cast<float>(call) + 1.f) * CALLS_WIDTH;
    quad.tl.v = quad.tr.v = CALLS_TOP;
    quad.bl.v = quad.br.v = CALLS_BOT;

    return quad;
}

quad2d text::render_digit(int num, glm::vec2 offset, glm::vec2 h_sz, glm::vec2 v_sz) const
{
    quad2d quad;

#ifndef NDEBUG
    if (bound_slot < 0)
        throw std::runtime_error("text::render_digit: texture not bound.");
#endif

    quad.tex_index(bound_slot);
    quad.tint(glm::vec4(1.f));

    quad.tl.position = offset;
    quad.tr.position = offset + h_sz;
    quad.bl.position = offset + v_sz;
    quad.br.position = offset + h_sz + v_sz;

    quad.tl.u = quad.bl.u = num * DIGITS_WIDTH;
    quad.tr.u = quad.br.u = (num + 1) * DIGITS_WIDTH;
    quad.tl.v = quad.tr.v = DIGITS_TOP;
    quad.bl.v = quad.br.v = DIGITS_BOT;

    return quad;
}
