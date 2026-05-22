#ifndef PLOTTING_DATA_H_
#define PLOTTING_DATA_H_

#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  float *values;
  char **labels;
  int count;
} ChartData;

// 1. График продаж билетов по месяцам (линейный график)
ChartData get_line_plot_data(PGconn *conn);

// 2. Гистограмма распределения стоимости билетов по концертам
ChartData get_histogram_data(PGconn *conn);

// 3. Круговая диаграмма: топ-4 группы по количеству проданных билетов
ChartData get_pie_chart_data(PGconn *conn);

// Освобождение памяти
void free_chart_data(ChartData *data);

#endif // PLOTTING_DATA_H_