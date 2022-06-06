
#include "test/test.hpp"
#include "core/mahjong/mahjong.hpp"

TEST t_Construct()
{
    mj::Hand h1("m123345567p333sw22d");
    mj::Hand h2("m123345567p333w22");

    assert_crit(h1.size() == 14, "h1 should have 14 tiles", 255);
    assert_crit(h2.size() == 14, "h2 should have 14 tiles", 255);

    for (int i = 0; i < 14; ++i)
        assert(h1[i] == h2[i], "h1 and h2 should be equal");
    
    assert(!h1.agari().empty(), "h1 can win");
}

int main()
{
    t_Construct();
    return g_FailureCount;
}