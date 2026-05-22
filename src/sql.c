#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>

#include "sql.h"

void exit_gracefully(PGconn *conn) {
  PQfinish(conn);
  exit(1);
}

void execPrintToList(PGconn *conn, PGresult **res, char *query, int *totalRows,
                     int *totalCols, char *errorMessage) {

  if (res == NULL) {
    fprintf(stderr, "execPrintToList: res pointer is NULL\n");
    return;
  }

  if (res != NULL && *res != NULL) {
    PQclear(*res);
    *res = NULL;
  }

  int nFields;
  int i, j;
  int rows;

  *res = PQexec(conn, query);

  if (*res == NULL) {
    fprintf(stderr, "PQexec returned NULL (out of memory?)\n");
    return;
  }

  if (PQresultStatus(*res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "Query failed: %s", PQerrorMessage(conn));
    sprintf(errorMessage,  "Query failed: %s", PQerrorMessage(conn));
    PQclear(*res);
    *res = NULL;
    *totalRows = 0;
    *totalCols = 0;
    return;
  }

  errorMessage[0] = '\0';

  *totalRows = PQntuples(*res);
  *totalCols = PQnfields(*res);

  errorMessage = "\0";
}