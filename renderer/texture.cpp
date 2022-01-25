#include "texture.hpp"

#define cimg_display 0
#define cimg_use_png 1

#define GLEW_STATIC
#include <GL/glew.h>
#include <CImg.h>

#include <chrono>
#include <iostream>

/**
 * @brief exception that is thrown when there is some problem preventing the
 * texture image from loading.
 */
class texture_loading_error : public std::exception
{
public:
    explicit texture_loading_error(std::string _msg)
        : msg(std::move(_msg)) {}

    const char *what() const noexcept override { return msg.c_str(); }

private:
    std::string msg;
};

texture::texture(const char *path)
{
    cimg_library::CImg<data_type> img(path);
    if (img.spectrum() != BPP)
        throw texture_loading_error("Texture image must be RGBA.");

    data_type *tex = new data_type[img.width() * img.height() * BPP];
    data_type *tex_ptr = tex;

    auto start = std::chrono::high_resolution_clock::now();

    for (int y{}; y < img.height(); ++y)
    {
        for (int x{}; x < img.width(); ++x)
        {
            for (int c{}; c < BPP; ++c)
            {
                *tex_ptr++ = img(x, y, 0, c);
            }
        }
    }

    // time
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Texture loading for " << path << " took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << "ms" << std::endl;

    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.width(), img.height(), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, tex);

    delete[] tex;
}

texture::~texture() noexcept
{
    unbind();
    glDeleteTextures(1, &tex_id);
}

void texture::bind(int slot) noexcept
{
    if (bound_slot == slot)
        return;
    if (bound_slot)
    {
        glActiveTexture(GL_TEXTURE0 + bound_slot);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, tex_id);
}

void texture::unbind() noexcept
{
    if (bound_slot)
    {
        glActiveTexture(GL_TEXTURE0 + bound_slot);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    bound_slot = 0;
}
