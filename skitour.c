#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
//#include <signal.h>
#include "skitour.h"

Bus bus_init(int capacity, int total_passengers, int max_transfer_time)
{
	Bus bus;
	bus.max_transfer_time = max_transfer_time;
	bus.gone= (sem_t *)mmap(NULL, sizeof(sem_t),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	bus.can_start= (sem_t *)mmap(NULL, sizeof(sem_t),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	bus.capacity= (sem_t *)mmap(NULL, sizeof(sem_t),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	bus.total_passengers_left_ptr = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	bus.seats_left = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	if(bus.gone == MAP_FAILED || bus.can_start == MAP_FAILED || bus.capacity == MAP_FAILED || bus.total_passengers_left_ptr == MAP_FAILED || bus.seats_left == MAP_FAILED)
		bus.max_transfer_time = -1;

	int control = 0;
	control += sem_init(bus.gone, 1, 0);
	control += sem_init(bus.can_start, 1, 0);
	control += sem_init(bus.capacity, 1, capacity);

	if(control < 0)
		bus.max_transfer_time = -1;

	*(bus.total_passengers_left_ptr) = total_passengers;
	*(bus.seats_left) = capacity;

	return bus;
}

int bus_cleanup(Bus *bus)
{
	int mmap_control = 0;
	int sem_control = 0;
	sem_control += sem_destroy(bus->gone);
	mmap_control += munmap(bus->gone, sizeof(sem_t));
	sem_control += sem_destroy(bus->can_start);
	mmap_control += munmap(bus->can_start, sizeof(sem_t));
	sem_control += sem_destroy(bus->capacity);
	mmap_control += munmap(bus->capacity, sizeof(sem_t));
	mmap_control += munmap(bus->total_passengers_left_ptr, sizeof(int));
	mmap_control += munmap(bus->seats_left, sizeof(int));
	if(mmap_control < 0)
	{
		fprintf(stderr, "Shared memory failed to deallocate!\n");
		return 1;
	}
	if(sem_control < 0)
	{
		fprintf(stderr, "Semaphore destruction failed!\n");
		return 1;
	}
	return 0;
}

Route route_init(int num_of_stations)
{
	Route route;
	route.num_of_stations = num_of_stations;
	route.cnt_mutex = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	route.gl_inout = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	route.isbus= (sem_t **)mmap(NULL, num_of_stations * sizeof(sem_t *), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	route.to_pick = (int *)mmap(NULL, num_of_stations * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	route.to_drop = (int *)mmap(NULL, num_of_stations * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	route.counter = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	if(route.gl_inout == MAP_FAILED || route.isbus == MAP_FAILED || route.to_pick == MAP_FAILED || route.to_drop == MAP_FAILED || route.counter == MAP_FAILED)
	{
		fprintf(stderr, "Shared memory failed to allocate!\n");
		route.num_of_stations = -1;
	}

	if(sem_init(route.gl_inout, 1, 1) == -1 || sem_init(route.cnt_mutex, 1, 1) == -1)
	{
		fprintf(stderr, "Semaphore initialization error!\n");
		route.num_of_stations = -1;
	}
	*route.counter = 0;
	for(int station = 0; station < num_of_stations; station++)
	{
		route.to_pick[station] = 0;
		route.to_drop[station] = 0;
		route.isbus[station] = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		if(route.isbus[station] == MAP_FAILED)
		{
			fprintf(stderr, "Shared memory failed to allocate!\n");
			route.num_of_stations = -1;
		}
		if(sem_init(route.isbus[station], 1, 0) == -1)
		{
			fprintf(stderr, "Semaphore initialization error!\n");
			route.num_of_stations = -1;
		}
	}
	
	if((route.logfile = fopen("proj2.out","w")) == NULL)
	{
		printf("Log file failed to open!\n");
		route.num_of_stations = -1;
	}else
		setvbuf(route.logfile, NULL, _IONBF, 0);
	return route;
}

int route_cleanup(Route *route)
{
	int mmap_control = 0;
	int sem_control = 0;
	sem_control += sem_destroy(route->gl_inout);
	mmap_control += munmap(route->gl_inout, sizeof(sem_t));
	sem_control += sem_destroy(route->cnt_mutex);
	mmap_control += munmap(route->cnt_mutex, sizeof(sem_t));
	mmap_control += munmap(route->to_pick, route->num_of_stations * sizeof(int));
	mmap_control += munmap(route->to_drop, route->num_of_stations * sizeof(int));
	mmap_control += munmap(route->counter, sizeof(int));
	for(int station = 0; station < route->num_of_stations; station++)
	{
		sem_control += sem_destroy(route->isbus[station]);
		mmap_control += munmap(route->isbus[station], sizeof(sem_t));
	}
	mmap_control += munmap(route->isbus, route->num_of_stations * sizeof(sem_t *));

	if(sem_control < 0)
	{
		fprintf(stderr, "Semaphore destruction failed!\n");
		return 1;
	}
	if(mmap_control < 0)
	{
		fprintf(stderr, "Shared memory failed to deallocate!\n");
		return 1;
	}
	if(fclose(route->logfile) == EOF)
	{
		printf("Log file failed to close!\n");
		return 1;
	}
	return 0;
}

int bus(Bus *bus, Route *route)
{
	int check_pid = fork();
	if(check_pid < 0)
	{
		fprintf(stderr, "Failed to create a \"bus\" process\n");
		return -1;
	}else if(check_pid > 0)
		return check_pid;
	sem_wait(route->cnt_mutex);
	fprintf(route->logfile, "%d: BUS: started\n", ++(*route->counter));
	sem_post(route->cnt_mutex);
	while((*bus->total_passengers_left_ptr) != 0)//maybe change to do-while
	{
		for(int station = 0; station < route->num_of_stations; station++)
		{
			usleep(rand()%(bus->max_transfer_time+1));
			sem_wait(route->cnt_mutex);
			fprintf(route->logfile, "%d: BUS: arrived to ", ++(*route->counter));
			fprintf(route->logfile, (station < route->num_of_stations - 1)?("%d\n"):("final\n"), station+1);
			sem_post(route->cnt_mutex);
			///START OF THE CRITICAL SECTION MUTEX1///
			sem_wait(route->gl_inout);
			if(((*bus->seats_left) == 0 || route->to_pick[station] == 0) && route->to_drop[station] == 0)
			{
				sem_wait(route->cnt_mutex);
				fprintf(route->logfile, "%d: BUS: leaving ", ++(*route->counter));
				fprintf(route->logfile, (station < route->num_of_stations - 1)?("%d\n"):("final\n"), station+1);
				sem_post(route->cnt_mutex);
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
			sem_wait(route->cnt_mutex);
			fprintf(route->logfile, "%d: BUS: leaving ", ++(*route->counter));
			fprintf(route->logfile, (station < route->num_of_stations - 1)?("%d\n"):("final\n"), station+1);
			sem_post(route->cnt_mutex);
		}
	}
	sem_wait(route->cnt_mutex);
	fprintf(route->logfile, "%d: BUS: finish\n", ++(*route->counter));
	sem_post(route->cnt_mutex);
	return 0;
}

int skiist(Skiist *skiist, Bus *bus, Route *route)
{
	int check_pid = fork();
	if(check_pid < 0)
	{
		fprintf(stderr, "Failed to create a \"skiist\" process\n");
		return -1;
	}else if(check_pid > 0)
		return check_pid;
	sem_wait(route->cnt_mutex);
	fprintf(route->logfile, "%d: L %d: started\n", ++(*route->counter), skiist->name);
	sem_post(route->cnt_mutex);
	usleep(rand()%(skiist->max_breakfast_time+1));
	///START OF CRITICAL SECTION MUTEX1///
	sem_wait(route->gl_inout);
	sem_wait(route->cnt_mutex);
	fprintf(route->logfile, "%d: L %d: arrived to %d\n", ++(*route->counter), skiist->name, skiist->from);
	sem_post(route->cnt_mutex);
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
			sem_wait(route->cnt_mutex);
			fprintf(route->logfile, "%d: L %d: boarding\n", ++(*route->counter), skiist->name);
			sem_post(route->cnt_mutex);
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
	sem_wait(route->cnt_mutex);
	fprintf(route->logfile, "%d: L %d: going to ski\n", ++(*route->counter), skiist->name);
	sem_post(route->cnt_mutex);
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
	return 0;
}
