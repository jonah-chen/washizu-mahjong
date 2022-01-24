#ifndef MJ_RENDERER_MESH_HPP
#define MJ_RENDERER_MESH_HPP

#include <vector>
#include <array>
#include <glm/glm.hpp>

using index_type = unsigned int;

struct vertex2d
{
    union
    {
        glm::vec2 position;
        struct
        {
            float x, y;
        };
    };
    union
    {
        glm::vec2 tex_coord;
        struct
        {
            float u, v;
        };
    };

    float tex_index;
};

struct vertex3d
{
    union
    {
        glm::vec3 position;
        struct
        {
            float x, y, z;
        };
    };
    union
    {
        glm::vec2 tex_coord;
        struct
        {
            float u, v;
        };
    };
    union
    {
        glm::vec3 normal;
        struct
        {
            float nx, ny, nz;
        };
    };
    float tex_index;
};

template <std::size_t QUADS>
struct quad_indices : public std::array<index_type, QUADS*6>
{
    constexpr quad_indices()
    {
        for (std::size_t i {}; i < QUADS; ++i)
        {
            (*this)[i*6+0] = i*4+0;
            (*this)[i*6+1] = i*4+1;
            (*this)[i*6+2] = i*4+2;
            (*this)[i*6+3] = i*4+2;
            (*this)[i*6+4] = i*4+3;
            (*this)[i*6+5] = i*4+0;
        }
    }
};

struct quad2d
{
    vertex2d bl, br, tr, tl;
};

bool load_obj(char const *filename, std::vector<index_type> &indices, std::vector<vertex3d> &vertices);

#endif
