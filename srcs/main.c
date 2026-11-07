#include "ft_traceroute.h"

volatile sig_atomic_t g_sig = 0;

int main(int argc, char **argv)
{
    t_config    conf;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: ft_traceroute HOST\n");
        return (1);
    }
    printf("---- ENTERING PARSING FUNCTION---\n");
    conf = parseInput(argv);
    printf("---- PARSING SUCCESS---\n");
    printf("hostname: %s\n", conf.hostname);
    
    if (resolveDNS(&conf) != 0)
    {
        fprintf(stderr, "ft_traceroute: hostname can't be resolved\n");
        return 1;
    }
    int ret = 0;
    ret = ft_traceroute(&conf, &conf.target_addr, conf.ip_str);
    if (ret < 0)
        return (1);
    return (0);
}
