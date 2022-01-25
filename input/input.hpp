#ifndef MJ_INPUT_INPUT_HPP
#define MJ_INPUT_INPUT_HPP

#include <GLFW/glfw3.h>
#include <iostream>

namespace input_2d
{
void on_mouse_button(GLFWwindow *window, int button, int action, int mods);

class istream : private std::istream
{
public:
    istream() = default;

    istream &operator>>(std::string &str);
};

}

namespace input_3d
{
void on_mouse_button(GLFWwindow *window, int button, int action, int mods);
}

#endif