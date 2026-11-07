#include "ft_traceroute.h"

int main(int argc, char **argv)
{
    (void)argc;
    t_config    conf;

    printf("---- ENTERING PARSING FUNCTION---\n");
    conf = parseInput(argv);
    printf("---- PARSING SUCCESS---\n");
    printf("hostname: %s\n", conf.hostname);
    
    if (resolveDNS(&conf) != 0)
    {
        fprintf(stderr, "ft_traceroute: hostname can't be resolved\n");
        return 1;
    }

 //   int ret = 0;
//    ret = ft_traceroute(&conf, &conf.target_addr, conf.ip_str);
    return (0);
}
