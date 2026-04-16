#include <stdio.h>
#include <unistd.h>

int main(void) {
    // With write directly
    write(1, "Hello from write()!\n", 20);

    // With puts
    puts("Hello from Unikraft!");
    return 0;
}
