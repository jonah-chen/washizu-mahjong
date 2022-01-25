#include "input.hpp"

#include <iostream>
void input_2d::on_mouse_button(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double x, y;
        int width, height;
        glfwGetCursorPos(window, &x, &y);
        glfwGetWindowSize(window, &width, &height);
        y /= height;
        x /= width;
        y *= 75;
        x *= 75;
        std::cout << "x: " << x << " y: " << y << std::endl;
    }
}

input_2d::istream &input_2d::istream::operator>>(std::string &str)
{
    str = "";
    return *this;
}
