#include <iostream>
#include "utils.hpp"

using namespace std::chrono_literals;

int worker(int y)
{
    int x;
    std::cin >> x;
    return x + y;
}


int main()
{
    std::cout << _timeout(5000, worker, 0, 10) << std::endl;
    return 0;
}