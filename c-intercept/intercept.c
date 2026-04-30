#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	write(1, "Hello from write()!\n", 20);
	dprintf(1, "Hello from dprintf()!\n");
	printf("Hello from printf()!\n");
	return 0;
}
