#ifndef FUNCTION_H_
#define FUNCTION_H_

#include <libpq-fe.h>

typedef struct {
  char *name;
  char *type;
  char *mode;
  int position;

  // enum support
  int is_enum;
  char **enum_values;
  int enum_count;
} FunctionArgument;

typedef struct {
    char *schema;
    char *name;
    char *return_type;
    FunctionArgument *arguments;
    int arg_count;
    char *language;
    char *description;
} FunctionMetadata;

typedef struct {
  FunctionMetadata *functions;
  int count;
  int capacity;
} FunctionMetadataArray; 

void init_function_metadata_array(FunctionMetadataArray *array);
void free_function_metadata_array(FunctionMetadataArray *array);
void load_function_metadata(PGconn *conn, FunctionMetadataArray *array, const char *schema);
FunctionMetadata* get_function_by_name(FunctionMetadataArray *array, const char *name);

char** get_enum_values(PGconn *conn, const char *type_name, int *count);

#endif // FUNCTION_H_