#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include <ctype.h>

#include "pg_config_map.h"

static char *get_next_token(char *buf, char *token, int max_token_len, int* token_len);

int
load_config_map(PGConfigMap* config, char *map_file)
{
    FILE *file = NULL;
    int         line_number = 1;

    config->list = NULL;
    config->num_entries = 0;

    file = fopen(map_file, "r");
    if (!file)
    {
        fprintf(stderr, "Failed to open map file %s:%s\n", map_file, strerror(errno));
        return -1;
    }
    while (!feof(file) && !ferror(file))
    {
        char    rawline[MAX_LINE];
        char    *lineptr;
        char    token[MAX_TOKEN_LEN];
        int     token_len = 0;
        if (!fgets(rawline, sizeof(rawline), file))
        {
            int     save_errno = errno;
    
            if (!ferror(file))
                break;          /* normal EOF */
            /* I/O error! */
            fprintf(stderr,"could not read file \"%s\": %s", map_file, strerror(save_errno));
            rawline[0] = '\0';
        }
        if (strlen(rawline) == MAX_LINE - 1)
        {  
            /* Line too long! */
            fprintf(stderr,"map line:%d too long in file \"%s\"", line_number,map_file);

        }
            
        /* Strip trailing linebreak from rawline */
        lineptr = rawline + strlen(rawline) - 1;
        while (lineptr >= rawline && (*lineptr == '\n' || *lineptr == '\r'))
            *lineptr-- = '\0';

        /* follow the easy route, get each token one by one */
        int step = 0;
        lineptr = rawline;
        PGConfigMapEntry *entry = NULL;
        entry = malloc(sizeof *entry);
        memset(entry,0x00,sizeof *entry);
        entry->status = ENTRY_EMPTY;
        if (entry == NULL)
        {
            perror("Not possible to allocate memory for the Parameters");
            fclose(file);
            return -2;
        }
        entry->next = NULL;

        while (step < 6)
        {
            lineptr = get_next_token(lineptr, token, MAX_TOKEN_LEN, &token_len);
            if (!token_len)
                break;
            switch (step)
            {
                case 0: /* param name */
                    entry->param = strdup(token);
                break;

                case 1:/* resource type */
                    entry->resource = identify_resource(token);
                break;

                case 2:/* formula */
                    entry->formula = identify_formula(token);
                break;

                case 3:/* OLTP */
                    entry->oltp_value = atof(token);
                break;

                case 4:/* OLAP */
                    entry->olap_value = atof(token);
                break;

                case 5:/* mixed */
                    entry->mixed_value = atof(token);
                break;
            }
            step++;
        }
        if (step == 6)
        {
            /* add it to list */
            if (config->list != NULL)
                entry->next = config->list;
            entry->status = ENTRY_LOADED;
            config->list = entry;
            config->num_entries++;
        }
        else if (entry)
        {
            free(entry);
            entry = NULL;
        }
    }
    fclose(file);
    return config->num_entries;
}

RESOURCES
identify_resource(char* token)
{
    if (!token)
        return INVALID_RESOURCE;
    if (!strcasecmp("MEMORY",token))
        return RESOURCE_MEMORY;
    if (!strcasecmp("CPU",token))
        return RESOURCE_CPU;
    if (!strcasecmp("DISK",token))
        return RESOURCE_DISK;
    if (!strcasecmp("WORKLOAD",token))
        return RESOURCE_WORKLOAD;
    if (!strcasecmp("NODE_TYPE",token))
        return RESOURCE_NODE_TYPE;
    if (!strcasecmp("HOST_TYPE",token))
        return RESOURCE_HOST_TYPE;
    if (!strcasecmp("CUSTOM",token))
        return RESOURCE_CUSTOM;

    return INVALID_RESOURCE;
}


FORMULAS
identify_formula(char* token)
{
    if (!token)
        return INVALID_FORMULA;
    if (!strcasecmp("PERCENTAGE",token))
        return PERCENTAGE;
    if (!strcasecmp("SCRIPT",token))
        return SCRIPT;
    if (!strcasecmp("CUSTOM",token))
        return CUSTOM;
    return INVALID_FORMULA;
}

char*
get_workload_type(WORKLOAD_TYPE wrk)
{
    switch (wrk)
    {
    case OLTP:
        return "OLTP";
        break;
    case OLAP:
        return "OLTP";
        break;
    case MIXED:
        return "MIXED";
        break;    
    default:
        return "UNKNOWN";
        break;
    }
}
char*
get_resource_name(RESOURCES res)
{
    switch (res)
    {
        case RESOURCE_MEMORY:
            return "MEMORY";
            break;
        case RESOURCE_CPU:
            return "CPU";
            break;
        case RESOURCE_DISK:
            return "DISK";
            break;
        case RESOURCE_WORKLOAD:
            return "WORKLOAD";
            break;
        case RESOURCE_NODE_TYPE:
            return "NODE_TYPE";
            break;
        case RESOURCE_HOST_TYPE:
            return "HOST_TYPE";
            break;
        case RESOURCE_CUSTOM:
            return "CUSTOM_RESOURCE";
            break;
        default:
            return "INVALID_RESOURCE";
            break;
    }
}

char*
get_formula_name(FORMULAS formula)
{
    switch (formula)
    {
        case PERCENTAGE:
           return "PERCENTAGE";
        break;
        case SCRIPT:
           return "SCRIPT";
        break;
        case CUSTOM:
           return "CUSTOM";
        break;
        default:
            return "INVALID_FORMULA";
        break;
    }
}

static char *
get_next_token(char *buf, char *token, int max_token_len, int *token_len)
{       
    char    *tbuf,
            *p;

    *token_len = 0;    
    *token = '\0';

    if (buf == NULL)
        return NULL;

    tbuf = buf;
    p = token;

    /* Move over any whitespaces preceding the next token */
    while (*tbuf != 0 && isspace(*tbuf))
        tbuf++;

    /* If its a comment, ignore till end */
    if (*tbuf == '#')
        return NULL;

    while (*tbuf != 0 && *token_len < max_token_len)
    {   
        if (isspace(*tbuf))
        {   
            *p = '\0';
            return tbuf + 1;
        }
        /* just copy to the tok */
        *p++ = *tbuf++;
        *token_len = *token_len + 1;
    }
    *p = '\0';
    return NULL;
 }

void
free_config_map(PGConfigMap* config)
{
    PGConfigMapEntry *entry;
    if (!config)
    {
        printf("LOG: Config Map is NULL\n");
        return;
    }
    entry = config->list;
    while(entry)
    {
        PGConfigMapEntry *tmp;;
        if(entry->param)
            free(entry->param);
        tmp = entry;
        entry = entry->next;
        free(tmp);
    }

}


static void
print_config_map_entry_report(PGConfigMapEntry *entry, SystemInfo *sys_info)
{
    if (!entry)
    {
        printf("LOG: Config Map entry is NULL\n");
        return;
    }

    printf("*** Processed parameter \"%s\": ",entry->param?entry->param:"NULL");
    printf("Using RESOURCE:%s: ",get_resource_name(entry->resource));
    printf("with FORMULA:%s: ",get_formula_name(entry->formula));
    printf("for [%s] workload\n",get_workload_type(sys_info->workload_type));
    printf("RESULT: %s\n",entry->message);
}

/* debug function */
void
print_config_map_entry(PGConfigMapEntry *entry)
{
    if (!entry)
    {
        printf("LOG: Config Map entry is NULL\n");
        return;
    }
    printf("%s: ",entry->param?entry->param:"NULL");
    printf("%s: ",get_resource_name(entry->resource));
    printf("%s: ",get_formula_name(entry->formula));
    printf("OLTP:%f OLAP:%f MIXED:%f\n",entry->oltp_value,entry->olap_value,entry->mixed_value);

    if (entry->status == ENTRY_PROCESSED_SUCCESS)
        printf("\t optimised_value=%f",entry->optimised_value);
    else if (entry->status == ENTRY_PROCESSED_ERROR)
        printf("\t *processing_error*");
    if (entry->conf_ref)
        printf("\t pg_conf_value:%s",entry->conf_ref->value?entry->conf_ref->value:"NIL");
    printf("\n");
}

void
print_config_map(PGConfigMap* config, SystemInfo *sys_info, bool report)
{
    int i;
    PGConfigMapEntry *entry;
    if (!config)
    {
        printf("LOG: Config Map is NULL\n");
        return;
    }
    entry = config->list;
    while(entry)
    {
        printf("[%d]:\t",i++);
        if (report)
            print_config_map_entry_report(entry,sys_info);
        else
            print_config_map_entry(entry);
        entry = entry->next;
    }
    printf("\n\n");
}

void
create_postgresql_conf(const char *output_file_path,PGConfigMap* config, SystemInfo *sys_info)
{
    FILE *fp;
    PGConfigMapEntry *entry;
    if (!config)
    {
        printf("LOG: Config Map is NULL\n");
        return;
    }
    entry = config->list;
    fp = fopen(output_file_path, "w+");

    while(entry)
    {
        if (entry->status == ENTRY_PROCESSED_SUCCESS)
        {
            fprintf(fp, "%s = ", entry->param);
            if (entry->resource == RESOURCE_MEMORY || entry->resource == RESOURCE_CPU)
                fprintf(fp, "%lld\n",(long long) entry->optimised_value);
            else
                fprintf(fp, "%f\n", entry->optimised_value);
        }
        entry = entry->next;
    }
    fclose(fp);
    printf("\nLOG: configuration file \"%s\"generated\n",output_file_path);
}
