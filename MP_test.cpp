#include <iostream>
#include <cassert>
#include <time.h>
#include <vector>
#include <stack>

#include "MemoryPool.h"

using namespace std;

/* Adjust these values depending on how much you trust your computer */
#define ELEMS 1000000
#define REPS 50

int main() {

    clock_t start;

    MemoryPool<int> pool;
    start = clock();
    for (int i = 0;i < REPS;++i) {
        for (int j = 0;j < ELEMS;++j) {
            // 创建元素
            int* x = pool.newElement();

            // 释放元素
            pool.deleteElement(x);
        }
    }
    std::cout << "MemoryPool Time: ";
    std::cout << (((double)clock() - start) / CLOCKS_PER_SEC) << "\n\n";


    start = clock();
    for (int i = 0;i < REPS;++i) {
        for (int j = 0;j < ELEMS;++j) {
            int* x = new int;

            delete x;
        }
    }
    std::cout << "new/delete Time: ";
    std::cout << (((double)clock() - start) / CLOCKS_PER_SEC) << "\n\n";

    system("pause");
    return 0;
}
