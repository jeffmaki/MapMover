void 	servo_close();
int 	servo_open();
void 	servo_init();
long 	servo_position(int addr);
void 	servo_move(int addr, long ticks, long velocity);
void 	servo_move_line(float x_new, float y_new);
void 	servo_resetposition(int addr);
int	servo_moving(int addr);
int	servo_limit1(int addr);
