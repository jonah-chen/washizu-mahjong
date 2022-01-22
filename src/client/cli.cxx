#define MJ_CLIENT_MODE_CLI
#include "game.hpp"

int main(int argc, char *argv[])
{
    unsigned int port = MJ_SERVER_DEFAULT_PORT;
    switch(argc)
    {
        case 3:
            port = std::stoi(argv[2]);
        case 2:
        {
            if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
            {
                std::cout << "Usage: " << argv[0] << " <host> [port]" << std::endl;
                return 1;
            }
            game g(std::cin, asio::ip::make_address(argv[1]), port);
            while (g.turn()) {}
            return 0;
        }
        case 1:
        {
            game g(std::cin, R::protocol::v4(), port);
            while (g.turn()) {};
            return 0;
        }
        default:
            std::cout << "Usage: " << argv[0] << " <host> [port]" << std::endl;
            return 1;
    }
}
