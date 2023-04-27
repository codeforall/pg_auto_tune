/*-------------------------------------------------------------------------
 *
 * pg_config_map.h
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
#ifndef __PG_CONFIG_MAP_H__
#define __PG_CONFIG_MAP_H__

#include "pg_parse_pgconfig.h"
#include "pg_auto_tune.h"

#define MAX_LINE 2048
#define MAX_TOKEN_LEN 512

// 

int load_config_map(PGConfigMap *config, char *map_file);
void free_config_map(PGConfigMap *config);
char *get_resource_name(RESOURCES res);
char *get_formula_name(FORMULAS formula);

RESOURCES identify_resource(char* token);
FORMULAS identify_formula(char* token);

int load_json_config_map(PGConfigMap* config, PGMapProfileDetails* profile, const char *file_path);

void print_config_map(PGConfigMap* config, SystemInfo *sys_info, bool report);
void create_postgresql_conf(const char *output_file_path,PGConfigMap* config, SystemInfo *sys_info);

#endif // __PG_CONFIG_MAP_H__