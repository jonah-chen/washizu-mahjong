#include <iostream>
#include "utils.hpp"

using namespace std::chrono_literals;

bool pcr;

int worker(int y)
{
    int x;
    std::cin >> x;
    return x + y;
}


int main()
{
    std::cout << _timeout(5s, worker, 0, 10) << std::endl;
    return 0;
}