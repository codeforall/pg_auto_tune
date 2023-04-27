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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

// Needed for ssize_t and other types
#include <sys/types.h>

// Provides isspace()
#include <ctype.h>

//
#include "pg_auto_tune.h"

#define CHAR_COMMENT '#' /* default comment chars */
#define CHAR_EQUAL '='   /* default key-val separator character */
#define CHAR_DOT '.'
#define CHAR_UNDERLINE '_'
#define CHAR_QUOTE '\''
#define CHAR_ESCAPE '\\'
#define CHAR_NLINE '\n'
#define NUM_LEN 64
#define NUM_UNIT_MAX 5
#define NUM_UNIT_KB (1024UL)
#define NUM_UNIT_MB (NUM_UNIT_KB * NUM_UNIT_KB)
#define NUM_UNIT_GB (NUM_UNIT_MB * NUM_UNIT_KB)
#define NUM_UNIT_TB (NUM_UNIT_GB * NUM_UNIT_KB)
#define NUM_UNIT_PB (NUM_UNIT_TB * NUM_UNIT_KB)

static bool PGConfig_parse_line(PGConfig *config, PGConfigKeyVal **curr_param, char *line);
static void PGConfigKeyVal_free(PGConfigKeyVal *param);

ssize_t line_nu = 0;

PGConfig *
PGConfig_parse(char *path)
{
    FILE *fp;
    PGConfig *config = NULL;
    PGConfigKeyVal *curr_param = NULL;

    // We try to open the file and return NULL in case of error
    if ((fp = fopen(path, "r")) == NULL)
    {
        perror("fopen()");
        return NULL;
    }

    config = calloc(1, sizeof *config);
    if (config == NULL)
    {
        perror("Not possible to allocate memory for the Parameters");
        return NULL;
    }

    size_t len = 0;
    ssize_t read;
    char *line = NULL;

    // We loop through all lines in the file
    while ((read = getline(&line, &len, fp)) != -1)
    {
        line_nu++;
        if (PGConfig_parse_line(config, &curr_param, line) == false)
            fprintf(stderr, "Error at line %ld: %s\n", line_nu, line);
    }

    fclose(fp);
    if (line)
        free(line);

    /*
     */

    return config;
}

static bool
PGConfig_parse_line(PGConfig *config, PGConfigKeyVal **curr_param, char *line)
{
    //
    PGConfigKeyVal *param = NULL;
    param = calloc(1, sizeof *param);
    if (param == NULL)
    {
        perror("Not possible to allocate memory for the Parameters");
        return false;
    }

    // We need a temporary buffer to store values
    char char_val[MAX_CONFIG_CHAR_VAL];
    char num_val[NUM_LEN];
    char num_unit[NUM_UNIT_MAX];

    // We started reading something
    bool is_reading = false;

    // If the param value is an enclosed string or not
    // We only use it here to be able to read special characters inside of an enclosed string, specially the "#"
    bool is_enclosed = false;

    //
    PARAM_TYPE vtype = PTYPE_CHAR;

    // We'll store the string position to make the code more readable and less error prone
    ssize_t str_pos = 0;

    /**
     * We need to know which phase we are now:
     *  - 0: We didn't start reading anything
     *  - 1: We started reading the key NAME
     *  - 2: We just started reading the key VALUE
     *  - 3: We are in the middle of reading the key VALUE
     *  - 4: We FINISHED reading the key VALUE
     */
    u_int8_t phase = 0;

    // Let's clean our buffer before we start working
    memset(&char_val, 0, MAX_CONFIG_CHAR_VAL);
    memset(&num_val, 0, NUM_LEN);
    memset(&num_unit, 0, NUM_UNIT_MAX);

    char c;
    while ((c = *line++) != CHAR_NLINE)
    {
        // We are reading
        if (is_reading)
        {
            switch (phase)
            {
            // And we are reading the key NAME
            case 1:
                // If we find a space or an equals sign (=) it means we reached to the end of the key name
                if (isspace(c) ||
                    c == CHAR_EQUAL)
                {
                    is_reading = false;
                    line++;

                    param->key = calloc(strlen(char_val), sizeof(char));
                    if (param->key == NULL)
                    {
                        perror("Not possible to allocate memory for the Parameters");
                        return false;
                    }
                    memcpy(param->key, char_val, strlen(char_val));

                    // printf("We found a Key: %s\n", tmp);
                }
                // We also need to check the character in the key name is valid
                else if (isdigit(c) || isalpha(c) || c == CHAR_UNDERLINE)
                {
                    char_val[str_pos++] = c;
                }
                else
                    return false;
                break;

            // We are now reading the key VALUE
            case 2:
                // We skip the spaces while not reading any value yet
                if (isspace(c))
                {
                    continue;
                }

                // Before we start populating anything we need to clean the buffer
                memset(&char_val, 0, MAX_CONFIG_CHAR_VAL);
                str_pos = 0;

                // We also need to check the character in the key name is valid
                // Is it a numeric value?
                if (isdigit(c) || isalpha(c))
                {
                    is_enclosed = false;
                }
                // Maybe a string value?
                else if (c == CHAR_QUOTE)
                {
                    is_enclosed = true;
                    line++;
                    c = *line;
                }
                else
                    return false;

                if (isdigit(c))
                {
                    vtype = PTYPE_INT;
                    num_val[strlen(num_val)] = c;
                }

                char_val[str_pos++] = c;
                phase++;
                break;

            case 3:

                // We check if we are dealing with an enclosed string
                if (is_enclosed)
                {
                    // We found a closing string character
                    if (c == CHAR_QUOTE)
                    {
                        is_reading = false;
                        is_enclosed = false;
                    }
                    // We found a scaping character, we need to check if the next one is the closing string char to escape it
                    else if (c == CHAR_ESCAPE)
                    {
                        if (*line == CHAR_QUOTE)
                            line++;
                    }

                    char_val[str_pos++] = c;
                }
                // or a number
                else
                {
                    // We check if this is a valid number, a valid string, or a dot
                    //(isdigit(c) || isalpha(c) || c == CHAR_UNDERLINE)
                    if (isdigit(c) || isalpha(c) || c == CHAR_DOT)
                        char_val[str_pos++] = c;

                    // If we find a space we just stop appending
                    else if (isspace(c))
                        is_reading = false;

                    // If none of the above then we have an invalid input
                    else
                        return false;
                }

                if (vtype != PTYPE_CHAR)
                {
                    if (isdigit(c) || c == CHAR_DOT)
                    {
                        if (c == CHAR_DOT)
                            vtype = PTYPE_FLOAT;

                        num_val[strlen(num_val)] = c;
                    }
                    else if (isalpha(c))
                    {
                        if (strlen(num_unit) >= NUM_UNIT_MAX)
                        {
                            fprintf(stderr, "Error invalid number unit at line %ld\n", line_nu);
                            return false;
                        }
                        else
                            num_unit[strlen(num_unit)] = c;
                    }
                }
                break;

            default:
                return false;
            }
        }
        else
        {
            // We just keep looping if we find space
            if (isspace(c))
                continue;

            // If a comment we break the loop
            if (c == CHAR_COMMENT)
                break;

            // We found a valid string. It's either a key NAME or a key VALUE depending on the "phase"
            is_reading = true;
            phase++;

            // Workaround to not lose the first character of the string because we already incremented above... there might be a nicer way to do it but I'm a bit lazy now lol
            line--;
        }
    }

    if (phase >= 3)
    {
        param->value = calloc(strlen(char_val), sizeof(char));
        if (param->value == NULL)
        {
            perror("Not possible to allocate memory for the Parameters");
            return false;
        }
        memcpy(param->value, char_val, strlen(char_val));
        if (vtype != PTYPE_CHAR)
        {
            if (vtype == PTYPE_INT)
            {
                param->int_val = atol(num_val);
                if (strlen(num_unit) > 0)
                {
                    if (!strcasecmp("kb", num_unit))
                        param->int_val *= NUM_UNIT_KB;
                    else if (!strcasecmp("mb", num_unit))
                        param->int_val *= NUM_UNIT_MB;
                    else if (!strcasecmp("gb", num_unit))
                        param->int_val *= NUM_UNIT_GB;
                    else if (!strcasecmp("tb", num_unit))
                        param->int_val *= NUM_UNIT_TB;
                    else if (!strcasecmp("pb", num_unit))
                        param->int_val *= NUM_UNIT_PB;
                }
            }
            else
            {
                param->dec_val = atof(num_val);
                // printf("%s: %.4f\n", param->key, param->dec_val);
            }
        }

        if (*curr_param == NULL)
        {
            *curr_param = param;
            config->list = param;
        }
        else
        {
            (*curr_param)->next = param;
            *curr_param = (*curr_param)->next;
        }

        config->num_params++;
    }

    return true;
}

void PGConfig_destroy(PGConfig *config)
{
    if (config != NULL)
    {
        PGConfigKeyVal *next_param;
        while ((next_param = config->list->next) != NULL)
        {
            config->list->next = next_param->next;
            PGConfigKeyVal_free(next_param);
        }

        PGConfigKeyVal_free(config->list);
        free(config);
    }
}

static void
PGConfigKeyVal_free(PGConfigKeyVal *param)
{
    if (param->key != NULL)
        free(param->key);

    if (param->value != NULL)
        free(param->value);

    free(param);
}

PGConfigKeyVal *
PGConfig_get_param_by_name(PGConfig *config, char *param_name)
{
    PGConfigKeyVal *val;
    if (!config || !param_name)
        return NULL;

    val = config->list;
    while (val)
    {
        if (val->key && !strcasecmp(val->key, param_name))
            return val;
        val = val->next;
    }
    return NULL;
}