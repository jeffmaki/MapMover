all:
	gcc -Wall -O2 -o map servo.cpp calibrate.c main.c sound.c object.c db.c -I/usr/include/pgsql -I./libnmc -L/usr/local/pgsql/lib/ -L/lib -lnmc -lpq -lpthread -lm -laudiofile -lesd -lstdc++

clean:
	rm -f map
