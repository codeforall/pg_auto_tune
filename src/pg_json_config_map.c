#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "json.h"

#include "pg_config_map.h"
/* profile keys */
#define NAME_KEY "name"
#define VERSION_KEY "version"
#define DATE_CREATED_KEY "date_created"
#define ENGINE_KEY "engine"
#define AUTHOR_KEY "author"
#define DESCRIPTION_KEY "description"
#define MIN_MEMORY_KEY "min_memory"
#define MIN_CPU_KEY "min_cpu"
#define MAX_MEMORY_KEY "max_memory"
#define MAX_CPU_KEY "max_cpu"

/* map entriy keys */
#define CONFIG_MAP_KEY "config_map"
#define PARAMETER_KEY "parameter"
#define RESOURCE_KEY "resource"
#define FORMULA_KEY "formula"
#define OLAP_FACTOR_KEY "olap_factor"
#define OLTP_FACTOR_KEY "oltp_factor"
#define MIXED_FACTOR_KEY "mixed_factor"
#define TRIGGER_KEY "trigger"

static PGConfigMapEntry *get_config_map_entry_from_json_obj(json_value *map_entry_json, SystemInfo *system_info);
static bool load_profile_details(json_value *map_entry_json, PGMapProfileDetails *pfofile);

int load_json_config_map(PGConfigMap *config, PGMapProfileDetails *profile, SystemInfo *system_info, const char *file_path)
{
    FILE *input_json;
    int seek_end_result;
    long tell_result;
    int seek_set_result;
    char *input_json_buffer;
    int i;
    size_t read_result;
    json_value *parsed_json;
    json_value *map_value = NULL;

    config->list = NULL;
    config->num_entries = 0;

    printf("DEBUG: Loading config map from file:%s\n", file_path);
    input_json = fopen(file_path, "rb");
    if (input_json == NULL)
    {
        fprintf(stderr, "Failed to read file %s reason:%s\n", file_path, strerror(errno));
        return -1;
    }

    seek_end_result = fseek(input_json, 0, SEEK_END);
    if (seek_end_result != 0)
    {
        fprintf(stderr, "fseek end error, %i\n", seek_end_result);
        fclose(input_json);
        return -1;
    }

    tell_result = ftell(input_json);
    if (tell_result < 0)
    {
        fprintf(stderr, "ftell error, %li\n", tell_result);
        fclose(input_json);
        return -1;
    }

    seek_set_result = fseek(input_json, 0, SEEK_SET);
    if (seek_set_result != 0)
    {
        fprintf(stderr, "fseek set error, %i\n", seek_set_result);
        fclose(input_json);
        return -1;
    }

    if (tell_result == 0)
    {
        input_json_buffer = (char *)calloc(1, 1);
    }
    else
    {
        input_json_buffer = (char *)malloc(tell_result);
    }
    if (input_json_buffer == NULL)
    {
        fprintf(stderr, "memory allocation failed, %li bytes\n", tell_result);
        fclose(input_json);
        return -1;
    }

    read_result = fread(input_json_buffer, 1, tell_result, input_json);
    fclose(input_json);
    if (read_result < (size_t)tell_result)
    {
        fprintf(stderr, "fread error, %lu\n", (unsigned long)read_result);
        free(input_json_buffer);
        return -1;
    }

    parsed_json = json_parse(input_json_buffer, tell_result);
    free(input_json_buffer);
    if (parsed_json == NULL)
    {
        fprintf(stderr, "Failed to parse json file %s\n", file_path);
        return -1;
    }

    if (load_profile_details(parsed_json, profile) == false)
    {
        fprintf(stderr, "Failed to load profile infromation from json file %s\n", file_path);
        return -1;
    }

    map_value = json_get_value_for_key(parsed_json, CONFIG_MAP_KEY);
    if (map_value == NULL || map_value->type != json_array)
    {
        fprintf(stderr, "Invalid Json. \"%s\" key not found\n", CONFIG_MAP_KEY);
        json_value_free(parsed_json);
        return -1;
    }
    if (map_value->u.array.length <= 0)
    {
        fprintf(stderr, "Invalid Json. \"%s\" does not contain any data\n", CONFIG_MAP_KEY);
        json_value_free(parsed_json);
        return -1;
    }
    printf("LOG: Trying to load config map containing %d entries\n", map_value->u.array.length);

    for (i = 0; i < map_value->u.array.length; i++)
    {
        json_value *map_entry = map_value->u.array.values[i];
        PGConfigMapEntry *entry = get_config_map_entry_from_json_obj(map_entry, system_info);
        if (entry)
        {
            if (config->list != NULL)
                entry->next = config->list;
            entry->status = ENTRY_LOADED;
            config->list = entry;
            config->num_entries++;
        }
    }
    json_value_free(parsed_json);
    return config->num_entries;
}

static bool
load_profile_details(json_value *map_entry_json, PGMapProfileDetails *profile)
{
    char *ptr;

    if (map_entry_json == NULL || map_entry_json->type != json_object)
    {
        fprintf(stderr, "Invalid Json object\n");
        return false;
    }

    ptr = json_get_string_value_for_key(map_entry_json, NAME_KEY);
    profile->name = ptr ? strdup(ptr) : strdup("no name");

    ptr = json_get_string_value_for_key(map_entry_json, VERSION_KEY);
    profile->version = ptr ? strdup(ptr) : strdup("no version info");

    ptr = json_get_string_value_for_key(map_entry_json, ENGINE_KEY);
    profile->engine = ptr ? strdup(ptr) : strdup("no Engine info");

    ptr = json_get_string_value_for_key(map_entry_json, AUTHOR_KEY);
    profile->author = ptr ? strdup(ptr) : strdup("no author info");

    ptr = json_get_string_value_for_key(map_entry_json, DESCRIPTION_KEY);
    profile->description = ptr ? strdup(ptr) : strdup("no description");

    ptr = json_get_string_value_for_key(map_entry_json, DATE_CREATED_KEY);
    profile->date_created = ptr ? strdup(ptr) : strdup("no date info");

    if (json_get_long_value_for_key(map_entry_json, MIN_MEMORY_KEY, &profile->max_memory))
        profile->min_memory = -1;
    if (json_get_long_value_for_key(map_entry_json, MIN_CPU_KEY, &profile->min_cpu))
        profile->min_cpu = -1;
    if (json_get_long_value_for_key(map_entry_json, MAX_MEMORY_KEY, &profile->max_memory))
        profile->max_memory = -1;
    if (json_get_long_value_for_key(map_entry_json, MAX_CPU_KEY, &profile->max_cpu))
        profile->max_cpu = -1;

    return true;
}

static PGConfigMapEntry *
get_config_map_entry_from_json_obj(json_value *map_entry_json, SystemInfo *system_info)
{
    PGConfigMapEntry *entry = NULL;
    char *ptr;

    if (map_entry_json == NULL || map_entry_json->type != json_object)
    {
        fprintf(stderr, "Invalid Json object\n");
        return NULL;
    }
    entry = malloc(sizeof *entry);
    memset(entry, 0x00, sizeof *entry);
    entry->status = ENTRY_EMPTY;
    if (entry == NULL)
    {
        perror("Not possible to allocate memory for the Parameters");
        return NULL;
    }
    entry->next = NULL;

    ptr = json_get_string_value_for_key(map_entry_json, PARAMETER_KEY);
    if (!ptr)
    {
        fprintf(stderr, "Invalid Json object, Json object does not contains required parameter\n");
        goto ERROR_EXIT;
    }
    entry->param = strdup(ptr);

    ptr = json_get_string_value_for_key(map_entry_json, RESOURCE_KEY);
    if (!ptr)
    {
        fprintf(stderr, "Invalid Json object, Json object does not contains required resource\n");
        goto ERROR_EXIT;
    }
    entry->resource = identify_resource(ptr);
    if (entry->resource == INVALID_RESOURCE)
    {
        fprintf(stderr, "Invalid Json object, parameter \"%s\" specifies invalid resource:\"%s\"\n", entry->param, ptr);
        goto ERROR_EXIT;
    }

    ptr = json_get_string_value_for_key(map_entry_json, FORMULA_KEY);
    if (!ptr)
    {
        fprintf(stderr, "Invalid Json object, Json object does not contains required formula\n");
        goto ERROR_EXIT;
    }
    entry->formula = identify_formula(ptr);
    if (entry->formula == INVALID_FORMULA)
    {
        fprintf(stderr, "Invalid Json object, parameter \"%s\" specifies invalid formula:\"%s\"\n", entry->param, ptr);
        goto ERROR_EXIT;
    }

    switch (system_info->workload_type)
    {
    case OLAP:
        entry->value = json_get_string_value_for_key(map_entry_json, OLAP_FACTOR_KEY);
        break;

    case OLTP:
        entry->value = json_get_string_value_for_key(map_entry_json, OLTP_FACTOR_KEY);
        break;

    case MIXED:
        entry->value = json_get_string_value_for_key(map_entry_json, MIXED_FACTOR_KEY);
        break;

    default:
        fprintf(stderr, "Invalid Json object, Json object does not contains required any valid workload Factor\n");
        goto ERROR_EXIT;
        break;
    }

    /* Trigger is optional */
    if (json_get_double_value_for_key(map_entry_json, TRIGGER_KEY, &entry->trigger_value))
    {
        entry->trigger_value = INVALID_DOUBLE_VAL;
    }

    return entry;

ERROR_EXIT:
    if (entry->param)
        free(entry->param);
    free(entry);
    return NULL;
}