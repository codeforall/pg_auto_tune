/*-------------------------------------------------------------------------
 *
 * pg_config_processor.c
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
#include<stdio.h>
#include<stdlib.h>

#include "pg_config_map.h"

static int percentage_processor(PGConfigMapEntry *map_entry, SystemInfo *system_info);

void
load_pg_config_in_map(PGConfigMap* config_map, PGConfig *pg_config)
{
    PGConfigMapEntry    *map_entry;
    if (!config_map || !pg_config)
    {
        printf("LOG: Config Map or PG config is NULL\n");
        return;
    }
    map_entry = config_map->list;
    while(map_entry)
    {
        map_entry->conf_ref = get_config_for_param(pg_config,map_entry->param);
        map_entry = map_entry->next;
    }
}


void
process_config_map(PGConfigMap* config_map, SystemInfo *system_info)
{
    PGConfigMapEntry    *map_entry;
    if (!config_map || !system_info)
    {
        printf("LOG: Failed to process config map: Config Map or System Info is missing\n");
        return;
    }
    map_entry = config_map->list;
    while(map_entry)
    {
        switch (map_entry->formula)
        {
            case PERCENTAGE:
                percentage_processor(map_entry, system_info);
                break;
            
            default:
                map_entry->status = ENTRY_PROCESSED_ERROR;
                break;
        }
        map_entry = map_entry->next;
    }
}

static int
percentage_processor(PGConfigMapEntry *map_entry, SystemInfo *system_info)
{
    long factor_value;
    if(!map_entry)
        return -1;

    if (system_info->workload_type == OLAP)
        factor_value = map_entry->olap_value;
    else if (system_info->workload_type == OLTP)
        factor_value = map_entry->oltp_value;
    else
        factor_value = map_entry->mixed_value;

    if (map_entry->resource == RESOURCE_MEMORY)
    {
        map_entry->optimised_value = (system_info->total_ram * factor_value)/100;
        map_entry->status = ENTRY_PROCESSED_SUCCESS;
        return 0;
    }
    else if (map_entry->resource == RESOURCE_CPU)
    {
        map_entry->optimised_value = (system_info->cpu_count * factor_value)/100;
        map_entry->status = ENTRY_PROCESSED_SUCCESS;
        return 0;
    }
    else
    {
        char* message = malloc(MAX_MESSAGE_LEN);
        snprintf(message,MAX_MESSAGE_LEN, "Invalid Resource type: %s for parameter: %s",
                 get_resource_name(map_entry->resource),map_entry->param);
        map_entry->message = message;
        map_entry->status = ENTRY_PROCESSED_ERROR;
    }

    return -2;
}