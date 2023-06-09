#include "PageRank.h"

int main()
{
    PageRank *prk = new PageRank(0.85, 1, 100, 100, 8);
    prk->process();
    delete prk;
}