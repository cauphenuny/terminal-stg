#ifndef FUNC_H
#define FUNC_H

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}

bool probability(int x, int y) {
    return (rand() % y) <= (x - 1);
}

#endif
