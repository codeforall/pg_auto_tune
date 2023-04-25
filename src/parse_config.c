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

#define COMMENT_CHARS "#" /* default comment chars */
#define KEYVAL_SEP '='    /* default key-val seperator character */
#define STR_TRUE "1"      /* default string valu of true */
#define STR_FALSE "0"     /* default string valu of false */

static bool PGConfig_parse_line(PGConfig *config, PGConfigKeyVal **curr_param, char *line);
static void PGConfigKeyVal_free(PGConfigKeyVal *param);
static bool CH_is_number(char c);
static bool CH_is_char(char c);
static bool CH_is_underscore(char c);

ssize_t line_nu = 0;

PGConfig *PGConfig_parse(char *path)
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

    config = malloc(sizeof *config);
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

static bool PGConfig_parse_line(PGConfig *config, PGConfigKeyVal **curr_param, char *line)
{
    //
    PGConfigKeyVal *param = NULL;
    param = malloc(sizeof *param);
    if (param == NULL)
    {
        perror("Not possible to allocate memory for the Parameters");
        return false;
    }

    // We need a temporary buffer to store values
    char tmp[MAX_CONFIG_CHAR_VAL];

    // We started reading something
    bool is_reading = false;

    // If the param value is an enclosed string or not
    // We only use it here to be able to read special characters inside of an enclosed string, specially the "#"
    bool is_str = false;

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
    memset(&tmp, 0, MAX_CONFIG_CHAR_VAL);

    char c;
    while ((c = *line++) != '\n')
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
                    c == KEYVAL_SEP)
                {
                    is_reading = false;
                    line++;

                    param->key = malloc(strlen(tmp) * sizeof(char));
                    if (param->key == NULL)
                    {
                        perror("Not possible to allocate memory for the Parameters");
                        return false;
                    }
                    memcpy(param->key, tmp, strlen(tmp));

                    // printf("We found a Key: %s\n", tmp);
                }
                // We also need to check the characther in the key name is valid
                else if (CH_is_number(c) || CH_is_char(c) || CH_is_underscore(c))
                {
                    tmp[str_pos++] = c;
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
                memset(&tmp, 0, MAX_CONFIG_CHAR_VAL);
                str_pos = 0;

                // We also need to check the characther in the key name is valid
                // Is it a numeric value?
                if (CH_is_number(c) || CH_is_char(c))
                {
                    is_str = false;
                }
                // Maybe a string value?
                else if (c == '\'')
                {
                    is_str = true;
                }
                else
                    return false;

                tmp[str_pos++] = c;
                phase++;
                break;

            case 3:

                // We check if we are dealing with an enclosed string
                if (is_str)
                {
                    // We found a closing string character
                    if (c == '\'')
                    {
                        is_reading = false;
                        is_str = false;
                    }
                    // We found a scaping character, we need to check if the next one is the closing string char to escape it
                    else if (c == '\\')
                    {
                        if (*line == '\'')
                        {
                            tmp[str_pos++] = '\'';
                            line++;
                        }
                    }

                    tmp[str_pos++] = c;
                }
                // or a number
                else
                {
                    // We check if this is a valid number, a valid string, or a dot
                    if (CH_is_number(c) || CH_is_char(c) || c == '.')
                        tmp[str_pos++] = c;

                    // If we find a space we just stop appending
                    else if (isspace(c))
                        is_reading = false;

                    // If none of the above then we have an invalid input
                    else
                        return false;
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
            if (c == '#')
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
        param->value = malloc(strlen(tmp) * sizeof(char));
        if (param->value == NULL)
        {
            perror("Not possible to allocate memory for the Parameters");
            return false;
        }
        memcpy(param->value, tmp, strlen(tmp));
        // printf("    Value: %s\n", tmp);

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

static void PGConfigKeyVal_free(PGConfigKeyVal *param)
{
    if (param->key != NULL)
        free(param->key);

    if (param->value != NULL)
        free(param->value);

    free(param);
}

static bool CH_is_number(char c)
{
    if (c >= '0' && c <= '9')
        return true;

    return false;
}

static bool CH_is_char(char c)
{
    if ((c >= 'A' && c <= 'Z')    // Upercase chars
        || (c >= 'a' && c <= 'z') // Lowercase chars
    )
        return true;

    return false;
}

static bool CH_is_underscore(char c)
{
    if (c == '_')
        return true;

    return false;
}
