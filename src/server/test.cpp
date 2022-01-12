#define ASIO_STANDALONE
#include <asio.hpp>
#include <string>
#include <iostream>
#include <thread>

#include <vector>

std::vector<asio::ip::tcp::socket> players;

void accept_connections(asio::io_context &i, asio::ip::tcp::acceptor &a)
{
    while(players.size() < 10)
    {
        std::cout << "handle_connection" << std::endl;
        asio::ip::tcp::socket socket(i);
        std::cout << "accept" << std::endl;
        a.accept(socket);
        std::cout << "accepted" << std::endl;
        players.push_back(std::move(socket));
    }
}

void handle_connection(asio::ip::tcp::socket &s)
{
    std::string x = "123456789012";
    asio::write(s, asio::buffer(x));
    asio::read(s, asio::buffer(x));
    std::cout << x << std::endl;
}

void server_debug_terminal()
{
    int x, y;
    std::cin >> x >> y;
    std::cout << x << " " << y << std::endl;
}

void handle_connections()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        if (players.size() >= 3)
        {
            std::cout << "handle_connection" << std::endl;

            auto &s = players.back();

            std::thread t1(handle_connection, std::ref(s));

            t1.join();

            players.erase(players.begin(), players.begin() + 2);
        }
    }
}

int main()
{
    // 10 byte buffer string
    std::string buffer(10, '\0');
    std::string data = "asdflkjasdflkj";

    // Start a server on localhost
    asio::io_context io_context;
    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), 8080);
    asio::ip::tcp::acceptor acceptor(io_context, endpoint);

    // Start accepting connections
    std::thread t1(accept_connections, io_context, acceptor);
    std::thread t2(server_debug_terminal);
    std::thread t3(handle_connections);

    t1.join();
    t2.join();
    t3.join();

    std::cout << "done" << std::endl;
    return 0;
}