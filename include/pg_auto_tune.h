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

#include "pg_parse_pgconfig.h"

#define MAX_MESSAGE_LEN 256

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

typedef enum RESOURCES
{
    RESOURCE_MEMORY,
    RESOURCE_CPU,
    RESOURCE_DISK,
    RESOURCE_WORKLOAD,
    RESOURCE_NODE_TYPE,
    RESOURCE_HOST_TYPE,
    INVALID_RESOURCE
} RESOURCES;

typedef enum FORMULAS
{
    PERCENTAGE,
    SCRIPT,
    INVALID_FORMULA
}FORMULAS;

typedef enum ENTRY_STATUS
{
    ENTRY_EMPTY = 0,
    ENTRY_LOADED,
    ENTRY_PROCESSED_SUCCESS,
    ENTRY_PROCESSED_ERROR
}ENTRY_STATUS;

typedef struct pg_config_map_entry PGConfigMapEntry;

struct pg_config_map_entry
{
    char *param;
    RESOURCES   resource;
    FORMULAS formula;
    long    oltp_value;
    long    olap_value;
    long    mixed_value;
    ENTRY_STATUS    status;

    /* These fields are used by processor */
    
    long optimised_value;
    char *message;
    PGConfigKeyVal  *conf_ref;

    /* Next item reference */
    PGConfigMapEntry *next;
};

typedef struct pg_config_map
{
    int num_entries;
    PGConfigMapEntry *list;

} PGConfigMap;

/* located in pg_config_processor.c */
void load_pg_config_in_map(PGConfigMap* config_map, PGConfig *pg_config);
void process_config_map(PGConfigMap* config_map, SystemInfo *system_info);

#endif  // __PG_AUTO_TUNE_H__