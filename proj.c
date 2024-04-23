#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>


int* stations; //pro kazdou zastavku, pro kazdeho lyzare na zastavce
int where_bus;

typedef struct route_t {
	sem_t **can_get_on;//whether the bus is on the stop
	int *crowd;//ppl on each stop
} Route;

typedef struct bus_t {
	int num_of_stations;
	int *to_drop;
	int *to_pick;
	sem_t *capacity;
	sem_t *can_start;
} Bus;

typedef struct pass_t {
	int name;
	int from;
	int to;
} Passenger;

void bus(Bus *skibus, Route *route)
{
	printf("A: BUS: started.\n");
	for(int station = 0; station < skibus.num_of_stations || passengers_left == 0; station++)//PSEUDOCODE
	{
		sem_post(route->can_get_on[station]);
		sem_wait(skibus->can_start);
		sem_wait(route->can_get_on[station]);
		printf("Going to station %d\n",station+1);
	}
	return;
	//while(not all passengers)
	//{
	//	sem_wait
	//	go_to_next_station
	//	increment passengers
	//	sem_post
	//}
	//while jsou lyzari na zastavkach
	//do smycku 
	//kde cte aktualni zastavku pokud se nevyprazdni
	//a jede dale
	//kdyz je na konci, jede na konecnou
}

void passenger(Passenger *passenger, Bus *skibus, Route *route)
{
	printf("A: L %d: started\n", passenger->name);
	//prijde na zastavku, stane do fronty
	sem_wait(route->can_get_on[passenger->from]);//UPDATE WITH ROUTE!!
	sem_wait(skibus->capacity);
	printf("A: L %d: boarding...", passenger->name);
	route->crowd[passenger->from]--;//PSEUDOCODE!!
	sem_post(skibus->capacity); 
	sem_post(route->can_get_on[passenger->from]);//UPDATE WITH ROUTE!!
	//pokud je bus na zastavce, nastoupi (SEMAFOR!)
	//pokud je bus na zastavce vystoupi (SEMAFOR!)
	//ukonci
}


int main(int argc, char** argv)
{
	int check_pid;
	int args[argc];
	for(int arg = 1; arg <= argc; arg++)
		args[arg-1] = atoi(argv[arg]);
	check_pid = fork();
	Bus skibus;
	skibus.num_of_stations = args[1];
	skibus.to_drop = (int*)malloc(args[1]*sizeof(int));
	skibus.to_pick = (int*)malloc(args[1]*sizeof(int));
	sem_init(skibus.capacity, 1, args[2]);
	for(int station = 0; station < args[1]; station++)
	{
		skibus.to_drop[station] = 0;
		skibus.to_pick[station] = 0;
	}

	Route route;
	route.crowd = (int*)malloc(args[1]*sizeof(int));
	route.can_get_on = (sem_t**)malloc(args[1]*sizeof(sem_t*));
	for(int station = 0; station < args[1]; station++)
	{
		sem_init(route.can_get_on[station], 1, 0);
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
		if(check_pid == 0)
		{
			Passenger skiist_waiting = {skiist, rand()%(args[2] - 1) + 1, 0};
			skiist_waiting.to = rand()%(args[2] - skiist_waiting.from) + skiist_waiting.from + 1;
			usleep(rand()%(args[3]+1));
			passenger(&skiist_waiting, &skibus, &route);
			return 0;
		}
	}
	return 0;
}
