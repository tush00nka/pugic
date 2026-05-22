#include "function.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void init_function_metadata_array(FunctionMetadataArray *array) {
    array->capacity = 100;
    array->count = 0;
    array->functions = malloc(array->capacity * sizeof(FunctionMetadata));
    memset(array->functions, 0, array->capacity * sizeof(FunctionMetadata));
}

void free_function_metadata_array(FunctionMetadataArray *array) {
    if (!array->functions) return;
    
    for (int i = 0; i < array->count; i++) {
        free(array->functions[i].schema);
        free(array->functions[i].name);
        free(array->functions[i].return_type);
        free(array->functions[i].language);
        free(array->functions[i].description);
        
        for (int j = 0; j < array->functions[i].arg_count; j++) {
            free(array->functions[i].arguments[j].name);
            free(array->functions[i].arguments[j].type);
            free(array->functions[i].arguments[j].mode);
            // Free enum values if present
            if (array->functions[i].arguments[j].is_enum) {
                for (int k = 0; k < array->functions[i].arguments[j].enum_count; k++) {
                    free(array->functions[i].arguments[j].enum_values[k]);
                }
                free(array->functions[i].arguments[j].enum_values);
            }
        }
        free(array->functions[i].arguments);
    }
    
    free(array->functions);
    array->functions = NULL;
    array->count = 0;
    array->capacity = 0;
}

// Get enum values for a specific type from PostgreSQL
char** get_enum_values(PGconn *conn, const char *type_name, int *count) {
    char query[512];
    snprintf(query, sizeof(query),
        "SELECT e.enumlabel "
        "FROM pg_type t "
        "JOIN pg_enum e ON t.oid = e.enumtypid "
        "WHERE t.typname = '%s' "
        "ORDER BY e.enumsortorder", type_name);
    
    PGresult *res = PQexec(conn, query);
    
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Failed to get enum values for %s: %s\n", type_name, PQerrorMessage(conn));
        if (res) PQclear(res);
        *count = 0;
        return NULL;
    }
    
    *count = PQntuples(res);
    char **values = malloc(*count * sizeof(char*));
    
    for (int i = 0; i < *count; i++) {
        values[i] = strdup(PQgetvalue(res, i, 0));
    }
    
    PQclear(res);
    return values;
}

// Check if a type is an enum
static int is_enum_type(PGconn *conn, const char *type_name) {
    char query[256];
    snprintf(query, sizeof(query),
        "SELECT EXISTS ("
        "  SELECT 1 FROM pg_type t "
        "  JOIN pg_enum e ON t.oid = e.enumtypid "
        "  WHERE t.typname = '%s'"
        ")", type_name);
    
    PGresult *res = PQexec(conn, query);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
        if (res) PQclear(res);
        return 0;
    }
    
    int is_enum = (strcmp(PQgetvalue(res, 0, 0), "t") == 0);
    PQclear(res);
    return is_enum;
}

void load_function_metadata(PGconn *conn, FunctionMetadataArray *array, const char *schema) {
    // Query that works with PostgreSQL 9.6 and later
    const char *query = 
        "SELECT "
        "  n.nspname as schema_name, "
        "  p.proname as func_name, "
        "  pg_get_function_result(p.oid) as return_type, "
        "  l.lanname as language, "
        "  d.description, "
        "  pg_get_function_arguments(p.oid) as arg_list "
        "FROM "
        "  pg_proc p "
        "  JOIN pg_namespace n ON p.pronamespace = n.oid "
        "  JOIN pg_language l ON p.prolang = l.oid "
        "  LEFT JOIN pg_description d ON p.oid = d.objoid "
        "WHERE "
        "  n.nspname = '%s' "
        "  AND p.prokind = 'f' "
        "ORDER BY "
        "  p.proname;";
    
    char full_query[4096];
    snprintf(full_query, sizeof(full_query), query, schema);
    
    PGresult *res = PQexec(conn, full_query);
    
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Failed to load functions: %s\n", PQerrorMessage(conn));
        if (res) PQclear(res);
        return;
    }
    
    free_function_metadata_array(array);
    init_function_metadata_array(array);
    
    int rows = PQntuples(res);
    printf("Loading %d functions from schema '%s'\n", rows, schema);
    
    for (int i = 0; i < rows; i++) {
        FunctionMetadata func = {0};
        
        func.schema = strdup(PQgetvalue(res, i, 0));
        func.name = strdup(PQgetvalue(res, i, 1));
        func.return_type = strdup(PQgetvalue(res, i, 2));
        func.language = strdup(PQgetvalue(res, i, 3));
        
        const char *desc = PQgetvalue(res, i, 4);
        func.description = desc ? strdup(desc) : strdup("");
        
        // Parse argument list
        const char *arg_list = PQgetvalue(res, i, 5);
        
        if (arg_list && strlen(arg_list) > 0) {
            char *arg_copy = strdup(arg_list);
            int arg_count = 1;
            for (char *p = arg_copy; *p; p++) {
                if (*p == ',') arg_count++;
            }
            
            func.arg_count = arg_count;
            func.arguments = malloc(arg_count * sizeof(FunctionArgument));
            
            // Parse each argument: "name type" or just "type"
            char *saveptr;
            char *token = strtok_r(arg_copy, ",", &saveptr);
            int idx = 0;
            
            while (token && idx < arg_count) {
                while (*token == ' ') token++;
                char *last_space = strrchr(token, ' ');
                if (last_space) {
                    *last_space = '\0';
                    func.arguments[idx].name = strdup(token);
                    func.arguments[idx].type = strdup(last_space + 1);
                } else {
                    func.arguments[idx].name = strdup("");
                    func.arguments[idx].type = strdup(token);
                }
                func.arguments[idx].mode = strdup("IN");
                func.arguments[idx].position = idx;
                
                // Check if this argument type is an enum and load values
                const char *type_name = func.arguments[idx].type;
                if (is_enum_type(conn, type_name)) {
                    func.arguments[idx].is_enum = 1;
                    func.arguments[idx].enum_values = get_enum_values(conn, type_name, 
                                                                      &func.arguments[idx].enum_count);
                    printf("  Argument %s is ENUM type '%s' with %d values\n", 
                           func.arguments[idx].name, type_name, func.arguments[idx].enum_count);
                } else {
                    func.arguments[idx].is_enum = 0;
                    func.arguments[idx].enum_values = NULL;
                    func.arguments[idx].enum_count = 0;
                }
                
                token = strtok_r(NULL, ",", &saveptr);
                idx++;
            }
            free(arg_copy);
        } else {
            func.arg_count = 0;
            func.arguments = NULL;
        }
        
        if (array->count >= array->capacity) {
            array->capacity *= 2;
            array->functions = realloc(array->functions, 
                                       array->capacity * sizeof(FunctionMetadata));
        }
        array->functions[array->count++] = func;
    }
    
    PQclear(res);
    printf("Loaded %d functions\n", array->count);
}

FunctionMetadata* get_function_by_name(FunctionMetadataArray *array, const char *name) {
    for (int i = 0; i < array->count; i++) {
        if (strcmp(array->functions[i].name, name) == 0) {
            return &array->functions[i];
        }
    }
    return NULL;
}