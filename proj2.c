#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

sem_t *mysem;
sem_t *mysem1;

typedef struct bus_t{
	sem_t *can_start;
	sem_t *capacity;
	sem_t *gone;//init with 0
	int *total_passengers_left_ptr;
	int max_transfer_time;
	int *seats_left;
} Bus;

typedef struct route_t{
	int num_of_stations;
	int *to_pick;
	int *to_drop;
	sem_t *gl_inout;//statistics controller
	sem_t **isbus;
} Route;

typedef struct skiist_t{
	int name; //literally 1984
	int from;
	int to;
	int max_breakfast_time;
} Skiist;

Bus bus_init(int capacity, int total_passengers, int max_transfer_time)
{
	Bus bus;
	bus.gone= (sem_t *)mmap(NULL, sizeof(sem_t),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(bus.gone, 1, 0);
	bus.can_start= (sem_t *)mmap(NULL, sizeof(sem_t),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(bus.can_start, 1, 0);
	bus.capacity= (sem_t *)mmap(NULL, sizeof(sem_t),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(bus.capacity, 1, capacity);
	bus.total_passengers_left_ptr = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*(bus.total_passengers_left_ptr) = total_passengers;
	bus.seats_left = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*(bus.seats_left) = capacity;
	bus.max_transfer_time = max_transfer_time;

	return bus;
}

Route route_init(int num_of_stations)
{
	Route route;
	route.gl_inout = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	sem_init(route.gl_inout, 1, 1);
	route.isbus= (sem_t **)mmap(NULL, num_of_stations * sizeof(sem_t *), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	route.to_pick = (int *)mmap(NULL, num_of_stations * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	route.to_drop = (int *)mmap(NULL, num_of_stations * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	route.num_of_stations = num_of_stations;
	for(int station = 0; station < num_of_stations; station++)
	{
		route.to_pick[station] = 0;
		route.to_drop[station] = 0;
		route.isbus[station] = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		sem_init(route.isbus[station], 1, 0);
	}
	return route;
}


void bus(Bus *bus, Route *route)
{
	while((*bus->total_passengers_left_ptr) != 0)
	{
		for(int station = 0; station < route->num_of_stations; station++)
		{
			usleep(rand()%(bus->max_transfer_time+1));
			printf("Bus arrived to station %d\n", station+1);
			///START OF THE CRITICAL SECTION MUTEX1///
			sem_wait(route->gl_inout);
			if(((*bus->seats_left) == 0 || route->to_pick[station] == 0) && route->to_drop[station] == 0)
			{
				printf("Bus goes to station %d just because\n", station+2);
				sem_post(route->gl_inout);
				///END OF THE CRITICAL SECTION MUTEX1///
				continue;
			}
			sem_post(route->gl_inout);
			///END OF THE CRITICAL SECTION MUTEX1///
			sem_post(route->isbus[station]);
			sem_wait(bus->can_start);//waits until something sets semaphore to 1
			//printf("log for test\n");
			sem_wait(route->isbus[station]);
			sem_post(bus->gone);
			printf("Bus goes to station %d\n", station+2);
		}
	}
	return;
}

void skiist(Skiist *skiist, Bus *bus, Route *route)
{	
	printf("Skiist %d started\n", skiist->name);
	usleep(rand()%(skiist->max_breakfast_time+1));
	///START OF CRITICAL SECTION MUTEX1///
	sem_wait(route->gl_inout);
	printf("Skiist %d arrived to station %d and goes to %d\n", skiist->name, skiist->from, skiist->to);
	route->to_pick[skiist->from-1]++;//transport me CRITICAL1
	sem_post(route->gl_inout);
	///END OF CRITICAL SECTION MUTEX1///
	while(1)
	{
		sem_wait(route->isbus[skiist->from-1]);//wait until bus arrives CRITICAL PAIR
		sem_post(route->isbus[skiist->from-1]);//MUST BE ATOMIC
		sem_wait(bus->capacity);

		///START OF CRITICAL SECTION MUTEX1///
		sem_wait(route->gl_inout);
		int is_bus;
		sem_getvalue(route->isbus[skiist->from-1], &is_bus);
		if(is_bus == 1)//RACE FOR CONDITION CRITICAL
		{
			route->to_pick[skiist->from-1]--;//doesnt wait anymore CRITICAL1 DATA
			route->to_drop[skiist->to-1]++;//drop me there CRITICAL1 DATA
			printf("Skiist %d: boarding...\n", skiist->name);
			(*bus->seats_left)--;//CRITICAL
			if(((*bus->seats_left) == 0 || route->to_pick[skiist->from-1] == 0) && route->to_drop[skiist->from-1] == 0) //CRITICAL DATA
			{
				//printf("No one left waiting\n");
				sem_post(bus->can_start);
				sem_wait(bus->gone);//no one can get in after bus has gone
				sem_post(route->gl_inout);
				///END OF CRITICAL SECTION MUTEX1///
			}else
				sem_post(route->gl_inout);
			break;
		}else
		{
			sem_post(bus->capacity);
			sem_post(route->gl_inout);
			///END OF CRITICAL SECTION MUTEX1///
			continue;//viz upper
		}
	}
	
	sem_wait(route->isbus[skiist->to-1]); //CRITICAL PAIR
	sem_post(route->isbus[skiist->to-1]); //MUST BE ATOMIC
	///START OF CRITICAL SECTION MUTEX1///
	sem_wait(route->gl_inout);
	printf("Skiist %d got off the bus at %d\n", skiist->name, skiist->to);
	route->to_drop[skiist->to-1]--;//already dropped CRITICAL1 DATA
	(*bus->total_passengers_left_ptr)--;//CRITICAL1 DATA, MUST BE ATOMIC
	(*bus->seats_left)++;
	sem_post(bus->capacity);
	if(route->to_drop[skiist->to-1] == 0 && route->to_pick[skiist->to-1] == 0)//CRITICAL DATA
	{
		sem_post(bus->can_start);
		//printf("let it go\n");
		//bus sets isbus[skiist->to-1] to 0
		sem_wait(bus->gone); //and only then release any mutex at the end
	}
	sem_post(route->gl_inout);
		///END OF CRITICAL SECTION MUTEX1///
	return;
}

int main(int argc, char** argv)
{
	if(argc != 6)
	{
		printf("Too few arguments!\n");
		return 1;
	}
	int args[argc-1];
	for(int index = 1; index < argc; index++)
	{
		if(atoi(argv[index]) == 0)
		{
			switch(index)
			{
				case 4:
				case 5:
					printf("Bad arguments!\n");
					return 1;
				default:
					if(strlen(argv[index]) != 1 || argv[index][0] != '0')
					{
						printf("Bad arguments!\n");
						return 1;
					}
			}
		}
		args[index-1] = atoi(argv[index]);
	}

	Bus skibus = bus_init(args[2], args[0], args[4]); 
	
	Route skitour = route_init(args[1]);

        int check_pid = fork();
        if(check_pid == 0)
	{
                bus(&skibus, &skitour);
		return 0;
	}else if(check_pid < 0) 
	{
		kill(check_pid, 9);
		return 1;
	}

	for(int skiist_num = 0; skiist_num < args[0]; skiist_num++)
	{
		int skiist_pid = fork();
		if(skiist_pid < 0)
		{
			kill(skiist_pid, 9);
			return 1;
		}else if(skiist_pid == 0)
		{
			srand(skiist_num * (int)getpid());
			Skiist my_skiist = {skiist_num+1, rand()%(skitour.num_of_stations-1)+1, args[1], args[3]};
			//my_skiist.to = my_skiist.from + rand()%(skitour.num_of_stations - my_skiist.from) + 1;
			skiist(&my_skiist, &skibus, &skitour);
			return 0;
		}
	}
	int finished_proc, status;
	while((finished_proc = wait(&status)) > 0)
	{
		if(WEXITSTATUS(status) != 0 || WTERMSIG(status))
		{
			printf("SMT WRONG..., %d\n", WTERMSIG(status));
			return 1;
		}
	}
	return 0;
}
