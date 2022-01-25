#ifndef MJ_INPUT_INPUT_HPP
#define MJ_INPUT_INPUT_HPP

#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <condition_variable>
#include <mutex>

namespace input_2d
{
void on_mouse_button(GLFWwindow *window, int button, int action, int mods);

void on_key(GLFWwindow *window, int key, int scancode, int action, int mods);

void inline trigger_render() { glfwPostEmptyEvent(); }

class istream : private std::istream
{
public:
    friend void on_mouse_button(GLFWwindow *window, int button, int action, int mods);

    istream(const istream &) = delete;
    istream &operator=(const istream &) = delete;

    static istream &get_instance();
    istream &operator>>(std::string &str);

    static void buffer(const std::string &str);

private:
    istream() = default;
    static std::string buf;
    static std::condition_variable cv;
    static std::mutex m;
};

}

namespace input_3d
{
void on_mouse_button(GLFWwindow *window, int button, int action, int mods);
}

#endif