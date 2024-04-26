#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

sem_t *mysem;
sem_t *mysem1;

typedef struct bus_t{
	sem_t *entrance;
	sem_t *can_start;
	sem_t *capacity;
	int total_passengers_left;
	int max_transfer_time;
} Bus;

typedef struct route_t{
	int num_of_stations;
	int *to_pick;
	int *to_drop;
	sem_t *inout;//statistics controller
	sem_t **isbus;
} Route;

typedef struct skiist_t{
	int name; //literally 1984
	int from;
	int to;
	int max_breakfast_time;
} Skiist;


void bus(Bus *bus, Route *route)
{
	while(bus->total_passengers_left > 0)
	{
		for(int station = 0; station < route->num_of_stations; station++)
		{
        		sem_post(route->isbus[station]);
        		printf("BUS: bus arrived to station %d\n", station+1);
			sem_post(bus->entrance);
        		printf("BUS: Waiting for departure\n");
        		sem_wait(bus->can_start);
			sem_wait(bus->entrance);
			sem_post(route->inout);
        		printf("BUS: Going to station %d\n", station+2);
			usleep(rand()%(bus->max_transfer_time+1));
		}
	}
        return;
}

void skiist(Skiist *skiist, Bus *bus, Route *route)
{	
	printf("SKIIST %d: Started\n", skiist->name);
	usleep(rand()%(skiist->max_breakfast_time+1));

	sem_wait(route->inout);
	printf("SKIIST %d: Arrived to %d\n", skiist->name, skiist->from);
	route->to_pick[skiist->from-1]++;
	sem_post(route->inout);

	while(1)
	{
        	sem_wait(route->isbus[skiist->from-1]);
		sem_post(route->isbus[skiist->from-1]);//add index when more stations
		

		sem_wait(bus->entrance);
		int seats_left;
		sem_getvalue(bus->capacity, &seats_left);
		if(seats_left == 0)
		{
			sem_wait(route->isbus[skiist->from-1]);
			sem_post(bus->can_start);
			sem_post(bus->entrance);
			continue;
		}
		sem_wait(bus->capacity);
		printf("SKIIST %d: Boarding\n", skiist->name);
		route->to_pick[skiist->from-1]--;

		if(route->to_pick[skiist->from-1] == 0)
		{
			sem_wait(route->inout);
			printf("Skiist %d says ithere si no one an the station %d\n", skiist->name, skiist->from);
        		sem_wait(route->isbus[skiist->from-1]);
			sem_post(bus->can_start);
		}

		sem_post(bus->entrance);


        	printf("SKIIST %d: on board\n", skiist->name);
		break;
	}
	return;
}

int main(int argc, char** argv)
{
	int args[argc-1];
	for(int index = 1; index < argc; index++)
		args[index-1] = atoi(argv[index]);

	Bus skibus; 
	skibus.entrance = (sem_t *)mmap(NULL, sizeof(sem_t),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(skibus.entrance, 1, 0);
	skibus.can_start= (sem_t *)mmap(NULL, sizeof(sem_t),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(skibus.can_start, 1, 0);
	skibus.capacity= (sem_t *)mmap(NULL, sizeof(sem_t),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(skibus.capacity, 1, args[2]);
	skibus.total_passengers_left = args[0];
	skibus.max_transfer_time = args[4];
	
	Route skitour;
	skitour.inout = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(skitour.inout, 1, 1);
	skitour.isbus= (sem_t **)mmap(NULL, args[1] * sizeof(sem_t *), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	skitour.to_pick = (int *)mmap(NULL, args[1] * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	skitour.to_drop = (int *)mmap(NULL, args[1] * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	skitour.num_of_stations = args[1];
	for(int station = 0; station < args[1]; station++)
	{
		skitour.to_pick[station] = 0;
		skitour.to_drop[station] = 0;
		skitour.isbus[station] = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		sem_init(skitour.isbus[station], 1, 0);
	}

        int check_pid = fork();
        if(check_pid == 0)
	{
                bus(&skibus, &skitour);
		return 0;
	}
	for(int skiist_num = 0; skiist_num < args[0]; skiist_num++)
	{
		check_pid = fork();
		if(check_pid == 0)
		{
			srand(skiist_num * (int)getpid());
			Skiist my_skiist = {skiist_num+1, rand()%(skitour.num_of_stations-1)+1, 0, args[3]};
			my_skiist.to = my_skiist.from + rand()%(skitour.num_of_stations - my_skiist.from) + 1;
			skiist(&my_skiist, &skibus, &skitour);
			return 0;
		}
	}
	return 0;
}
