#include "renderer/2d.hpp"
#include "input.hpp"

#include <iostream>
void input_2d::on_mouse_button(GLFWwindow *window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return;
    constexpr float X_SCALE = (renderer2d::PLAYFIELD_RIGHT - renderer2d::PLAYFIELD_LEFT) / renderer2d::WINDOW_WIDTH;
    constexpr float Y_SCALE = (renderer2d::PLAYFIELD_TOP - renderer2d::PLAYFIELD_BOTTOM) / renderer2d::WINDOW_HEIGHT;
    constexpr float X_OFFSET = renderer2d::PLAYFIELD_LEFT + renderer2d::HAND_OFFSET;
    constexpr float Y_OFFSET = renderer2d::PLAYFIELD_BOTTOM;
    double x, y;
    int width, height;
    glfwGetCursorPos(window, &x, &y);
    glfwGetWindowSize(window, &width, &height);
    y = (renderer2d::WINDOW_HEIGHT - y) * Y_SCALE - Y_OFFSET;
    x = x * X_SCALE - X_OFFSET;
    if (y < renderer2d::TILE_HEIGHT)
    {
        int tile = static_cast<int>(x / renderer2d::TILE_WIDTH) + 1;
        std::cout << "Clicked on tile at " << tile << std::endl;
        istream::buffer(std::to_string(tile));
    }
}

void input_2d::on_key(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS)
        return;
    switch (key)
    {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    case GLFW_KEY_R:
        istream::buffer("R");
        break;
    }
}

input_2d::istream &input_2d::istream::get_instance()
{
    static istream instance;
    return instance;
}

void input_2d::istream::get(std::string &str)
{
    get_instance().get_impl(str);
}
void input_2d::istream::get_impl(std::string &str)
{
    std::unique_lock lk(m);
    cv.wait(lk, [this] { return !buf.empty(); });
    str = std::move(buf);
    buf = "";
}

void input_2d::istream::buffer(const std::string &str)
{
    std::scoped_lock lk(m);
    buf = str;
    cv.notify_one();
}

std::string input_2d::istream::buf;
std::condition_variable input_2d::istream::cv;
std::mutex input_2d::istream::m;