#include "mesh.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <unordered_map>

bool load_obj(char const *filename, std::vector<index_type> &indices, std::vector<vertex3d> &vertices)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> tex_coords;
    std::vector<glm::vec3> normals;
    std::unordered_map<std::string, unsigned int> faces;

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        if (token == "#")
        {
            continue;
        }
        else if (token == "v")
        {
            glm::vec3 position;
            iss >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
        else if (token == "vt")
        {
            glm::vec2 tex_coord;
            iss >> tex_coord.x >> tex_coord.y;
            tex_coords.push_back(tex_coord);
        }
        else if (token == "vn")
        {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (token == "f")
        {
            for (int i = 0; i < 3; ++i)
            {
                std::string vertex_str;
                iss >> vertex_str;

                if (faces.find(vertex_str) == faces.end())
                {
                    indices.push_back(faces.size());
                    faces[vertex_str] = faces.size();
                }
                else
                {
                    std::cout << vertex_str << std::endl;
                    indices.push_back(faces.at(vertex_str));
                }
            }
        }
        else if (token == "o" or token == "s" or token == "usemtl" or token == "mtllib")
        {
            continue;
        }
        else
        {
            std::cerr << "Unknown token: " << token << std::endl;
            return false;
        }
    }

    vertices.resize(faces.size());
    for (auto const &[vertex_str, index] : faces)
    {
        int pos_index = vertex_str.find('/');
        int tex_index = vertex_str.find('/', pos_index + 1);
        int norm_index = vertex_str.find('/', tex_index + 1);
        int pos_id = std::stoi(vertex_str.substr(0, pos_index));
        int tex_id = std::stoi(vertex_str.substr(pos_index + 1, tex_index - pos_index - 1));
        int norm_id = std::stoi(vertex_str.substr(tex_index + 1, norm_index - tex_index - 1));
        vertices[index].position = positions[pos_id - 1];
        vertices[index].tex_coord = tex_coords[tex_id - 1];
        vertices[index].normal = normals[norm_id - 1];
    }

    return true;
}

void quad2d::tint(glm::vec4 const &color)
{
    bl.tint = color;
    br.tint = color;
    tr.tint = color;
    tl.tint = color;
}

void quad2d::tex_index(float index)
{
    bl.tex_index = index;
    br.tex_index = index;
    tr.tex_index = index;
    tl.tex_index = index;
}
