#include "message.hpp"
#include <iostream>

int main()
{
    constexpr auto buf = msg::buffer_data(msg::header::you_won, 99);
    std::cout << msg::data<unsigned short>(buf) << std::endl;
    return 0;
}