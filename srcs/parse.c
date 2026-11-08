#include "ft_traceroute.h"

#include <stdlib.h>
#include <getopt.h>


static void print_usage(void)
{
	printf("Usage:\n");
    printf("  ft_traceroute host\n");
    printf("Arguments:\n");
    printf("      host      The host to traceroute to\n");
}

t_config	parseInput(char **argv)
{
	t_config 	conf;
	memset(&conf, 0, sizeof(conf));
	conf.pid = (uint16_t)getpid();
	conf.timeout_ms = 5000;
    if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
            print_usage();
            exit (0);
    }
    else {
       conf.hostname = strdup(argv[1]); 
    }
    return conf;
}
