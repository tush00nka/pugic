#include "tabs.h"
#include "function.h"
#include "nuklear.h"
#include "plotting.h"
#include "plotting_data.h"
#include "sql.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void build_function_call(FunctionMetadata *func, char **arg_values,
                         int arg_count, char *out_query, size_t out_size) {
  char *ptr = out_query;
  ptr += snprintf(ptr, out_size - (ptr - out_query), "SELECT * FROM %s.%s(",
                  func->schema, func->name);

  for (int i = 0; i < arg_count; i++) {
    if (i > 0) {
      ptr += snprintf(ptr, out_size - (ptr - out_query), ", ");
    }

    const char *type = func->arguments[i].type;
    const char *value = arg_values[i];

    // empty -> NULL
    if (value == NULL || value[0] == '\0') {
      ptr += snprintf(ptr, out_size - (ptr - out_query), "NULL");
      continue;
    }

    // bool
    if (strstr(type, "bool") != NULL) {
      int is_true = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
      ptr += snprintf(ptr, out_size - (ptr - out_query), "%s",
                      is_true ? "true" : "false");
    }
    // int, float, etc
    else if (strstr(type, "int") != NULL || strstr(type, "numeric") != NULL ||
             strstr(type, "float") != NULL || strstr(type, "decimal") != NULL) {
      ptr += snprintf(ptr, out_size - (ptr - out_query), "%s", value);
    }
    // everything else treat as string
    else {
      char escaped[1024];
      const char *src = value;
      char *dst = escaped;
      while (*src && (dst - escaped) < (int)sizeof(escaped) - 2) {
        if (*src == '\'') {
          *dst++ = '\'';
          *dst++ = '\'';
        } else {
          *dst++ = *src;
        }
        src++;
      }
      *dst = '\0';
      ptr += snprintf(ptr, out_size - (ptr - out_query), "'%s'", escaped);
    }
  }

  snprintf(ptr, out_size - (ptr - out_query), ");");
}

void draw_tab(struct nk_context *ctx, const char *label, TabType tab_id,
              TabType *current_tab, int is_last) {

  struct nk_style_button old_style = ctx->style.button;

  if (*current_tab == tab_id) {
    ctx->style.button.normal = nk_style_item_color(nk_rgb(70, 130, 200));
    ctx->style.button.hover = nk_style_item_color(nk_rgb(90, 150, 220));
    ctx->style.button.active = nk_style_item_color(nk_rgb(60, 120, 190));
    ctx->style.button.border_color = nk_rgb(100, 160, 230);
  } else {
    ctx->style.button.normal = nk_style_item_color(nk_rgb(60, 60, 70));
    ctx->style.button.hover = nk_style_item_color(nk_rgb(80, 80, 90));
    ctx->style.button.active = nk_style_item_color(nk_rgb(50, 50, 60));
    ctx->style.button.border_color = nk_rgb(80, 80, 90);
  }

  ctx->style.button.rounding = 0;

  if (nk_button_label(ctx, label)) {
    *current_tab = tab_id;
  }

  ctx->style.button = old_style;
}

const static char *table_names[8] = {"album",        "band",    "client",
                                     "concert_hall", "concert", "sector",
                                     "ticket_sales", "song"};
static int selected_table = 0;

void draw_tab_query(struct nk_context *ctx, PGconn *conn, PGresult **res,
                    int *totalRows, int *totalCols, char (*buf)[2048],
                    struct nk_scroll *scroll, nk_bool *manual_mode,
                    struct connection_settings *connsettings,
                    char *errorMessage) {
  float height_left = GetScreenHeight() - 100;
  nk_layout_row_dynamic(ctx, 30, 3);
  height_left -= 30;
  char connInfo[512] = "Connected to ";
  strcat(connInfo, connsettings->dbname);
  strcat(connInfo, " as ");
  strcat(connInfo, connsettings->username);
  nk_label(ctx, connInfo, NK_TEXT_LEFT);

  nk_spacer(ctx);

  nk_radio_label(ctx, "SQL Mode", manual_mode);

  if (*manual_mode) {
    nk_layout_row_dynamic(ctx, 30, 1);
    height_left -= 30;
    nk_label(ctx, "Enter query:", NK_TEXT_LEFT);
    nk_layout_row_dynamic(ctx, 250, 1);
    height_left -= 250;
    nk_edit_string_zero_terminated(
        ctx, NK_EDIT_BOX | NK_EDIT_MULTILINE | NK_EDIT_ALLOW_TAB, *buf,
        sizeof(*buf) - 1, nk_filter_default);
  } else {
    nk_layout_row_dynamic(ctx, 30, 1);
    height_left -= 30;
    nk_label(ctx, "Select table:", NK_TEXT_LEFT);
    nk_layout_row_dynamic(ctx, 40, 1);
    height_left -= 40;
    selected_table = nk_combo(ctx, table_names, 8, selected_table, 40,
                              nk_vec2(400.f, 400.f));
  }
  nk_layout_row_dynamic(ctx, 30, 1);
  height_left -= 30;
  if (nk_button_label(ctx, "Execute query")) {
    if (*manual_mode) {
      execPrintToList(conn, res, *buf, totalRows, totalCols, errorMessage);
    } else {
      char query[256] = "";
      sprintf(query, "SELECT * FROM %s", table_names[selected_table]);
      execPrintToList(conn, res, query, totalRows, totalCols, errorMessage);
    }
    *scroll = (struct nk_scroll){0, 0};
    printf("exec ok, rows: %d\n", *totalRows);
  }

  nk_layout_row_dynamic(ctx, height_left, 1);
  if (errorMessage[0] != '\0') {
    nk_text_wrap_colored(ctx, errorMessage, strlen(errorMessage),
                         nk_rgb(255, 0, 0));
    // nk_label_colored(ctx, errorMessage, NK_TEXT_LEFT, nk_rgb(255, 0, 0));
  } else if (res == NULL || totalRows <= 0 || totalCols <= 0) {
    const static char *resp = "No query results";
    nk_text_wrap_colored(ctx, resp, strlen(resp), nk_rgb(255, 0, 0));
  } else {
    if (nk_group_scrolled_begin(ctx, scroll, "TableGroup", 0)) {
      float *col_widths = malloc(*totalCols * sizeof(float));
      float col_w = (float)GetScreenWidth() / (*totalCols + 1);
      for (int i = 0; i < *totalCols; i++) {
        col_widths[i] = col_w;
      }

      nk_layout_row(ctx, NK_STATIC, 30, *totalCols, col_widths);

      for (int i = 0; i < *totalCols; i++) {
        const char *col_name = PQfname(*res, i);
        nk_label_colored(ctx, col_name, NK_TEXT_RIGHT, nk_rgb(200, 200, 255));
      }

      for (int row = 0; row < *totalRows; row++) {
        nk_layout_row(ctx, NK_STATIC, 25, *totalCols, col_widths);

        for (int col = 0; col < *totalCols; col++) {
          if (PQgetisnull(*res, row, col)) {
            nk_label_colored(ctx, "NULL", NK_TEXT_LEFT, nk_rgb(128, 128, 128));
          } else {
            char *value = PQgetvalue(*res, row, col);
            Oid col_type = PQftype(*res, col);

            if (col_type == 20 || col_type == 21 || col_type == 23) { // INTs
              nk_label(ctx, value, NK_TEXT_RIGHT);
            } else if (col_type == 700 || col_type == 701) { // FLOAT
              nk_label(ctx, value, NK_TEXT_RIGHT);
            } else {
              nk_label(ctx, value, NK_TEXT_LEFT);
            }
          }
        }
      }
      free(col_widths);
      nk_group_scrolled_end(ctx);
    }
  }
}

void draw_tab_settings(PGconn *conn, struct nk_context *ctx,
                       struct connection_settings *connsettings,
                       char *errorMessage) {
  nk_layout_row_dynamic(ctx, 30, 1);
  nk_label_colored(ctx, "Connection Settings", NK_TEXT_ALIGN_LEFT,
                   nk_rgb(128, 128, 128));
  nk_layout_row_dynamic(ctx, 40, 2);
  nk_label(ctx, "Host", NK_TEXT_ALIGN_LEFT);
  nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, connsettings->host,
                                 sizeof(connsettings->host) - 1,
                                 nk_filter_default);
  nk_layout_row_dynamic(ctx, 40, 2);
  nk_label(ctx, "User", NK_TEXT_ALIGN_LEFT);
  nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, connsettings->username,
                                 sizeof(connsettings->username) - 1,
                                 nk_filter_default);
  nk_layout_row_dynamic(ctx, 40, 2);
  nk_label(ctx, "Password", NK_TEXT_ALIGN_LEFT);
  nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, connsettings->password,
                                 sizeof(connsettings->password) - 1,
                                 nk_filter_default);
  nk_layout_row_dynamic(ctx, 40, 2);
  nk_label(ctx, "DB name", NK_TEXT_ALIGN_LEFT);
  nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, connsettings->dbname,
                                 sizeof(connsettings->dbname) - 1,
                                 nk_filter_default);

  nk_layout_row_dynamic(ctx, 30, 1);
  nk_spacer(ctx);

  nk_layout_row_dynamic(ctx, 30, 3);

  nk_spacer(ctx);

  if (nk_button_label(ctx, "connect")) {
    sprintf(connsettings->conninfo, "postgresql://%s:%s@%s/%s",
            connsettings->username, connsettings->password, connsettings->host,
            connsettings->dbname);

    conn = PQconnectdb(connsettings->conninfo);
    if (PQstatus(conn) != CONNECTION_OK) {
      sprintf(errorMessage, "Connection failed: %s", PQerrorMessage(conn));
    } else {
      sprintf(errorMessage, "All good");
    }
  }

  nk_spacer(ctx);

  nk_layout_row_dynamic(ctx, 300, 1);
  // nk_text(ctx, errorMessage, strlen(errorMessage), NK_TEXT_ALIGN_LEFT);
  nk_text_wrap(ctx, errorMessage, strlen(errorMessage));
}

void draw_tab_functions(struct nk_context *ctx, PGconn *conn, PGresult **res,
                        int *totalRows, int *totalCols, char (*buf)[2048],
                        char ***names, int *last_function_count,
                        FunctionMetadataArray *functions,
                        int *selected_function, char ****argument_values,
                        TabType *current_tab, char *errorMessage) {

  nk_layout_row_dynamic(ctx, 30, 1);
  nk_label(ctx, "Select a function:", NK_TEXT_LEFT);

  if (!*names || *last_function_count != functions->count) {
    if (*names)
      free(*names);
    *names = malloc(functions->count * sizeof(char *));
    for (int i = 0; i < functions->count; i++) {
      (*names)[i] = functions->functions[i].name;
    }
    *last_function_count = functions->count;
  }

  nk_layout_row_dynamic(ctx, 30, 1);
  *selected_function = nk_combo(ctx, (const char **)*names, functions->count,
                                *selected_function, 25, nk_vec2(600, 400));

  if (*selected_function >= 0 && *selected_function < functions->count) {
    FunctionMetadata *func = &functions->functions[*selected_function];

    if (func->arg_count > 0) {
      nk_layout_row_dynamic(ctx, 10, 1);
      nk_spacer(ctx);
      nk_layout_row_dynamic(ctx, 25, 1);
      nk_label_colored(ctx, "Function Arguments:", NK_TEXT_LEFT,
                       nk_rgb(200, 200, 100));

      // scrollable area for many arguments
      static struct nk_scroll args_scroll = {0, 0};
      nk_layout_row_dynamic(ctx, 200, 1);
      if (nk_group_scrolled_begin(ctx, &args_scroll, "ArgsScroll", 0)) {
        for (int i = 0; i < func->arg_count; i++) {
          FunctionArgument *arg = &func->arguments[i];
          nk_layout_row_dynamic(ctx, 40, 2);

          // argument name and type
          char label[256];
          if (strcmp(arg->type, "date") == 0)
            snprintf(label, sizeof(label), "%s (YYYY-MM-DD):", arg->name);
          else
            snprintf(label, sizeof(label), "%s (%s):", arg->name, arg->type);
          nk_label(ctx, label, NK_TEXT_LEFT);

          // get the correct buffer for this argument
          char *value = (*argument_values)[*selected_function][i];

          if (arg->is_enum) {
            const char **enum_items = (const char **)arg->enum_values;
            int selected = 0;
            for (size_t i = 0; i < arg->enum_count; ++i) {
              if (strcmp(enum_items[i], value) == 0) {
                selected = i;
                break;
              }
            }

            int new_selected = nk_combo(ctx, enum_items, arg->enum_count,
                                        selected, 40, nk_vec2(300, 200));

            if (new_selected != selected) {
              char new_value[32];
              snprintf(new_value, sizeof(new_value), "%s",
                       enum_items[new_selected]);
              strcpy(value, new_value);
            }
          } else if (strstr(arg->type, "int") != NULL ||
                     strstr(arg->type, "numeric") != NULL ||
                     strstr(arg->type, "float") != NULL) {
            nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, value, 255,
                                           nk_filter_decimal);
          } else if (strstr(arg->type, "bool") != NULL) {
            int val = (strcmp(value, "true") == 0);
            if (nk_checkbox_label(ctx, "True", &val)) {
              snprintf(value, 256, val ? "true" : "false");
            }
          } else {
            nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, value, 255,
                                           nk_filter_default);
          }
        }
        nk_group_scrolled_end(ctx);
      }
    } else {
      nk_layout_row_dynamic(ctx, 25, 1);
      nk_label(ctx, "(No arguments required)", NK_TEXT_CENTERED);
    }
  }

  nk_layout_row_dynamic(ctx, 30, 1);
  if (nk_button_label(ctx, "Execute")) {
    if (*selected_function >= 0 && *selected_function < functions->count) {
      FunctionMetadata *func = &functions->functions[*selected_function];

      build_function_call(func, (*argument_values)[*selected_function],
                          func->arg_count, *buf, sizeof(*buf));

      printf("Executing: %s\n", *buf);
      execPrintToList(conn, res, *buf, totalRows, totalCols, errorMessage);

      *current_tab = TAB_QUERY; // switch to query tab to see results
    }
  }
}

void draw_tab_analytics(struct nk_context *ctx, PGconn *conn) {
  static ChartData line_data = {0};
  static ChartData hist_data = {0};
  static ChartData pie_data = {0};
  static RenderTexture2D line_tex = {0};
  static RenderTexture2D hist_tex = {0};
  static RenderTexture2D pie_tex = {0};
  static int loaded = 0;

  // Refresh button
  nk_layout_row_dynamic(ctx, 35, 1);
  if (nk_button_label(ctx, "Load Charts")) {
    // Free old chart data
    free_chart_data(&line_data);
    free_chart_data(&hist_data);
    free_chart_data(&pie_data);

    // Unload old textures before creating new ones
    if (IsRenderTextureValid(line_tex))
      UnloadRenderTexture(line_tex);
    if (IsRenderTextureValid(hist_tex))
      UnloadRenderTexture(hist_tex);
    if (IsRenderTextureValid(pie_tex))
      UnloadRenderTexture(pie_tex);

    // Clear textures
    line_tex = (RenderTexture2D){0};
    hist_tex = (RenderTexture2D){0};
    pie_tex = (RenderTexture2D){0};

    // Load data from database
    line_data = get_line_plot_data(conn);
    hist_data = get_histogram_data(conn);
    pie_data = get_pie_chart_data(conn);

    printf("Loaded: line=%d, hist=%d, pie=%d\n", line_data.count,
           hist_data.count, pie_data.count);

    // Line plot
    if (line_data.count > 0) {
      float *x_vals = (float *)malloc(line_data.count * sizeof(float));
      for (int i = 0; i < line_data.count; i++) {
        x_vals[i] = (float)i;
      }

      float max = 0;
      for (int i = 0; i < line_data.count; i++) {
        if (line_data.values[i] > max)
          max = line_data.values[i];
      }
      if (max == 0)
        max = 1;

      line_tex =
          DrawLinePlotToTexture(x_vals, line_data.values, line_data.count, 0,
                                line_data.count - 1, 0, max, 400, 300, RED);
      free(x_vals);
    }

    // Histogram
    if (hist_data.count > 0) {
      float max = 0;
      for (int i = 0; i < hist_data.count; i++) {
        if (hist_data.values[i] > max)
          max = hist_data.values[i];
      }
      if (max == 0)
        max = 1;

      hist_tex = DrawHistogramToTexture(hist_data.values, hist_data.count, max,
                                        400, 300, SKYBLUE);
    }

    // Pie chart
    if (pie_data.count > 0) {
      Color colors[] = {RED, BLUE, GREEN, ORANGE, PURPLE, YELLOW};
      pie_tex = DrawPieChartToTexture(pie_data.values, colors, pie_data.count,
                                      400, 300);
    }

    loaded = 1;
  }

  if (!loaded) {
    nk_layout_row_dynamic(ctx, 30, 1);
    nk_label(ctx, "Click 'Load Charts' to see data", NK_TEXT_CENTERED);
    return;
  }

  nk_layout_row_dynamic(ctx, 350, 3);

  if (nk_group_begin(ctx, "Monthly Sales",
                     NK_WINDOW_BORDER | NK_WINDOW_TITLE)) {
    if (line_tex.texture.id != 0 && line_data.count > 0) {
      struct nk_image img = RenderTextureToNuklear(line_tex);
      nk_layout_row_dynamic(ctx, 280, 1);
      nk_image(ctx, img);
    } else {
      nk_label(ctx, "No data available", NK_TEXT_CENTERED);
    }
    nk_group_end(ctx);
  }

  if (nk_group_begin(ctx, "Price Distribution",
                     NK_WINDOW_BORDER | NK_WINDOW_TITLE)) {
    if (hist_tex.texture.id != 0 && hist_data.count > 0) {
      struct nk_image img = RenderTextureToNuklear(hist_tex);
      nk_layout_row_dynamic(ctx, 280, 1);
      nk_image(ctx, img);
    } else {
      nk_label(ctx, "No data available", NK_TEXT_CENTERED);
    }
    nk_group_end(ctx);
  }

  if (nk_group_begin(ctx, "Top Bands", NK_WINDOW_BORDER | NK_WINDOW_TITLE)) {
    if (pie_tex.texture.id != 0 && pie_data.count > 0) {
      struct nk_image img = RenderTextureToNuklear(pie_tex);
      nk_layout_row_dynamic(ctx, 280, 1);
      nk_image(ctx, img);
    } else {
      nk_label(ctx, "No data available", NK_TEXT_CENTERED);
    }
    nk_group_end(ctx);
  }
}