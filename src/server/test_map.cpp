#include <map>

class asdf
{
public:
    asdf(int x, int y, int z)
    {
    }
};

int main()
{
    std::map<int, asdf> m;

    m.try_emplace(1, 2, 3, 3);
    return 0;
}