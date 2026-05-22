#ifndef SQL_H_
#define SQL_H_

#include <libpq-fe.h>

void exit_gracefully(PGconn *conn);
void execPrintToList(PGconn *conn, PGresult **res, char *query, int *totalRows,
                       int *totalCols, char *errorMessage);

#endif // SQL_H_