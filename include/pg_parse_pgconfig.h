/*-------------------------------------------------------------------------
 *
 * pg_parse_dbconfig.h
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

#ifndef __PG_PARSE_PGCONFIG_H__
#define __PG_PARSE_PGCONFIG_H__

typedef enum PARAM_TYPE
{
    PTYPE_CHAR,
    PTYPE_INT,
    PTYPE_FLOAT
} PARAM_TYPE;

typedef struct pg_config_key_value PGConfigKeyVal;
struct pg_config_key_value
{
    PARAM_TYPE type;
    char *key;
    char *value;
    u_int64_t int_val;
    float dec_val;
    PGConfigKeyVal *next;

};

typedef struct pg_config
{
    int num_params;
    PGConfigKeyVal *list;

} PGConfig;

//
#define MAX_CONFIG_CHAR_VAL 1024

//
PGConfig *PGConfig_parse(char *path);
void PGConfig_destroy(PGConfig *config);
PGConfigKeyVal* PGConfig_get_param_by_name(PGConfig *config, char *param_name);


#endif // __PG_PARSE_PGCONFIG_H__