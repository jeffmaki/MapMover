#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <libpq-fe.h>
#include <libpq/libpq-fs.h>
#include <string.h>
#include <math.h>
#include <sys/io.h>

#include "config.h"
#include "db.h"
#include "servo.h"
#include "sound.h"

static float old_x = WORKSPACE_WIDTH_IN / 2;
static float old_y = (WORKSPACE_HEIGHT_IN / 2) - .5;

static void move(float target_x, float target_y) {
	float left_l = H(target_x, WORKSPACE_HEIGHT_IN - target_y);
	float right_l = H(WORKSPACE_WIDTH_IN - target_x, WORKSPACE_HEIGHT_IN - target_y);

        long  ticks_r = INCHES_TO_TICKS(right_l, INCHES_PER_REV_RIGHT);
        long  ticks_l = INCHES_TO_TICKS(left_l, INCHES_PER_REV_LEFT);

 	servo_move(LEFT_SERVO, ticks_l, SERVO_MAX_VEL_PULSE);
	servo_move(RIGHT_SERVO, ticks_r, SERVO_MAX_VEL_PULSE);
}

void object_launch(int object, PGconn *db) {
	FILE 		*cache;
	pthread_t	thread;
	char		sql[1024];
	char		filename[1024];

	char 		*x_t, *y_t;
	float 		x_coord, y_coord;
	
	fprintf(stderr,"NOTICE: OBJECT %d: MOVING SERVOS...\n", object);

	sprintf(sql, "SELECT X(mh_place.the_geom) FROM mh_object, mh_place, mh_layer WHERE mh_object.id = mh_layer.object_id AND mh_place.id = mh_layer.place_id AND mh_object.id=%d LIMIT 1\n", object);

	x_t = db_query_scalar(db, sql);

	if(x_t != NULL)
               	x_coord = atof(x_t);
        else {
               	fprintf(stderr, "ERROR: error fetching longitude value for object %d", object);
               	return;            
        }                

	sprintf(sql, "SELECT Y(mh_place.the_geom) FROM mh_object, mh_place, mh_layer WHERE mh_object.id = mh_layer.object_id AND mh_place.id = mh_layer.place_id AND mh_object.id=%d LIMIT 1\n", object);

	y_t = db_query_scalar(db, sql);

	if(y_t != NULL)
               	y_coord = atof(y_t);
        else {
               	fprintf(stderr, "ERROR: error fetching latitude value for object %d", object);
               	return;            
        }                

	free(x_t);
	free(y_t);

	float target_x = MAP_OFFSET_LEFT + ((fabs(fabs(x_coord) - fabs(EXTENT_LEFT)) / fabs(EXTENT_LEFT - EXTENT_RIGHT)) * (float)MAP_WIDTH_IN);
	float target_y = MAP_OFFSET_BOTTOM + ((fabs(fabs(y_coord) - fabs(EXTENT_BOTTOM)) / fabs(EXTENT_TOP - EXTENT_BOTTOM)) * (float)MAP_HEIGHT_IN);

	int chunks = sqrt(pow(fabs(old_x - target_x), 2) + pow(fabs(old_y - target_y), 2)) / .125;

	if(chunks > 0) {
		float delta_x = (target_x - old_x) / chunks;
		float delta_y = (target_y - old_y) / chunks;
		int i;

		for(i = 1; i <= chunks; i++)
			move(old_x + delta_x * i, old_y + delta_y * i);

		old_x = target_x;
		old_y = target_y;
	}

	sprintf(filename, "/tmp/MAP-BLOB-CACHE-ID-%d", object);

	cache = fopen(filename, "r");

	if(cache != NULL)
		fclose(cache);
	else {		
		db_ensureconnected(db);
		PQclear(PQexec(db, "BEGIN"));

		sprintf(sql, "SELECT data FROM mh_media WHERE table_flag LIKE '%%mh_object%%' AND table_id=%d", object);

		char	*oid_t = db_query_scalar(db, sql);

		if(oid_t == NULL) {
			fprintf(stderr,"ERROR: ERROR FETCHING BLOB OID FOR OBJECT %d\n", object);

			PQclear(PQexec(db, "ABORT"));
			free(oid_t);

			return;
		}

		int	oid = atoi(oid_t);
	
		free(oid_t);

		if(oid <= 0) {
			fprintf(stderr,"ERROR: ERROR OPENING LOB FOR OBJECT %d\n", object);

			PQclear(PQexec(db, "ABORT"));
			return;
		}

		lo_open(db, oid, INV_READ);
		lo_export(db, oid, filename);
		lo_close(db, oid);

		PQclear(PQexec(db, "ABORT"));
	}

	fprintf(stderr,"NOTICE: PLAYING SOUND FOR OBJECT %d...\n", object);

	if(pthread_create(&thread, NULL, (void *)play_file, (void *)strdup(filename))) {
		fprintf(stderr,"ERROR: ERROR PLAYING SOUND FOR OBJECT %d\n", object);
	      	return;
	}

	pthread_detach(thread);

	fprintf(stderr,"NOTICE: DONE PLAYING SOUND FOR OBJECT %d\n", object);

	return;
}
