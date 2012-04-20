#include <math.h>
#include <stdlib.h>

#include "config.h"
#include "sio_util.h"
#include "nmccom.h"
#include "picio.h"
#include "picservo.h"

static int nummod;

extern "C" void servo_close() {
  	if (nummod == 0) 
		return;

  	printf("SERVO: SHUTTING DOWN SERVO SYSTEM...\n");  

  	NmcShutdown();
}

extern "C" int servo_open() {
	printf("SERVO: ENUMERATING SERVO CONTROLLERS...\n");

	nummod = NmcInit("/dev/ttyS0", 19200);

	if (nummod==0) {  
		printf("SERVO: NO SERVO CONTROLLERS FOUND.\n");
		servo_close();
	} else {
  		for (int i=0; i<nummod; i++) {
    			if (NmcGetModType((byte)(i+1)) == SERVOMODTYPE)
      				printf("SERVO: MODULE %d: PIC-SERVO CONTROLLER\n", i+1);
    
			if (NmcGetModType((byte)(i+1)) == IOMODTYPE)
      				printf("SERVO: MODULE %d: PIC-IO CONTROLLER\n", i+1);
    		}
  	}

	return nummod;
}

extern "C" void servo_init() {
  	for (int i=0; i<nummod; i++) {
		ServoSetGain(i + 1,
 	     		100,	//Kp
		        1000,	//Kd
             		0,	//Ki
		     	0,	//IL
             		255,	//OL
            		0,	//CL
             		600,	//EL
             		1,	//SR
             		0	//DC
       		);

		ServoStopMotor(i + 1, AMP_ENABLE | MOTOR_OFF);
		ServoStopMotor(i + 1, AMP_ENABLE | STOP_SMOOTH);
	}
}

extern "C" int servo_limit1(int addr) {
	NmcNoOp(addr);
	return (NmcGetStat(addr) & LIMIT1);
}

extern "C" int servo_moving(int addr) {
	NmcNoOp(addr);
	return !(NmcGetStat(addr) & MOVE_DONE);
}

extern "C" long servo_position(int addr) {
	long ticks;

	NmcDefineStatus(addr, SEND_POS);
	ticks = ServoGetPos(addr);

	if(addr == RIGHT_SERVO)
		ticks = ticks * -1;

	return ticks;
}

extern "C" void servo_move(int addr, long ticks, long velocity) {
	if(addr == RIGHT_SERVO)
		ticks = ticks * -1;

	ServoLoadTraj(
		addr, 	
	    	LOAD_POS | LOAD_VEL | LOAD_ACC | ENABLE_SERVO | START_NOW, 
	        ticks,
		velocity,	
             	1000,
	       	0
       	);

	return;
}

extern "C" void servo_resetposition(int addr) {
	ServoResetPos(addr);
}
