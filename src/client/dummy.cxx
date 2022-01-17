#include "reciever.hpp"
#include <iostream>
int main()
{
    R interface(R::protocall::v4(), 10000);
    
    // send a connection request
    interface.send(msg::header::join_as_player, msg::NEW_PLAYER);
    while (1)
    {
        msg::buffer msg = interface.recv();
        // print the message
        std::cout << static_cast<char>(msg::type(msg)) << " " << msg::data<unsigned short>(msg) << std::endl;
    }
    return 0;
}