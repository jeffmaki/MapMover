#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "servo.h"
#include "sound.h"

static	FILE *param;

static int wait_button(char *prompt) {
	int 		i = 0;
 	pthread_t	thread;

	if(prompt != NULL) {
		// free()'d by play_file...
 		if(pthread_create(&thread, NULL, (void *)play_file, (void *)strdup(prompt))) {
                	fprintf(stderr,"ERROR: ERROR PLAYING SOUND FOR CALIBRATION EVENT: %s\n", prompt);
	                return 0;
        	}

		pthread_detach(thread);
	}

	while(servo_limit1(RIGHT_SERVO)) {
		usleep(500);

		if(i > 500)
			return 0;

		i++;
	}

	while(! servo_limit1(RIGHT_SERVO))
		usleep(500);

	return 1;
}

void manual_calibration() {
	fprintf(stderr,"ACTION: MOVE PUCK TO UPPER LEFT CORNER. PRESS CALIBRATE KEY WHEN DONE.\n");
	while(! wait_button("/exhibit/sounds/upper_left.wav"));

	float left_top_x = X(SERVO_HOME_IN + TICKS_TO_INCHES(servo_position(LEFT_SERVO), INCHES_PER_REV_LEFT), SERVO_HOME_IN + TICKS_TO_INCHES(servo_position(RIGHT_SERVO), INCHES_PER_REV_RIGHT));
	float left_top_y = Y(SERVO_HOME_IN + TICKS_TO_INCHES(servo_position(LEFT_SERVO), INCHES_PER_REV_LEFT), SERVO_HOME_IN + TICKS_TO_INCHES(servo_position(RIGHT_SERVO), INCHES_PER_REV_RIGHT));

	fprintf(stderr,"ACTION: MOVE PUCK TO LOWER LEFT CORNER. PRESS CALIBRATE KEY WHEN DONE.\n");
	while(! wait_button("/exhibit/sounds/lower_left.wav"));

//	float left_bottom_x = X(SERVO_HOME_IN + TICKS_TO_INCHES(servo_position(LEFT_SERVO), INCHES_PER_REV_LEFT), SERVO_HOME_IN + TICKS_TO_INCHES(servo_position(RIGHT_SERVO), INCHES_PER_REV_RIGHT));
	float left_bottom_y = Y(SERVO_HOME_IN + TICKS_TO_INCHES(servo_position(LEFT_SERVO), INCHES_PER_REV_LEFT), SERVO_HOME_IN + TICKS_TO_INCHES(servo_position(RIGHT_SERVO), INCHES_PER_REV_RIGHT));

	fprintf(stderr,"ACTION: MOVE PUCK TO UPPER RIGHT CORNER. PRESS CALIBRATE KEY WHEN DONE.\n");
	while(! wait_button("/exhibit/sounds/upper_right.wav"));

	float right_top_x = X(SERVO_HOME_IN + TICKS_TO_INCHES(servo_position(LEFT_SERVO), INCHES_PER_REV_LEFT), SERVO_HOME_IN + TICKS_TO_INCHES(servo_position(RIGHT_SERVO), INCHES_PER_REV_RIGHT));
//	float right_top_y = Y(SERVO_HOME_IN + TICKS_TO_INCHES(servo_position(LEFT_SERVO), INCHES_PER_REV_LEFT), SERVO_HOME_IN + TICKS_TO_INCHES(servo_position(RIGHT_SERVO), INCHES_PER_REV_RIGHT));

	MAP_WIDTH_IN 		= right_top_x - left_top_x;
	MAP_HEIGHT_IN 		= left_top_y - left_bottom_y;
	MAP_OFFSET_TOP 		= WORKSPACE_HEIGHT_IN - left_top_y;
	MAP_OFFSET_BOTTOM	= left_bottom_y;
	MAP_OFFSET_LEFT 	= left_top_x;
	MAP_OFFSET_RIGHT	= WORKSPACE_WIDTH_IN - right_top_x; 

	fprintf(stderr,"NOTICE: SAVING MAP PARAMETERS TO DISK...");
	
	param = fopen("calibrate.dat", "w+");

	if(param == NULL)
		fprintf(stderr, "FAILED.\n");
	else {
		fprintf(param, "%f\n", MAP_WIDTH_IN);
		fprintf(param, "%f\n", MAP_HEIGHT_IN);
		fprintf(param, "%f\n", MAP_OFFSET_TOP);
		fprintf(param, "%f\n", MAP_OFFSET_BOTTOM);
		fprintf(param, "%f\n", MAP_OFFSET_LEFT);
		fprintf(param, "%f\n", MAP_OFFSET_RIGHT);

		fflush(param);
		fclose(param);

		fprintf(stderr,"DONE.\n");
	}

	return;
}

void calibrate() {
	char buffer[1024];

	fprintf(stderr,"ACTION: MOVE PUCK TO HOME POSITION. PRESS CALIBRATE KEY WHEN DONE.\n");

	while(! wait_button("/exhibit/sounds/center.wav"));

	servo_resetposition(LEFT_SERVO);
	servo_resetposition(RIGHT_SERVO);

	param = fopen("calibrate.dat", "r");

	if(param == NULL) {
		play_file(strdup("/exhibit/sounds/recalibrate.wav"));
		sleep(1);

		fprintf(stderr,"NOTICE: NO CALIBRATION FILE WAS FOUND. MANUAL CALIBRATION WILL NOW OCCUR.\n");

		manual_calibration();
	} else {
		fprintf(stderr,"NOTICE: A CALIBRATION FILE WAS FOUND. USING PARAMETERS FOUND FROM LAST CALIBRATION...\n");

		fgets(buffer, 1024, param);
		MAP_WIDTH_IN = atof(buffer);

		fgets(buffer, 1024, param);
		MAP_HEIGHT_IN = atof(buffer);

		fgets(buffer, 1024, param);
		MAP_OFFSET_TOP = atof(buffer);

		fgets(buffer, 1024, param);
		MAP_OFFSET_BOTTOM = atof(buffer);

		fgets(buffer, 1024, param);
		MAP_OFFSET_LEFT = atof(buffer);

		fgets(buffer, 1024, param);
		MAP_OFFSET_RIGHT = atof(buffer);

		fclose(param);
	}	

	fprintf(stderr,"NOTICE: MAP WIDTH=%f\nNOTICE: MAP HEIGHT=%f\nNOTICE: TOP OFFSET=%f\nNOTICE: BOTTOM OFFSET=%f\nNOTICE: LEFT OFFSET=%f\nNOTICE: RIGHT OFFSET=%f\n", MAP_WIDTH_IN, MAP_HEIGHT_IN, MAP_OFFSET_TOP, MAP_OFFSET_BOTTOM, MAP_OFFSET_LEFT, MAP_OFFSET_RIGHT);

	servo_init();

	servo_move(LEFT_SERVO, 0, SERVO_MAX_VEL_SLEW);
	servo_move(RIGHT_SERVO, 0, SERVO_MAX_VEL_SLEW);

  	while(servo_moving(LEFT_SERVO) || servo_moving(RIGHT_SERVO))
                usleep(500);

	return;
}
