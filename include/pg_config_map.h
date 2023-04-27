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

/* DEBUG functions */
void print_config_map(PGConfigMap *config);
void print_config_map_entry(PGConfigMapEntry *entry);
#endif // __PG_CONFIG_MAP_H__