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
	sem_t *gone;//init with 0
	int *total_passengers_left_ptr;
	int max_transfer_time;
} Bus;

typedef struct route_t{
	int num_of_stations;
	int *to_pick;
	int *to_drop;
	sem_t *inout;//statistics controller
	sem_t **isbus;
	sem_t **all_gone;//init with 1s
} Route;

typedef struct skiist_t{
	int name; //literally 1984
	int from;
	int to;
	int max_breakfast_time;
} Skiist;


void bus(Bus *bus, Route *route)
{
	while(*(bus->total_passengers_left_ptr) > 0)
	{
		for(int station = 0; station < route->num_of_stations; station++)
		{
			sem_wait(route->inout);
			sem_wait(route->all_gone[station]);
			int seats_left;
			sem_getvalue(bus->capacity, &seats_left);
        		printf("BUS: bus arrived to station %d with %d people to drop\n", station+1, route->to_drop[station]);
			if(route->to_drop[station] == 0)
			{
				sem_post(route->all_gone[station]);
				if(seats_left <= 0)
				{
					printf("BUS: bus arrived to station %d full\n", station+1);
					printf("BUS: Going to station %d\n", station+2);
					sem_post(route->inout);
					continue;
				}
			}
			sem_post(route->inout);
        		sem_post(route->isbus[station]);
        		//printf("BUS: bus arrived to station %d with %d people on board\n", station+1, 10-seats_left);
			sem_post(bus->entrance);
        		printf("Entrance was released after bus arrived\n");
        		sem_wait(bus->can_start);
			//printf("yay, can start!\n");
			sem_wait(route->isbus[station]);
			sem_post(bus->gone);
			sem_wait(bus->entrance);
			printf("I BLOCKED ENTRANCE\n");
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
	printf("SKIIST %d: Arrived to %d and goes to %d\n", skiist->name, skiist->from, skiist->to);
	route->to_pick[skiist->from-1]++;
	sem_post(route->inout);

	while(1)
	{
        	sem_wait(route->isbus[skiist->from-1]);
		sem_post(route->isbus[skiist->from-1]);//add index when more stations
						       //
		sem_wait(route->all_gone[skiist->from-1]);
		sem_post(route->all_gone[skiist->from-1]);

		sem_wait(bus->entrance);
		printf("no, i blocked entrance!\n");


		int seats_left;
		int is_bus_there;
		sem_getvalue(bus->capacity, &seats_left);
		sem_getvalue(route->isbus[skiist->from-1], &is_bus_there);
		if(is_bus_there == 0)
		{
			sem_post(bus->entrance);//vulnerability!
			printf("Bus is gone, so I released entrance\n");
			continue;
		}
		if(seats_left <= 0)
		{
			printf("Capacity full, let it go\n");
			sem_post(bus->can_start);
			sem_wait(bus->gone);
			sem_post(bus->entrance);//how not to open it at all
			printf("Capacity is full, so I released entrance\n");
			continue;
		}
		sem_wait(bus->capacity);
		printf("SKIIST %d: Boarding\n", skiist->name);
		sem_wait(route->inout);
		route->to_pick[skiist->from-1]--;
		route->to_drop[skiist->to-1]++;
		//printf("%d skiists to drop at %d\n", route->to_drop[skiist->to-1], skiist->to);
		//printf("%d skiists left on station\n",route->to_pick[skiist->from-1]);

		if(route->to_pick[skiist->from-1] == 0)
		{
			printf("Skiist %d says there is no one an the station %d\n", skiist->name, skiist->from);
			sem_post(bus->can_start);
			sem_post(bus->entrance);//how not to open it at all
			printf("Entrance released\n");
			sem_wait(bus->gone);
			//sem_post(route->inout);
			//sem_post(bus->entrance);
		}
		sem_post(route->inout);
		sem_post(bus->entrance);
		printf("entrance was released at the end of boarding\n");

		sem_wait(route->isbus[skiist->to-1]);
		sem_post(route->isbus[skiist->to-1]);

		sem_wait(bus->entrance);
		printf("ENTRANCE BLOCKED\n");
		printf("%d skiists to drop at %d\n", route->to_drop[skiist->to-1], skiist->to);
		printf("SKIIST %d: Got off at station %d\n", skiist->name, skiist->to);
		sem_wait(route->inout);
		route->to_drop[skiist->to-1]--;
		(*bus->total_passengers_left_ptr)--;
		sem_post(bus->capacity);
		printf("%d skiists to drop\n", route->to_drop[skiist->to-1]);
		if(route->to_drop[skiist->to-1] == 0)
		{
			sem_post(route->all_gone[skiist->to-1]);
			//sem_wait(route->inout);
			if(route->to_pick[skiist->to-1] == 0)
			{
				printf("SOMEHOW\n");
				sem_post(bus->can_start);
			}
			//sem_post(route->inout);
		}
		sem_post(route->inout);
		sem_post(bus->entrance);
		printf("Entrance was released in the end of a while loop\n");

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
	skibus.gone= (sem_t *)mmap(NULL, sizeof(sem_t),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(skibus.gone, 1, 0);
	skibus.can_start= (sem_t *)mmap(NULL, sizeof(sem_t),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(skibus.can_start, 1, 0);
	skibus.capacity= (sem_t *)mmap(NULL, sizeof(sem_t),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(skibus.capacity, 1, args[2]);
	skibus.total_passengers_left_ptr = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*(skibus.total_passengers_left_ptr) = args[0];
	skibus.max_transfer_time = args[4];
	
	Route skitour;
	skitour.inout = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(skitour.inout, 1, 1);
	skitour.isbus= (sem_t **)mmap(NULL, args[1] * sizeof(sem_t *), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	skitour.all_gone= (sem_t **)mmap(NULL, args[1] * sizeof(sem_t *), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	skitour.to_pick = (int *)mmap(NULL, args[1] * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	skitour.to_drop = (int *)mmap(NULL, args[1] * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	skitour.num_of_stations = args[1];
	for(int station = 0; station < args[1]; station++)
	{
		skitour.to_pick[station] = 0;
		skitour.to_drop[station] = 0;
		skitour.isbus[station] = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		skitour.all_gone[station] = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		sem_init(skitour.isbus[station], 1, 0);
		sem_init(skitour.all_gone[station], 1, 1);
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
