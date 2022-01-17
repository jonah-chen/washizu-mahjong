#include "receiver.hpp"
#include <iostream>
int main()
{
    R interface(R::protocall::v4(), 10000);
    
    // send a connection request
    msg::buffer msg = interface.recv();
    std::cout << static_cast<char>(msg::type(msg)) << " " << msg::data<unsigned short>(msg) << std::endl;
    interface.send(msg::header::join_as_player, msg::NEW_PLAYER);
    interface.send(msg::header::my_id, msg::data<unsigned short>(msg));
    while (1)
    {
        msg::buffer msg = interface.recv();
        // print the message
        std::cout << static_cast<char>(msg::type(msg)) << " " << msg::data<unsigned short>(msg) << std::endl;
    }
    return 0;
}