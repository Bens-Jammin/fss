#include "stdlib.h"
#include "fss.hpp"
#include "fc/btree.h"

int main() {

    crawl(TEST_ROOT_DIRECTORY, true);

    namespace fc = frozenca;

    fc::BTreeMap<string, int> tree;

    tree["apple"]  = 1;
    tree["banana"] = 2;
    tree["cherry"] = 3;
    tree["date"]   = 4;
    tree["grape"]  = 5;

    // exact lookup
    std::cout << "apple -> " << tree["apple"] << "\n";

    // range scan: "b" to "d"
    auto lo = tree.lower_bound("b");
    auto hi = tree.lower_bound("e");
    std::cout << "range b-d: ";
    // very strange, ++it doesn't apply on the first loop. runs body then checks and increment. preincrement allows date to be included
    for (auto it = lo; it != hi; ++it)
        std::cout << it->first << " ";
    std::cout << "\n";

    // prefix scan: keys starting with "c"
    auto p = tree.lower_bound("c");
    std::cout << "prefix c: ";
    while (p != tree.end() && p->first.starts_with("c"))
        std::cout << (p++)->first << " ";
    std::cout << "\n";

    return 0;

}

