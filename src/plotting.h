#ifndef PLOTTING_H_
#define PLOTTING_H_

#include <raylib.h>

void InitPlotFont(Font font);

RenderTexture2D DrawLinePlotToTexture(float *x_data, float *y_data,
                                      int data_count, float x_min, float x_max,
                                      float y_min, float y_max,
                                      int texture_width, int texture_height,
                                      Color line_color);

RenderTexture2D DrawHistogramToTexture(float *bins, int bin_count,
                                       float max_frequency, int texture_width,
                                       int texture_height, Color bar_color);

RenderTexture2D DrawPieChartToTexture(float *values, Color *colors,
                                      int value_count, int texture_width,
                                      int texture_height);

struct nk_image TextureToNuklear(Texture tex);
struct nk_image RenderTextureToNuklear(RenderTexture2D renderTex);
void UnloadPlotTexture(RenderTexture2D texture);
Texture2D GetTextureFromRenderTexture(RenderTexture2D renderTex);

#endif // PLOTTING_H_