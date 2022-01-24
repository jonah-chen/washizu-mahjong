#ifndef MJ_RENDERER_TEXTURE_HPP
#define MJ_RENDERER_TEXTURE_HPP

class texture
{
public:
    static constexpr int BPP = 4;
    using data_type = unsigned char;
public:
    explicit texture(const char *path);
    ~texture() noexcept;

    void bind(int tex_slot) noexcept;
    void unbind() noexcept;

private:
    unsigned int tex_id;
    int bound_slot {};
};

#endif
