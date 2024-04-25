#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

sem_t *mysem;
sem_t *mysem1;

typedef struct bus_t{
	sem_t entrance;
	sem_t can_start;
	sem_t capacity;
	int total_passengers_left;
} Bus;

typedef struct route_t{
	int num_of_stations;
	int to_pick;
	int to_drop;
	sem_t inout;//statistics controller
	sem_t isbus;
} Route;

typedef struct skiist_t{
	int name; //literally 1984
	int from;
	int to;
} Skiist;


void bus(Bus *bus, Route *route)
{
        sem_post(&route->isbus);
        printf("BUS: bus arrived\n");
        printf("BUS: Waiting for departure\n");
        sem_wait(&bus->can_start);
        printf("BUS: Started\n");
        return;
}

void skiist(Skiist *skiist, Bus *bus, Route *route)
{
        printf("SKIIST: Waiting for bus\n");
        sem_wait(&route->isbus);
        printf("SKIIST: boarding on the bus\n");
        sem_post(&bus->can_start);
        printf("SKIIST: on board\n");
	return;
}

int main(int argc, char** argv)
{
	int args[argc-1];
	for(int index = 1; index < argc; index++)
		args[index-1] = atoi(argv[index]);

	Bus skibus = *(Bus *)mmap(NULL,sizeof(Bus),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(&skibus.entrance, 1, 0);
	sem_init(&skibus.can_start, 1, 0);
	sem_init(&skibus.capacity, 1, args[2]);
	skibus.total_passengers_left = args[0];
	
	Route skitour = *(Route *)mmap(NULL,sizeof(Route), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(&skitour.inout, 1, 1);
	sem_init(&skitour.isbus, 1, 1);
	skitour.to_pick = 0;
	skitour.to_drop = 0;
	skitour.num_of_stations = 1;

        int check_pid = fork();
        if(check_pid == 0)
	{
                bus(&skibus, &skitour);
		return 0;
	}
	check_pid = fork();
	if(check_pid == 0)
	{
		Skiist my_skiist = {1,1,1};
		skiist(&my_skiist, &skibus, &skitour);
		return 0;
	}
	return 0;
}
