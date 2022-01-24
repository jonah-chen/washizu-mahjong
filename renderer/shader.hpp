#ifndef MJ_RENDERER_SHADER_HPP
#define MJ_RENDERER_SHADER_HPP

#include <unordered_map>
#include <string>
#include <glm/glm.hpp>

class shader
{
public:
    shader() = default;
    shader(char const *vertex_src, char const *fragment_src);

    ~shader() noexcept;

    void from_src(char const *vertex_src, char const *fragment_src);
    void from_file(char const *vertex_file, char const *fragment_file);

    void bind() noexcept;
    static void unbind() noexcept;

    void fill_tex_slots(char const *name, unsigned int num_slots=32);
    void uniform(char const *name, float value);
    void uniform(char const *name, glm::vec2 const &value);
    void uniform(char const *name, glm::vec3 const &value);
    void uniform(char const *name, glm::vec4 const &value);
    void uniform(char const *name, glm::mat4 const &value, bool transpose=false);

private:
    unsigned int program_id {};
    std::unordered_map<std::string, int> uniform_cache;

private:
    unsigned int compile(unsigned int type, char const *src);
    int locate_uniform(char const *name);
};

#endif