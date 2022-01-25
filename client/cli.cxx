#define MJ_CLIENT_MODE_CLI
#include "game.hpp"

int main(int argc, char *argv[])
{

#ifdef NDEBUG
    unsigned int port = MJ_SERVER_DEFAULT_PORT;
    if (argc == 2)
    {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
        {
            std::cout << "Usage: " << argv[0] << " [port (optional)]" << std::endl;
            return 0;
        }

        port = std::stoi(argv[1]);
    }


    std::cout << "Please enter the server's IP (or localhost):\n> ";

    while (true)
    {
        try
        {
            std::string ip_str;
            std::cin >> ip_str;

            if (ip_str == "localhost")
            {
                game g([](std::string&str){std::cin >> str;}, R::protocol::v4(), port);
                while (g.turn()) {};
                return 0;
            }

            game g([](std::string&str){std::cin >> str;}, asio::ip::make_address(ip_str), port);
            while (g.turn()) {}
            return 0;
        }
        catch (server_exception const &e)
        {
            std::cerr << "Server Error: " << e.what() << std::endl;
            return server_exception::ERROR_CODE;
        }
        catch(std::exception const &e)
        {
            std::cout << "Invalid IP address. Please try again.\n> ";
        }
    }
#else
    game g([](std::string&str){std::cin >> str;}, R::protocol::v4(), MJ_SERVER_DEFAULT_PORT);
    while (g.turn()) {}
    return 0;
#endif

}
