#include "plotting_data.h"

ChartData get_line_plot_data(PGconn *conn) {
  ChartData data = {0};
  PGresult *res = PQexec(
      conn,
      "SELECT TO_CHAR(sale_date, 'Mon') as month, COUNT(*) as tickets_sold "
      "FROM ticket_sales "
      "WHERE sale_date >= NOW() - INTERVAL '6 months' "
      "  AND order_status = 'paid' "
      "GROUP BY EXTRACT(MONTH FROM sale_date), TO_CHAR(sale_date, 'Mon') "
      "ORDER BY EXTRACT(MONTH FROM sale_date)");

  if (PQresultStatus(res) == PGRES_TUPLES_OK) {
    data.count = PQntuples(res);
    data.values = (float *)malloc(data.count * sizeof(float));
    data.labels = (char **)malloc(data.count * sizeof(char *));

    for (int i = 0; i < data.count; i++) {
      data.values[i] = atof(PQgetvalue(res, i, 1));
      data.labels[i] = strdup(PQgetvalue(res, i, 0));
    }
  }

  PQclear(res);
  return data;
}

ChartData get_histogram_data(PGconn *conn) {
  ChartData data = {0};
  PGresult *res = PQexec(conn, 
      "SELECT "
      "  CASE "
      "    WHEN cost < 1000 THEN '0-999' "
      "    WHEN cost BETWEEN 1000 AND 2999 THEN '1000-2999' "
      "    WHEN cost BETWEEN 3000 AND 4999 THEN '3000-4999' "
      "    WHEN cost BETWEEN 5000 AND 9999 THEN '5000-9999' "
      "    ELSE '10000+' "
      "  END as price_range, "
      "  COUNT(*) as count "
      "FROM ticket_sales "
      "WHERE order_status = 'paid' "
      "GROUP BY price_range "
      "ORDER BY MIN(cost)");

  if (PQresultStatus(res) == PGRES_TUPLES_OK) {
    data.count = PQntuples(res);
    data.values = (float *)malloc(data.count * sizeof(float));
    data.labels = (char **)malloc(data.count * sizeof(char *));

    for (int i = 0; i < data.count; i++) {
      data.values[i] = atof(PQgetvalue(res, i, 1));
      data.labels[i] = strdup(PQgetvalue(res, i, 0));
    }
  }

  PQclear(res);
  return data;
}

ChartData get_pie_chart_data(PGconn *conn) {
  ChartData data = {0};

  PGresult *res = PQexec(conn, 
      "SELECT b.band_name, COUNT(ts.sale_id) as tickets_sold "
      "FROM band b "
      "JOIN concert c ON b.band_id = c.band_id "
      "JOIN ticket_sales ts ON c.concert_id = ts.concert_id "
      "WHERE ts.order_status = 'paid' "
      "GROUP BY b.band_name "
      "ORDER BY tickets_sold DESC "
      "LIMIT 4");

  if (PQresultStatus(res) == PGRES_TUPLES_OK) {
    data.count = PQntuples(res);
    data.values = (float *)malloc(data.count * sizeof(float));
    data.labels = (char **)malloc(data.count * sizeof(char *));

    for (int i = 0; i < data.count; i++) {
      data.values[i] = atof(PQgetvalue(res, i, 1));
      data.labels[i] = strdup(PQgetvalue(res, i, 0));
    }
  }

  PQclear(res);
  return data;
}

void free_chart_data(ChartData *data) {
  if (data->labels) {
    for (int i = 0; i < data->count; i++) {
      free(data->labels[i]);
    }
    free(data->labels);
  }
  free(data->values);
  data->count = 0;
}
