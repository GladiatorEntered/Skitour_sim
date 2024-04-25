#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>


int* stations; //pro kazdou zastavku, pro kazdeho lyzare na zastavce
int where_bus;

typedef struct route_t {
	sem_t *can_get_on;//whether the bus is on the stop
	int *crowd;//ppl on each stop
	int total_passengers_left;
	sem_t can_stand_in_line;
} Route;

typedef struct bus_t {
	int num_of_stations;
	int *to_drop;
	int *to_pick;
	sem_t capacity;
	sem_t can_start;
} Bus;

typedef struct pass_t {
	int name;
	int from;
	int to;
} Passenger;


void bus(Bus *skibus, Route *route)
{
	for(int station = 0; station < skibus->num_of_stations; station++)
	{
		sem_post(&route->can_get_on[station]);
		printf("Semaphore %d opened\n", station);
		sem_wait(&skibus->can_start);
		printf("BUS GOES FURTHER\n");
		sem_wait(&route->can_get_on[station]);
	}
}

void passenger(Passenger *passenger, Bus *skibus, Route *route)
{
	printf("A: passenger %d arrived to station %d\n", passenger->name, passenger->from-1);
	sem_wait(&route->can_stand_in_line);//go to line
	printf("Passenger is waiting\n");
	route->crowd[passenger->from-1]++;
	sem_post(&route->can_stand_in_line);//leave the enter_queue state
	printf("Waiting for opened semaphore at %d\n", passenger->from-1);
	sem_wait(&route->can_get_on[passenger->from-1]);//literally wait for a bus
	route->crowd[passenger->from-1]--;
	printf("Passenger %d allowed to enter\n", passenger->name);
	sem_post(&route->can_get_on[passenger->from-1]);//bus can start
	sem_post(&skibus->can_start);
}


int main(int argc, char** argv)
{
	int check_pid;
	int args[argc-1];
	for(int arg = 1; arg <= argc-1; arg++)
		args[arg-1] = atoi(argv[arg]);
	check_pid = fork();
	Bus skibus;
	skibus.num_of_stations = args[1];
	skibus.to_drop = (int*)mmap(NULL, args[1]*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	skibus.to_pick = (int*)mmap(NULL, args[1]*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(&skibus.capacity, 1, args[2]);
	sem_init(&skibus.can_start,1, 0);
	for(int station = 0; station < args[1]; station++)
	{
		skibus.to_drop[station] = 0;
		skibus.to_pick[station] = 0;
	}

	Route route;
	route.crowd = (int*)mmap(NULL, args[1]*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	route.can_get_on = (sem_t *)mmap(NULL, args[1]*sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	route.total_passengers_left = args[0];
	sem_init(&route.can_stand_in_line, 1, 1);
	for(int station = 0; station < args[1]; station++)
	{
		sem_init(&route.can_get_on[station], 1, 0);
		route.crowd[station] = 0;
	}

	if(check_pid == 0)
	{
		bus(&skibus, &route);
		free(skibus.to_drop);
		free(skibus.to_pick);
		return 0;
	}


	for(int skiist = 1; skiist <= args[0]; skiist++)
	{
		check_pid = fork();
		srand(getpid());
		if(check_pid == 0)
		{
			Passenger skiist_waiting = {1, rand()%(args[1] - 1) + 1, 0};
			skiist_waiting.to = rand()%(args[1] - skiist_waiting.from) + skiist_waiting.from + 1;
			usleep(rand()%(args[3]+1));
			passenger(&skiist_waiting, &skibus, &route);
			return 0;
		}
	}
	return 0;
}
