#include <stdio.h>


int main() {
    int x=1;

    return *((char *) &x);
}