#include "font.h"
#include "function.h"
#include "plotting.h"
#include "raylib.h"
#include "sql.h"
#include "tabs.h"
#include <libpq-fe.h>
#include <stdio.h>
#include <string.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_LIB
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#include "nuklear.h"

#define NK_RAYLIB_IMPLEMENTATION
#include "nuklear_raylib.h"

TabType currentTab = TAB_SETTINGS;

static float my_font_get_width(nk_handle handle, float height, const char *text,
                               int len) {
  Font *f = (Font *)handle.ptr;
  // copy text to null-terminated buffer
  char tmp[512];
  int l = len < (int)sizeof(tmp) - 1 ? len : (int)sizeof(tmp) - 1;
  memcpy(tmp, text, l);
  tmp[l] = '\0';
  Vector2 m = MeasureTextEx(*f, tmp, height, 0.0f);
  return m.x;
}

int main(void) {
  PGconn *conn;
  PGresult *res = NULL;

  static ConnectionSettings connsettings = {.conninfo = "",
                                            .dbname = "kurs",
                                            .host = "localhost",
                                            .username = "app_dba",
                                            .password = "dba_password"};

  sprintf(connsettings.conninfo, "postgresql://%s:%s@%s/%s",
          connsettings.username, connsettings.password, connsettings.host,
          connsettings.dbname);

  conn = PQconnectdb(connsettings.conninfo);
  if (PQstatus(conn) != CONNECTION_OK) {
    fprintf(stderr, "Connection failed: %s", PQerrorMessage(conn));
    exit_gracefully(conn);
  }

  printf("INFO: Connected to PostgreSQL\n");

  bool textBoxEditMode = false;
  int totalRows = 0;
  int totalCols = 0;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1280, 720, "pugic");
  SetTargetFPS(60);

  Font rayFont = LoadRussianFont("./res/notosansmono.ttf", 32);
  if (rayFont.texture.id == 0)
    rayFont = GetFontDefault();

  InitPlotFont(rayFont);

  struct nk_context *ctx = nk_raylib_init(1280, 720);

  // create nk_user_font wrapping raylib font
  struct nk_user_font nkf;
  nkf.userdata = nk_handle_ptr(&rayFont);
  nkf.height = 32;
  nkf.width = my_font_get_width;
  nk_style_set_font(ctx, &nkf);

  static char buf[2048] = "SELECT * FROM song;";

  struct nk_scroll scroll = {0, 0};
  char errorMessage[1024] = "";

  FunctionMetadataArray functions;
  int selected_function = 0;

  char ***argument_values = NULL; // 2D array: [function_index][arg_index]
  int *argument_counts = NULL;    // Cache of argument counts per function
  char **names = NULL;
  static int last_function_count = 0;

  init_function_metadata_array(&functions);
  load_function_metadata(conn, &functions, "public");

  argument_values = malloc(functions.count * sizeof(char **));
  argument_counts = malloc(functions.count * sizeof(int));

  for (int i = 0; i < functions.count; i++) {
    int arg_count = functions.functions[i].arg_count;
    argument_counts[i] = arg_count;

    if (arg_count > 0) {
      argument_values[i] = malloc(arg_count * sizeof(char *));
      for (int j = 0; j < arg_count; j++) {
        // allocate a new buffer for each argument
        argument_values[i][j] = calloc(256, sizeof(char));

        // set default '\0' or 0
        const char *type = functions.functions[i].arguments[j].type;
        if (strstr(type, "int") || strstr(type, "numeric") ||
            strstr(type, "float")) {
          snprintf(argument_values[i][j], 256, "0");
        } else if (strstr(type, "bool")) {
          snprintf(argument_values[i][j], 256, "false");
        } else {
          argument_values[i][j][0] = '\0'; // empty string
        }
      }
    } else {
      argument_values[i] = NULL;
    }
  }

  nk_bool manual_mode = 0;

  while (!WindowShouldClose()) {
    nk_raylib_input(ctx);

    if (nk_begin(ctx, "pugic",
                 nk_rect(10, 10, GetScreenWidth() - 20, GetScreenHeight() - 20),
                 NK_WINDOW_BORDER)) {

      if (strncmp(errorMessage, "Connection failed",
                  strlen("Connection failed")) == 0) {
        nk_layout_row_dynamic(ctx, 35, 4);
        nk_spacer(ctx);
        nk_spacer(ctx);
        nk_spacer(ctx);
        draw_tab(ctx, "Settings", TAB_SETTINGS, &currentTab, 1);
      } else {
        nk_layout_row_dynamic(ctx, 35, 4);
        draw_tab(ctx, "Query", TAB_QUERY, &currentTab, 0);
        draw_tab(ctx, "Functions", TAB_FUNCTIONS, &currentTab, 0);
        draw_tab(ctx, "Analytics", TAB_ANALYTICS, &currentTab, 0);
        draw_tab(ctx, "Settings", TAB_SETTINGS, &currentTab, 1);
      }

      switch (currentTab) {

      case TAB_QUERY:
        draw_tab_query(ctx, conn, &res, &totalRows, &totalCols, &buf, &scroll,
                       &manual_mode, &connsettings, errorMessage);
        break;
      case TAB_FUNCTIONS:
        draw_tab_functions(ctx, conn, &res, &totalRows, &totalCols, &buf,
                           &names, &last_function_count, &functions,
                           &selected_function, &argument_values, &currentTab,
                           errorMessage);
        break;
      case TAB_ANALYTICS:
        draw_tab_analytics(ctx, conn);
        break;
      case TAB_SETTINGS:
        draw_tab_settings(conn, ctx, &connsettings, errorMessage);
        break;
      }
    }

    nk_end(ctx);

    BeginDrawing();
    ClearBackground((Color){30, 30, 40, 255});
    // Render Nuklear commands
    nk_raylib_render(nk_rgb(30, 30, 40), nk_rgb(200, 200, 200), rayFont);
    EndDrawing();
  }

  nk_raylib_shutdown();
  UnloadFont(rayFont);
  CloseWindow();

  if (argument_values) {
    for (int i = 0; i < functions.count; i++) {
      if (argument_values[i]) {
        for (int j = 0; j < argument_counts[i]; j++) {
          free(argument_values[i][j]);
        }
        free(argument_values[i]);
      }
    }
    free(argument_values);
    free(argument_counts);
  }
  free_function_metadata_array(&functions);
  if (names)
    free(names);
  PQfinish(conn);
  return 0;
}