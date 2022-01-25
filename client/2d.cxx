#define MJ_CLIENT_MODE_2D
#include "renderer/2d.hpp"
#include "game.hpp"

int main(int argc, char *argv[])
{
    mj_hand h1, h2, h3, h4;

    mj_parse("123444m44p55sw2d", &h1);
    mj_parse("123444m44p55sw2d", &h2);
    mj_parse("144m44p55s234w23d", &h3);
    mj_parse("144m44p55s234w23d", &h4);
    renderer2d::submit(h1, MJ_EAST);
    renderer2d::submit(h2, MJ_SOUTH);
    renderer2d::submit(h3, MJ_WEST);
    renderer2d::submit(h4, MJ_NORTH);
    glClearColor(0.1f, 0.3f, 0.f, 1.f);

    glfwSetMouseButtonCallback(renderer2d::window_ptr(), input::on_mouse_button);
    while (!glfwWindowShouldClose(renderer2d::window_ptr()))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        renderer2d::flush();

        glfwSwapBuffers(renderer2d::window_ptr());
        glfwWaitEvents();
    }
    return 0;
}
