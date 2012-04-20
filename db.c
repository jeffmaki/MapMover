#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <libpq-fe.h>

#include "config.h"

void db_ensureconnected(PGconn *db) {
	while(PQstatus(db) != CONNECTION_OK) {
		fprintf(stderr, "ERROR: DATABASE COULD NOT BE CONTACTED, BUT WILL RETRY; ERROR WAS %s", PQerrorMessage(db));
		PQreset(db);

		sleep(5);
	}
}

char *db_query_scalar(PGconn *db, char *query) {
	PGresult 	*q = NULL;
	char		*r = NULL;

	db_ensureconnected(db);

	q = PQexec(db, query);

	if(PQntuples(q) <= 0) {
		PQclear(q);
		return NULL;
	}

	if(PQresultStatus(q) == PGRES_TUPLES_OK)
		r = (char *)strdup(PQgetvalue(q, 0, 0));
	else
		fprintf(stderr,"ERROR: DB REPORTED: %s; %s\n", PQresStatus(PQresultStatus(q)), PQresultErrorMessage(q));

	PQclear(q);
	return r;
}

PGconn *db_connect() {
	PGconn 		*db = PQconnectdb(DB_CONNECTION_STRING);

	db_ensureconnected(db);

	return db;
}

void db_close(PGconn *db) {
	PQfinish(db);
}
