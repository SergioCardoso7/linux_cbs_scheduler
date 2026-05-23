CC=/usr/bin/gcc

all: launcher  task server
launcher: launch.c 
	${CC} -Wall  launch.c -o launcher -lrt

task: task.c 
	${CC} -Wall task.c -o task -lrt

server: server_task.c
	${CC} -Wall server_task.c -o server_task -lrt

clean:
	rm -f *.o *.a *~ launcher  task server_task
