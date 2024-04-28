#ifndef _SKITOUR_PACK_
#define _SKITOUR_PACK_
#include <semaphore.h>

typedef struct skiist_t{
	int name; //literally 1984
	int from;
	int to;
	int max_breakfast_time;
} Skiist;

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
	int *counter;
	int *to_pick;
	int *to_drop;
	sem_t *gl_inout;//statistics controller
	sem_t *cnt_mutex;
	sem_t **isbus;
	FILE *logfile;
} Route;

Bus bus_init(int capacity, int total_passengers, int max_transfer_time);
int bus_cleanup(Bus *bus);
Route route_init(int num_of_stations);//route and ctrl
int route_cleanup(Route *route);
int bus(Bus *bus, Route *route);
int skiist(Skiist *skiist, Bus *bus, Route *route);

#endif
