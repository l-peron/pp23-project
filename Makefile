DIRS=build

$(shell mkdir -p $(DIRS))

default: pthread sequential mpi

pthread: src/pairs_pthread.c
	gcc src/pairs_pthread.c -o build/pthread

sequential: src/pairs_sequential.c
	gcc src/pairs_sequential.c -o build/sequential

mpi: src/pairs_mpi.c
	mpicc src/pairs_mpi.c -o build/mpi

clean:
	-rm -fR build