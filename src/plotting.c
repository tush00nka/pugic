#include "tabs.h"
#include <math.h>
#include <raylib.h>

// Global font for plots (set this once)
static Font plot_font = {0};

// Initialize plot font (call this once in main)
void InitPlotFont(Font font) {
    plot_font = font;
}

// Helper function to begin drawing to a render texture
RenderTexture2D BeginPlotTexture(int width, int height) {
  RenderTexture2D target = LoadRenderTexture(width, height);
  BeginTextureMode(target);
  ClearBackground(RAYWHITE);
  return target;
}

// Helper function to end texture drawing
void EndPlotTexture(RenderTexture2D target) {
  EndTextureMode();
  Image img = LoadImageFromTexture(target.texture);
  ImageFlipVertical(&img);
  UpdateTexture(target.texture, img.data);
  UnloadImage(img);
}

// Custom text drawing with the plot font
void DrawPlotText(const char *text, float x, float y, float font_size, Color color) {
    if (plot_font.texture.id != 0) {
        DrawTextEx(plot_font, text, (Vector2){x, y}, font_size*2, 0, color);
    } else {
        DrawText(text, (int)x, (int)y, (int)font_size, color); // fallback
    }
}

// Draw line plot to a render texture
RenderTexture2D DrawLinePlotToTexture(float *x_data, float *y_data,
                                      int data_count, float x_min, float x_max,
                                      float y_min, float y_max,
                                      int texture_width, int texture_height,
                                      Color line_color) {
  RenderTexture2D target = BeginPlotTexture(texture_width, texture_height);

  // Define plot area within the texture (with margins)
  Rectangle plot_area = {
      50,                  // left margin
      50,                  // top margin
      texture_width - 100, // width minus margins
      texture_height - 100 // height minus margins
  };

  // Calculate scaling factors
  float x_scale = plot_area.width / (x_max - x_min);
  float y_scale = plot_area.height / (y_max - y_min);

  // Draw axes
  DrawLine(plot_area.x, plot_area.y + plot_area.height,
           plot_area.x + plot_area.width, plot_area.y + plot_area.height,
           DARKGRAY);
  DrawLine(plot_area.x, plot_area.y, plot_area.x,
           plot_area.y + plot_area.height, DARKGRAY);

  // Draw axis labels
  char label[32];
  snprintf(label, sizeof(label), "%.1f", x_min);
  DrawPlotText(label, plot_area.x - 30, plot_area.y + plot_area.height - 5, 10, DARKGRAY);
  
  snprintf(label, sizeof(label), "%.1f", x_max);
  DrawPlotText(label, plot_area.x + plot_area.width + 5, plot_area.y + plot_area.height - 5, 10, DARKGRAY);
  
  snprintf(label, sizeof(label), "%.1f", y_min);
  DrawPlotText(label, plot_area.x - 25, plot_area.y + plot_area.height - 5, 10, DARKGRAY);
  
  snprintf(label, sizeof(label), "%.1f", y_max);
  DrawPlotText(label, plot_area.x - 25, plot_area.y - 5, 10, DARKGRAY);

  // Draw the line connecting points
  for (int i = 0; i < data_count - 1; i++) {
    Vector2 p1 = {plot_area.x + ((x_data[i] - x_min) * x_scale),
                  plot_area.y + plot_area.height -
                      ((y_data[i] - y_min) * y_scale)};
    Vector2 p2 = {plot_area.x + ((x_data[i + 1] - x_min) * x_scale),
                  plot_area.y + plot_area.height -
                      ((y_data[i + 1] - y_min) * y_scale)};
    DrawLineEx(p1, p2, 2.0f, line_color);
  }

  EndPlotTexture(target);
  return target;
}

// Draw histogram to a render texture
RenderTexture2D DrawHistogramToTexture(float *bins, int bin_count,
                                       float max_frequency, int texture_width,
                                       int texture_height, Color bar_color) {
  RenderTexture2D target = BeginPlotTexture(texture_width, texture_height);

  // Define plot area within the texture
  Rectangle plot_area = {50, 50, texture_width - 100, texture_height - 100};

  float bar_width = plot_area.width / bin_count;
  float y_scale = plot_area.height / max_frequency;

  for (int i = 0; i < bin_count; i++) {
    float bar_height = bins[i] * y_scale;
    Rectangle bar = {plot_area.x + (i * bar_width),
                     plot_area.y + plot_area.height - bar_height,
                     (bar_width > 2.0f) ? bar_width - 2.0f : bar_width,
                     bar_height};
    DrawRectangleRec(bar, bar_color);

    // Draw value on top of bar
    if (bar_height > 15) {
      char value[32];
      snprintf(value, sizeof(value), "%.0f", bins[i]);
      float text_x = bar.x + (bar_width / 2) - (MeasureTextEx(plot_font, value, 10, 0).x / 2);
      DrawPlotText(value, text_x, bar.y - 15, 10, DARKGRAY);
    }
  }

  // Draw axes
  DrawLine(plot_area.x, plot_area.y + plot_area.height,
           plot_area.x + plot_area.width, plot_area.y + plot_area.height,
           DARKGRAY);
  DrawLine(plot_area.x, plot_area.y, plot_area.x,
           plot_area.y + plot_area.height, DARKGRAY);

  EndPlotTexture(target);
  return target;
}

// Draw pie chart to a render texture
RenderTexture2D DrawPieChartToTexture(float *values, Color *colors,
                                      int value_count, int texture_width,
                                      int texture_height) {
  RenderTexture2D target = BeginPlotTexture(texture_width, texture_height);

  Vector2 center = {texture_width / 2.0f, texture_height / 2.0f};
  float radius = (texture_width < texture_height ? texture_width : texture_height) * 0.35f;

  // Calculate total for percentages
  float total = 0;
  for (int i = 0; i < value_count; i++) {
    total += values[i];
  }

  // Draw each slice
  float start_angle = 0;
  for (int i = 0; i < value_count; i++) {
    float slice_angle = (values[i] / total) * 360.0f;
    DrawCircleSector(center, radius, start_angle, start_angle + slice_angle, 36,
                     colors[i]);

    // Calculate position for label
    float mid_angle = start_angle + (slice_angle / 2.0f);
    float label_radius = radius * 0.7f;
    Vector2 label_pos = {center.x + cos(mid_angle * DEG2RAD) * label_radius,
                         center.y + sin(mid_angle * DEG2RAD) * label_radius};

    // Draw percentage label
    float percentage = (values[i] / total) * 100.0f;
    char percentage_text[32];
    snprintf(percentage_text, sizeof(percentage_text), "%.0f%%", percentage);
    
    Vector2 text_size = MeasureTextEx(plot_font, percentage_text, 12, 0);
    DrawPlotText(percentage_text, label_pos.x - text_size.x / 2,
                 label_pos.y - 5, 12, WHITE);

    start_angle += slice_angle;
  }

  // Draw outline
  DrawCircleLines(center.x, center.y, radius, DARKGRAY);

  EndPlotTexture(target);
  return target;
}

// Helper function to convert RenderTexture2D to Texture2D
Texture2D GetTextureFromRenderTexture(RenderTexture2D renderTex) {
  return renderTex.texture;
}

// Helper function to unload plot textures
void UnloadPlotTexture(RenderTexture2D texture) {
  UnloadRenderTexture(texture);
}

/**
 * Convert raylib Texture to Nuklear image
 */
struct nk_image TextureToNuklear(Texture tex) {
  Texture *stored_tex = (Texture *)MemAlloc(sizeof(Texture));

  stored_tex->id = tex.id;
  stored_tex->width = tex.width;
  stored_tex->height = tex.height;
  stored_tex->mipmaps = tex.mipmaps;
  stored_tex->format = tex.format;

  struct nk_image img;
  img.handle.ptr = stored_tex;
  img.w = (nk_ushort)stored_tex->width;
  img.h = (nk_ushort)stored_tex->height;
  img.region[0] = 0;
  img.region[1] = 0;
  img.region[2] = img.w;
  img.region[3] = img.h;

  return img;
}

// Convert RenderTexture2D to Nuklear image
struct nk_image RenderTextureToNuklear(RenderTexture2D renderTex) {
  return TextureToNuklear(renderTex.texture);
}