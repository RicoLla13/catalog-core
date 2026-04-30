#include <lwip/ip4_addr.h>
#include <lwip/netif.h>
#include <stdio.h>
#include <unistd.h>

static void print_lwip_default_interface(void) {
    if (!netif_default) {
        dprintf(1, "lwIP default interface: <none>\n");
        return;
    }

    dprintf(1, "lwIP default interface: %c%c%d\n", netif_default->name[0],
            netif_default->name[1], netif_default->num);
    dprintf(1, "  ip=%s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));
    dprintf(1, "  netmask=%s\n", ip4addr_ntoa(netif_ip4_netmask(netif_default)));
    dprintf(1, "  gateway=%s\n", ip4addr_ntoa(netif_ip4_gw(netif_default)));
}

static void print_argv(int argc, char *argv[])
{
    dprintf(1, "argc=%d\n", argc);
    for (int i = 0; i < argc; ++i)
        dprintf(1, "argv[%d]=%s\n", i, argv[i] ? argv[i] : "<null>");
}

int main(int argc, char *argv[]) {
    print_argv(argc, argv);
    print_lwip_default_interface();

    write(1, "Hello from write()!\n", 20);
    dprintf(1, "Hello from Unikraft!\n");
    dprintf(1, "Hello Printf\n");
    return 0;
}
