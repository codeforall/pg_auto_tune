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
static int custom_processor(PGConfigMapEntry *map_entry, SystemInfo *system_info);

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
        map_entry->conf_ref = PGConfig_get_param_by_name(pg_config,map_entry->param);
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

            case CUSTOM:
                custom_processor(map_entry, system_info);
                break;

            default:
                map_entry->status = ENTRY_PROCESSED_ERROR;
                snprintf(map_entry->message,MAX_MESSAGE_LEN, "Unsupported specified formula for parameter: \"%s\"",
                 map_entry->param);

                break;
        }
        map_entry = map_entry->next;
    }
}

static int
percentage_processor(PGConfigMapEntry *map_entry, SystemInfo *system_info)
{
    long factor_value;
    double ref_value = INVALID_DOUBLE_VAL;

    if(!map_entry)
        return -1;

    if (map_entry->conf_ref)
    {
        if(map_entry->conf_ref->type == PTYPE_INT)
            ref_value = (double)map_entry->conf_ref->int_val;
        else if(map_entry->conf_ref->type == PTYPE_FLOAT)
            ref_value = (double)map_entry->conf_ref->dec_val;
    }

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
        if (ref_value == map_entry->optimised_value)
            snprintf(map_entry->message,MAX_MESSAGE_LEN, "Optimised value for parameter: \"%s\" is already optimum (%lld) based on system memory = %lld bytes",
                 map_entry->param, (long long) map_entry->optimised_value,system_info->total_ram);
        else if (ref_value != INVALID_DOUBLE_VAL)
            snprintf(map_entry->message,MAX_MESSAGE_LEN, "Optimised value for parameter: \"%s\" is changed from %lld to %lld based on system memory = %lld bytes",
                 map_entry->param, (long long) ref_value, (long long) map_entry->optimised_value,system_info->total_ram);
        else
            snprintf(map_entry->message,MAX_MESSAGE_LEN, "Optimised value for parameter: \"%s\" is %lld based on system memory = %lld bytes",
                 map_entry->param, (long long) map_entry->optimised_value,system_info->total_ram);

        return 0;
    }
    else if (map_entry->resource == RESOURCE_CPU)
    {
        map_entry->optimised_value = (system_info->cpu_count * factor_value)/100;
        map_entry->status = ENTRY_PROCESSED_SUCCESS;

        if (ref_value == map_entry->optimised_value)
            snprintf(map_entry->message,MAX_MESSAGE_LEN, "Optimised value for parameter: \"%s\" is already optimum (%lld) based on system CPU = %ld",
                 map_entry->param, (long long) map_entry->optimised_value,system_info->cpu_count);
        else if (ref_value != INVALID_DOUBLE_VAL)
            snprintf(map_entry->message,MAX_MESSAGE_LEN, "Optimised value for parameter: \"%s\" is changed from %lld to %lld based on system CPU = %ld",
                 map_entry->param, (long long) ref_value,(long long) map_entry->optimised_value,system_info->cpu_count);
        else
            snprintf(map_entry->message,MAX_MESSAGE_LEN, "Optimised value for parameter: \"%s\" is %lld based on system CPU = %ld",
                 map_entry->param, (long long) map_entry->optimised_value,system_info->cpu_count);
        return 0;
    }
    else
    {
        snprintf(map_entry->message,MAX_MESSAGE_LEN, "Invalid Resource type: %s for parameter: %s. Only CPU and MEMOEY resources allowd for percentage processor",
                 get_resource_name(map_entry->resource),map_entry->param);
        map_entry->status = ENTRY_PROCESSED_ERROR;
    }

    return -2;
}

static int
custom_processor(PGConfigMapEntry *map_entry, SystemInfo *system_info)
{
    long factor_value;
    double ref_value = INVALID_DOUBLE_VAL;

    if(!map_entry)
        return -1;

    if (map_entry->conf_ref)
    {
        if(map_entry->conf_ref->type == PTYPE_INT)
            ref_value = (double)map_entry->conf_ref->int_val;
        else if(map_entry->conf_ref->type == PTYPE_FLOAT)
            ref_value = (double)map_entry->conf_ref->dec_val;
    }

    if (system_info->workload_type == OLAP)
        factor_value = map_entry->olap_value;
    else if (system_info->workload_type == OLTP)
        factor_value = map_entry->oltp_value;
    else
        factor_value = map_entry->mixed_value;

    if (map_entry->resource == RESOURCE_CUSTOM)
    {
        map_entry->optimised_value = factor_value;
        map_entry->status = ENTRY_PROCESSED_SUCCESS;
        if (ref_value == map_entry->optimised_value)
            snprintf(map_entry->message,MAX_MESSAGE_LEN, "Optimised value for parameter: \"%s\" is already optimum (%f)",
                 map_entry->param, map_entry->optimised_value);
        else if (ref_value != INVALID_DOUBLE_VAL)
            snprintf(map_entry->message,MAX_MESSAGE_LEN, "Optimised value for parameter: \"%s\" is changed from %f to %f",
                 map_entry->param,ref_value, map_entry->optimised_value);
        else
            snprintf(map_entry->message,MAX_MESSAGE_LEN, "Optimised value for parameter: \"%s\" is set to %f",
                 map_entry->param, map_entry->optimised_value);

        return 0;
    }
    else
    {
        snprintf(map_entry->message,MAX_MESSAGE_LEN, "Invalid Resource type: %s for parameter: %s. Only CUSTOM resource is allowd for CUSTOM processor",
                 get_resource_name(map_entry->resource),map_entry->param);
        map_entry->status = ENTRY_PROCESSED_ERROR;
    }

    return -2;
}