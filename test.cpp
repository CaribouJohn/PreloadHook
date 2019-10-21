#include <iostream>
#include <cstdlib>

class test {
    int x;
};

int main( int argc, const char** argv) {
    void* chunk = malloc(16);
    test * p = new test();
    for ( int i = 0; i < 2000; i++) {
        chunk = realloc(chunk, 10);
        p = new test();
    }
    delete p;
    free(chunk);
    return 0;
}