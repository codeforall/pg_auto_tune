/*-------------------------------------------------------------------------
 *
 * pg_auto_tune.c
 *		Utility to auto tune PostgreSQL configuration parameters based on system resources.
 *
 * Copyright © Percona LLC and/or its affiliates
 *
 * Hackathon Team one
 *  Abdul Sayeed
 *  Agustin Gallego
 *  Charly Batista
 *  Jobin Augustine
 *  Muhammad Usama
 *
 *-------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>

#include "pg_auto_tune.h"
#include "pg_config_map.h"

#define MAX_FILE_PATH_SIZE 1024

/* globalse */
int verbose_output = 0;
char *data_dir = NULL;
char *map_file = NULL;
const char *progname = "pg_auto_tune";
const char *description = "Auto tuning for PostgreSQL by Percona";
const char *package = "Percona";
const char *version = "1.0";
const char *speed_test_file = "base/1/1255";
const char *output_conf_file = "per_postgresql.conf";
const char *map_file_name = "ConfigParams.map";

static long long get_ram_size(void);
static int get_CPU_count(void);
static double get_disk_speed(const char *filePath);
static void create_postgresql_conf(char *data_dir, SystemInfo *sys_inf);
static void usage(void);

int main(int argc, char **argv)
{
    int ch;
    int optindex;
    char test_file_path[MAX_FILE_PATH_SIZE];
    char pgconf_file_path[MAX_FILE_PATH_SIZE];
    const char *allowed_options = "h:n:d:w:D:m:vV";
    PGConfig *pg_config;
    PGConfigMap config_map;

    SystemInfo system_info = {
        .total_ram = -1,
        .cpu_count = -1,
        .disk_speed = -1,
        .host_type = UNKNOWN_HOST,
        .node_type = UNKNOWN_NT,
        .disk_type = UNKNOWN_DT,
        .workload_type = UNKNOWN_WL};
    static struct option long_options[] = {
        {"help", no_argument, NULL, '?'},
        {"version", no_argument, NULL, 'V'},
        {"verbose", no_argument, NULL, 'v'},
        {"host-type", required_argument, NULL, 'h'},
        {"node-type", required_argument, NULL, 'n'},
        {"disk-type", required_argument, NULL, 'd'},
        {"workload-type", required_argument, NULL, 'w'},
        {"data-dir", required_argument, NULL, 'D'},
        {"map-file", required_argument, NULL, 'm'},
        {NULL, 0, NULL, 0}};

    if (argc > 1)
    {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
        {
            usage();
            exit(0);
        }
        else if (strcmp(argv[1], "-V") == 0 || strcmp(argv[1], "--version") == 0)
        {
            fprintf(stderr, "%s (%s) %s\n", progname, package, version);
            exit(0);
        }
    }

    while ((ch = getopt_long(argc, argv, allowed_options, long_options, &optindex)) != -1)
    {
        switch (ch)
        {
        case 'h': /*Host type*/
            if (strcmp(optarg, "p") == 0 || strcmp(optarg, "pod") == 0)
                system_info.host_type = POD;
            else if (strcmp(optarg, "s") == 0 || strcmp(optarg, "standard") == 0)
                system_info.host_type = STANDARD;
            else if (strcmp(optarg, "c") == 0 || strcmp(optarg, "cloud") == 0)
                system_info.host_type = CLOUD;
            else
            {
                fprintf(stderr, "%s: Invalid host type \"%s\", must be either \"pod\", \"standard\" or \"cloud\" \n", progname, optarg);
                exit(1);
            }
            break;

        case 'n': /*Node type*/
            if (strcmp(optarg, "p") == 0 || strcmp(optarg, "primary") == 0)
                system_info.node_type = PRIMARY;
            else if (strcmp(optarg, "s") == 0 || strcmp(optarg, "standby") == 0)
                system_info.node_type = STANDBY;
            else
            {
                fprintf(stderr, "%s: Invalid node type \"%s\", must be either \"primary\" or \"standby\" \n", progname, optarg);
                exit(1);
            }
            break;

        case 'd': /*Disk type*/
            if (strcmp(optarg, "m") == 0 || strcmp(optarg, "magnetic") == 0)
                system_info.disk_type = MAGNETIC;
            else if (strcmp(optarg, "s") == 0 || strcmp(optarg, "ssd") == 0)
                system_info.disk_type = SSD;
            else if (strcmp(optarg, "n") == 0 || strcmp(optarg, "network") == 0)
                system_info.disk_type = NETWORK;
            else
            {
                fprintf(stderr, "%s: Invalid disk type \"%s\", must be either \"magnetic\", \"network\" or \"ssd\" \n", progname, optarg);
                exit(1);
            }
            break;

        case 'w': /*Workload */
            if (strcmp(optarg, "l") == 0 || strcmp(optarg, "olap") == 0)
                system_info.workload_type = OLAP;
            else if (strcmp(optarg, "t") == 0 || strcmp(optarg, "oltp") == 0)
                system_info.workload_type = OLTP;
            else if (strcmp(optarg, "m") == 0 || strcmp(optarg, "mixed") == 0)
                system_info.workload_type = MIXED;
            else
            {
                fprintf(stderr, "%s: Invalid workload type \"%s\", must be either \"olap\", \"oltp\" or \"mixed\" \n", progname, optarg);
                exit(1);
            }
            break;

        case 'v':
            verbose_output = 1;
            break;

        case 'm':
            map_file = strdup(optarg);
            break;

        case 'D':
            data_dir = strdup(optarg);
            break;

        case '?':
        default:

            /*
             * getopt_long should already have emitted a complaint
             */
            fprintf(stderr, "Try \"%s --help\" for more information.\n\n", progname);
            exit(1);
        }
    }
    /*
     * if we still have arguments, use it
     */
    while (argc - optind >= 1)
    {
        if (data_dir == NULL)
        {
            data_dir = strdup(argv[optind]);
        }
        else
            fprintf(stderr, "%s: Warning: extra command-line argument \"%s\" ignored\n",
                    progname, argv[optind]);

        optind++;
    }

    if (data_dir == NULL)
    {
        fprintf(stderr, "%s: missing data-dir\n", progname);
        fprintf(stderr, "Try \"%s --help\" for more information.\n\n", progname);
        exit(1);
    }

    /* Ok, Done with trivial stuff, Get on with the real work */
    /* First gather all system info that we can */

    system_info.total_ram = get_ram_size();
    system_info.cpu_count = get_CPU_count();

    snprintf(test_file_path, MAX_FILE_PATH_SIZE, "%s/%s", data_dir, speed_test_file);
    system_info.disk_speed = get_disk_speed(test_file_path);

    snprintf(pgconf_file_path, MAX_FILE_PATH_SIZE, "%s/%s", data_dir, "postgresql.conf");

    /* Load configuration parameters from postgresql.conf */
    pg_config = PGConfig_parse(pgconf_file_path);

    /* Load the map file */
    load_config_map(&config_map, map_file ? map_file : map_file_name);
    if (verbose_output)
        print_config_map(&config_map);

    load_pg_config_in_map(&config_map, pg_config);
    process_config_map(&config_map, &system_info);

    print_config_map(&config_map);

    /* Enough with gathering info. create a meaningfull config */
    create_postgresql_conf(data_dir, &system_info);
    return 0;
}

static long long
get_ram_size(void)
{
    struct sysinfo info;

    if (sysinfo(&info) != 0)
    {
        fprintf(stderr, "Failed to retrieve system information %s:\n", strerror(errno));
        return -1;
    }

    if (verbose_output)
        printf("LOG: RAM size: %ld bytes\n", info.totalram);
    return info.totalram;
}

static int
get_CPU_count(void)
{
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    if (count < 1)
    {
        fprintf(stderr, "Failed to retrieve CPU core count %s:\n", strerror(errno));
        return -1;
    }

    if (verbose_output)
        printf("LOG: CPU core count: %ld\n", count);
    return count;
}

static void
create_postgresql_conf(char *data_dir, SystemInfo *sys_inf)
{
    FILE *fp;
    char output_file_path[MAX_FILE_PATH_SIZE];
    snprintf(output_file_path, MAX_FILE_PATH_SIZE, "%s/%s", data_dir, output_conf_file);

    if (verbose_output)
        printf("LOG: Optimizing configuration in: %s\n", output_file_path);

    // shared_buffers
    // max_worker_processes
    // work_mem
    // random_page_cost

    fp = fopen(output_file_path, "w+");
    fprintf(fp, "shared_buffers = %dGB\n", 10);
    fprintf(fp, "max_worker_processes = %ld\n", sys_inf->cpu_count);
    fclose(fp);
}

static double
get_disk_speed(const char *filePath)
{
    int fd;
    struct timeval start, end;

#define BUFFER_SIZE (1024 * 8)
    char buffer[BUFFER_SIZE];

    fd = open(filePath, O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr, "Failed to open file %s:\n", strerror(errno));
        return -1.0;
    }

    gettimeofday(&start, NULL);

    while (read(fd, buffer, BUFFER_SIZE) > 0)
        ;

    gettimeofday(&end, NULL);

    close(fd);

    double duration = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_usec - start.tv_usec) / 1000000.0;
    double speed = (double)BUFFER_SIZE / (1024.0 * 1024.0 * duration);

    if (verbose_output)
        printf("Disk speed: %.2f MB/s\n", speed);

    return speed;
}

static void
usage(void)
{
    fprintf(stderr, "%s - %s\n", progname, description);
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s [OPTION...] [data-dir]\n", progname);

    fprintf(stderr, "Options:\n");

    /*
     * print the command options
     */
    fprintf(stderr, "  -h, --host-type=TYPE        TYPE can be \"pod\", \"standard\", or \"cloud\"\n");
    fprintf(stderr, "  -n, --node-type=TYPE        TYPE can be \"primary\", or \"standby\"\n");
    fprintf(stderr, "  -d, --disk-type=TYPE        TYPE can be \"magnetic\", \"ssd\", or \"network\"\n");
    fprintf(stderr, "  -n, --worklload-type=TYPE   TYPE can be \"olap\", \"oltp\", or \"mixed\"\n");

    fprintf(stderr, "  -m, --file=file-path        path of config map file\n");
    fprintf(stderr, "  -D, --data-dir=DIR          location of the PostgreSQL data directory\n");

    fprintf(stderr, "  -v, --verbose               output verbose messages\n");
    fprintf(stderr, "  -V, --version               output version information and exit\n");
    fprintf(stderr, "  -?, --help                  print this help\n\n");
}