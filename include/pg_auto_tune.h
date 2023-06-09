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
#include <stdbool.h>
#include "pg_parse_pgconfig.h"

#define MAX_MESSAGE_LEN 256
#define INVALID_DOUBLE_VAL  999999999.99

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
    RESOURCE_CUSTOM,
    INVALID_RESOURCE
} RESOURCES;

typedef enum FORMULAS
{
    PERCENTAGE,
    SCRIPT,
    CUSTOM,
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
    PARAM_TYPE type;
    char *value;
    // union 
    // {
    //     char *char_val;
    //     uint64_t int_val;
    //     double dbl_val;
    // };
    
    // double    oltp_value;
    // double    olap_value;
    // double    mixed_value;
    
    double    trigger_value;
    ENTRY_STATUS    status;

    /* These fields are used by processor */
    
    double optimised_value;
    char message[MAX_MESSAGE_LEN];
    PGConfigKeyVal  *conf_ref;

    /* Next item reference */
    PGConfigMapEntry *next;
};

typedef struct pg_map_profile_details
{
    long    min_memory;
    long    min_cpu;
    long    max_memory;
    long    max_cpu;
    char*   name;
    char*   author;
    char*   description;
    char*   version;
    char*   date_created;
    char*   engine;
}PGMapProfileDetails;

typedef struct pg_config_map
{
    int num_entries;
    PGConfigMapEntry *list;

} PGConfigMap;

/* located in pg_config_processor.c */
void load_pg_config_in_map(PGConfigMap* config_map, PGConfig *pg_config);
void process_config_map(PGConfigMap* config_map, SystemInfo *system_info);

#endif  // __PG_AUTO_TUNE_H__