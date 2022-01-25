#define MJ_CLIENT_MODE_2D
#include "renderer/2d.hpp"
#include "game.hpp"
#include <thread>

void run_game()
{
    game g(input::istream::get, R::protocol::v4(), MJ_SERVER_DEFAULT_PORT);
    while (g.turn())
        {}
}


int main(int argc, char *argv[])
{
    std::thread t(run_game);
    t.detach();
    glfwSetMouseButtonCallback(renderer2d::window_ptr(), input::on_mouse_button);
    glClearColor(0.1f, 0.4f, 0.0f, 0.5f);
    while (!glfwWindowShouldClose(renderer2d::window_ptr()))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        renderer2d::flush();

        glfwSwapBuffers(renderer2d::window_ptr());
        glfwPollEvents();
    }
    return 0;
}
