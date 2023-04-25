/*-------------------------------------------------------------------------
 *
 * pg_auto_tune.h
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

#ifndef __PG_AUTO_TUNE_H__
#define __PG_AUTO_TUNE_H__

typedef enum WORKLOAD_TYPE
{
    OLTP,
    OLAP,
    MIXED,
    UNKNOWN_WL
} WORKLOAD_TYPE;

typedef enum DISK_TYPE
{
    MAGNETIC,
    SSD,
    NETWORK,
    UNKNOWN_DT
} DISK_TYPE;

typedef enum NODE_TYPE
{
    PRIMARY,
    STANDBY,
    UNKNOWN_NT
} NODE_TYPE;

typedef enum HOST_TYPE
{
    POD,
    STANDARD,
    CLOUD,
    UNKNOWN_HOST
} HOST_TYPE;

typedef enum PARAM_TYPE
{
    CHAR,
    INT,
    FLOAT
} PARAM_TYPE;

typedef struct system_info
{
    long long total_ram;
    long cpu_count;
    double disk_speed;
    HOST_TYPE host_type;
    NODE_TYPE node_type;
    DISK_TYPE disk_type;
    WORKLOAD_TYPE workload_type;
} SystemInfo;

typedef struct pg_config_key_value PGConfigKeyVal;
struct pg_config_key_value
{
    char *key;
    char *value;
    PGConfigKeyVal *next;

};

typedef struct pg_config
{
    int num_params;
    PGConfigKeyVal *list;

} PGConfig;

#define MAX_CONFIG_CHAR_VAL 1024

PGConfig *PGConfig_parse(char *path);
void PGConfig_destroy(PGConfig *config);

#endif  // __PG_AUTO_TUNE_H__