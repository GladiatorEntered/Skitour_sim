#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

sem_t bus_capacity;//kapacita

int* stations; //pro kazdou zastavku, pro kazdeho lyzare na zastavce
int where_bus;

typedef route_t {
	sem_t *can_get_on;//whether the bus is on the stop
	int *crowd;//ppl on each stop
}

typedef struct bus_t {
	int where;
	int capacity;
	int *to_drop;
	int *to_pick;
	sem_t capacity;
} Bus;

typedef struct pass_t {
	int name;
	int from;
	int to;
} Passenger;

void bus(int num_of_stations)
{
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

void passenger(Passenger *passenger, Bus *skibus)
{
	printf("A: L %d: started\n", passenger->name);
	//prijde na zastavku, stane do fronty
	sem_wait(route->can_get_on[passenger->from]);//UPDATE WITH ROUTE!!
	sem_wait(skibus->capacity);
	printf("A: L %d: boarding...", passenger->name)
	route->crowd[i]--;//PSEUDOCODE!!
	sem_post(skibus->capacity); 
	sem_post(route->can_get_on[passenger->from]);//UPDATE WITH ROUTE!!
	//pokud je bus na zastavce, nastoupi (SEMAFOR!)
	//pokud je bus na zastavce vystoupi (SEMAFOR!)
	//ukonci
}


int main(int argc, char** argv)
{
	int check_pid;
	check_pid = fork();
	Bus skibus;
	skibus.where = 0;
	skibus.capacity = argv[3];
	skibus.to_drop = (int*)malloc(argv[2]*sizeof(int));
	skibus.to_pick = (int*)malloc(argv[2]*sizeof(int));
	sem_init(skibus.capacity, 1, argv[3]);
	for(int station = 0; station < argv[2]; station++)
	{
		skibus.to_drop[i] = 0;
		skibus.to_pick[i] = 0;
	}
	if(check_pid == 0)
	{
		bus(&skibus);
		free(skibus.to_drop);
		free(skipus.to_pick);
		return 0;
	}

	for(int skiist = 1; skiist <= argv[1]; skiist++)
	{
		check_pid = fork();
		if(check_pid == 0)
		{
			Passenger passenger = {skiist, rand()%(argv[3] - 1) + 1, 0};
			passenger.to = rand()%(argv[3] - passenger.from) + passenger.from + 1;
			usleep(rand()%(argv[4]+1));
			passenger(&passenger, &skibus);
			return 0;
		}
	}
	return 0;
}
