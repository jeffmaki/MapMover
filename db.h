char 	*db_query_scalar(PGconn *db, char *query);
PGconn 	*db_connect();
void 	db_close(PGconn *db);
void	db_ensureconnected(PGconn *db);
