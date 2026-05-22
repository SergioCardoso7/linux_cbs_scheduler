CC=/usr/bin/gcc

all: launcher  task
launcher: launch.c 
	${CC} -Wall  launch.c -o launcher -lrt

task: task.c 
	${CC} -Wall task.c -o task -lrt

clean:
	rm -f *.o *.a *~ launcher  task
