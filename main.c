#include <stdio.h>
#include <signal.h>
#include <libpq-fe.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/io.h>

#include "config.h"
#include "calibrate.h"
#include "db.h"
#include "object.h"
#include "servo.h"

int main(int argc, char *argv[]) {
	PGconn	*db = NULL;
		
	long	start = -1;
	long	end = -1;
	char 	*start_t;
	char	*end_t;

	char	sql[10240];

	double	speed_rate = -1;

	long	time_l;
	int	object_i, r;

	fprintf(stderr, "SERVO: OPENING SERVO NETWORK...\n");

	r = servo_open();

	if(r <= 0)
		return 1;
	else 
		fprintf(stderr,"SERVO: FOUND %d SERVO CONTROLLER(S) ON NETWORK\n", r);

	// this is guaranteed to succeed
	db = db_connect();

	calibrate();

	while(1) {
		sprintf(sql, "select extract('epoch' from created) as d from mh_object,mh_layer,mh_place where mh_layer.place_id = mh_place.id and mh_layer.object_id = mh_object.id and mh_layer.group_id = 5 and the_geom && GeometryFromText('POLYGON((%f %f, %f %f, %f %f, %f %f, %f %f))',4326) order by created asc limit 1", EXTENT_LEFT, EXTENT_TOP, EXTENT_RIGHT, EXTENT_TOP, EXTENT_LEFT, EXTENT_BOTTOM, EXTENT_RIGHT, EXTENT_BOTTOM, EXTENT_LEFT, EXTENT_TOP);

		start_t = db_query_scalar(db, sql);

		if(start_t != NULL) 
			start = atol(start_t);
		else {
			fprintf(stderr, "ERROR: ERROR FETCHING START RANGE DATE.\n");
			continue;
		}

		sprintf(sql, "select extract('epoch' from created) as d from mh_object,mh_layer,mh_place where mh_layer.place_id = mh_place.id and mh_layer.object_id = mh_object.id and mh_layer.group_id = 5 and the_geom && GeometryFromText('POLYGON((%f %f, %f %f, %f %f, %f %f, %f %f))',4326) order by created desc limit 1", EXTENT_LEFT, EXTENT_TOP, EXTENT_RIGHT, EXTENT_TOP, EXTENT_LEFT, EXTENT_BOTTOM, EXTENT_RIGHT, EXTENT_BOTTOM, EXTENT_LEFT, EXTENT_TOP);

		end_t = db_query_scalar(db, sql);

		if(end_t != NULL)
			end = atol(end_t);
		else {	
			fprintf(stderr, "ERROR: ERROR FETCHING END RANGE DATE.\n");
			continue;
		}

		free(start_t);
		free(end_t);

		speed_rate = abs(start - end) / (PLAYBACK_L_HR * 60 * 60);

		fprintf(stderr, "NOTICE: SOUND START DATE IS %s", ctime(&start));
		fprintf(stderr, "NOTICE: SOUND END DATE IS %s", ctime(&end));
		fprintf(stderr, "NOTICE: THERE ARE %.2f RECORDED SECONDS FOR EVERY WALL CLOCK SECOND.\n", speed_rate);
		fprintf(stderr, "NOTICE: LOOP TIME IS %.2f WALL CLOCK SECONDS.\n", (end - start) / speed_rate);

		object_i = 0;
		time_l = start;
		while(1) {
			PGresult        *q = NULL;

			sprintf(sql, "select mh_object.id, extract('epoch' from created) as d, name from mh_object,mh_layer,mh_place where mh_layer.place_id = mh_place.id and mh_layer.object_id = mh_object.id and mh_layer.group_id = 5 and the_geom && GeometryFromText('POLYGON((%f %f, %f %f, %f %f, %f %f, %f %f))',4326) ORDER BY created ASC OFFSET %d LIMIT 1", EXTENT_LEFT, EXTENT_TOP, EXTENT_RIGHT, EXTENT_TOP, EXTENT_LEFT, EXTENT_BOTTOM, EXTENT_RIGHT, EXTENT_BOTTOM, EXTENT_LEFT, EXTENT_TOP, object_i);

        		q = PQexec(db, sql);

			if(PQntuples(q) == 1) {
				int 	id = atoi(PQgetvalue(q, 0, 0));
				long 	trigger_l = atol(PQgetvalue(q, 0, 1));
				char 	*name = PQgetvalue(q, 0, 2);
				long 	sleep_l = (trigger_l - time_l) / speed_rate;
		
				fprintf(stderr, "NOTICE: OBJECT %d (I=%d): NAME='%s' IS NEXT IN QUEUE.\n", id, object_i, name);
				fprintf(stderr, "NOTICE: OBJECT %d: SLEEPING FOR %ld SECONDS UNTIL RELEASE...\n", id, sleep_l);

				sleep(sleep_l);

				fprintf(stderr, "NOTICE: OBJECT %d: DONE SLEEPING.\n", id);
			
				object_launch(id, db);

				object_i = object_i + 1;
				time_l = trigger_l;

		     		PQclear(q);
				continue;
			} 

	     		PQclear(q);

			fprintf(stderr,"NO MORE OBJECTS TO DISPLAY; STARTING NEW LOOP...\n");
			break;
		}
	}

// we never get here!
	fprintf(stderr, "NOTICE: PUTTING SERVOS AT HOME POSITION...\n");

	servo_move(LEFT_SERVO, SERVO_HOME_IN, SERVO_MAX_VEL_SLEW);
	servo_move(RIGHT_SERVO, SERVO_HOME_IN, SERVO_MAX_VEL_SLEW);

   	while(servo_moving(LEFT_SERVO) || servo_moving(RIGHT_SERVO))
                usleep(500);

	fprintf(stderr, "NOTICE: CLEANING UP...\n");

	servo_close();
	db_close(db);

	fprintf(stderr, "NOTICE: STOP\n");
	return 0;
}

