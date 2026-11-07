#include "ft_traceroute.h"

#include <stdlib.h>
#include <getopt.h>


static void print_usage(void)
{
	printf("Usage: ft_ping [-c COUNT] [-n] [-q] [-r] [-s SIZE]\n");
	printf("                [-t TTL] [-T TOS] [--ip-timestamp=FLAG] [-v] [-w TIMEOUT] [-W LINGER]\n");
	printf("                [-p PATTERN] [-6] [-?] [-h] HOST ...\n");
}

t_config	parseInput(char **argv)
{
	t_config 	conf;
	memset(&conf, 0, sizeof(conf));
	conf.pid = (uint16_t)getpid();
    if (strcmp(argv[1], "--help") || strcmp(argv[1], "-h")) {
            print_usage();
            exit (0);
    }
    else {
       strcpy(conf.hostname, argv[1]); 
    }
    return conf;
}
