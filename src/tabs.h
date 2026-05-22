#ifndef TABS_H_
#define TABS_H_

#include "function.h"
#include "nuklear.h"
#include <libpq-fe.h>

typedef enum {
  TAB_QUERY = 0,
  TAB_FUNCTIONS,
  TAB_ANALYTICS,
  TAB_SETTINGS
} TabType;

typedef struct connection_settings {
  char host[32];
  char username[32];
  char password[32];
  char dbname[32];
  char conninfo[256];
} ConnectionSettings;

void draw_tab(struct nk_context *ctx, const char *label, TabType tab_id,
              TabType *current_tab, int is_last);

void draw_tab_query(struct nk_context *ctx, PGconn *conn, PGresult **res,
                    int *totalRows, int *totalCols, char (*buf)[2048],
                    struct nk_scroll *scroll,
                    nk_bool *manual_mode,
                    struct connection_settings *connsettings, char *errorMessage);

void draw_tab_functions(struct nk_context *ctx, PGconn *conn, PGresult **res,
                        int *totalRows, int *totalCols, char (*buf)[2048],
                        char ***names, int *last_function_count,
                        FunctionMetadataArray *functions,
                        int *selected_function, char ****argument_values,
                        TabType *current_tab, char *errorMessage);

void draw_tab_analytics(struct nk_context *ctx, PGconn *conn);

void draw_tab_settings(PGconn *conn, struct nk_context *ctx,
                       struct connection_settings *connsettings,
                       char *errorMessage);

#endif // TABS_H_