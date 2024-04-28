#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include "skitour.h"


#define SKIERS_MIN 0
#define SKIERS_MAX 20000
#define STOPS_MIN 1
#define STOPS_MAX 10
#define CAPACITY_MAX 100
#define CAPACITY_MIN 10
#define TL_MAX 10000
#define TL_MIN 0
#define TB_MAX 1000
#define TB_MIN 0

enum arguments{PROGRAM_NAME, SKIERS_NUM, STOPS_NUM, BUS_CAPACITY, TL, TB, EXPECTED_ARGC};

int main(int argc, char** argv)
{
	if(argc != EXPECTED_ARGC)
	{
		fprintf(stderr, "Too few arguments!\n");
		return 1;
	}
	int args[argc-1];
	for(int index = 1; index < argc; index++)
	{
		if(atoi(argv[index]) <= 0)
		{
			if(strlen(argv[index]) != 1 || argv[index][0] != '0')//if not an integer and not zero
			{
				fprintf(stderr, "Bad arguments!\n");
				return 1;
			}
			
		}
		switch(index)
		{
			case SKIERS_NUM:
				if(atoi(argv[index]) >= SKIERS_MAX || atoi(argv[index]) < SKIERS_MIN)
				{
					fprintf(stderr, "Invalid number of skiers!\n");
					return 1;
				}
				break;
			case STOPS_NUM:
				if(atoi(argv[index]) > STOPS_MAX || atoi(argv[index]) < STOPS_MIN)
				{
					fprintf(stderr, "Invalid number of stops!\n");
					return 1;
				}
				break;
			case BUS_CAPACITY:
				if(atoi(argv[index]) > CAPACITY_MAX || atoi(argv[index]) < CAPACITY_MIN)
				{
					fprintf(stderr, "Invalid bus capacity!\n");
					return 1;
				}
				break;
			case TL:
				if(atoi(argv[index]) > TL_MAX || atoi(argv[index]) < TL_MIN)
				{
					fprintf(stderr, "Invalid TL (fourth argument)!\n");
					return 1;
				}
				break;
			case TB:
				if(atoi(argv[index]) > TB_MAX || atoi(argv[index]) < TB_MIN)
				{
					fprintf(stderr, "Invalid TB (fifth argument)!\n");
					return 1;
				}
		}
		args[index-1] = atoi(argv[index]);
	}

	Bus skibus = bus_init(args[BUS_CAPACITY-1], args[SKIERS_NUM-1], args[TB-1]);//create a bus
	Route skitour = route_init(args[STOPS_NUM-1]+1);//create a route
	if(skibus.max_transfer_time == -1 || skitour.num_of_stations == -1)
	{
		//printf("There was a cleanup");
		bus_cleanup(&skibus);
		route_cleanup(&skitour);
		return 1;
	}

	int bus_proc;
	int pids[SKIERS_MAX+1] = {0};
        switch(bus_proc = bus(&skibus, &skitour))//start a bus
	{
		case -1: //failed to fork
			bus_cleanup(&skibus);
			route_cleanup(&skitour);
			return 1;
		case 0: //OK
			return 0;
		default:
			pids[0] = bus_proc;
			break;
	}

	for(int skiist_num = 0; skiist_num < args[SKIERS_NUM-1]; skiist_num++)
	{
		srand(((skiist_num * skiist_num)^(~0))<<(skiist_num)%sizeof(int));//pseudo-random number generator
		Skiist my_skiist = {skiist_num+1, rand()%(skitour.num_of_stations)+1, args[STOPS_NUM-1]+1, args[TL-1]};//init a skiist, 1 command
		//my_skiist.to = my_skiist.from + rand()%(skitour.num_of_stations - my_skiist.from) + 1;
		int skiist_proc;
		switch(skiist_proc = skiist(&my_skiist, &skibus, &skitour))
		{
			case -1:
				for(int process = 0; process < skiist_num+1; process++)
					kill(pids[process],0);
				bus_cleanup(&skibus);
				route_cleanup(&skitour);
				return 1;
			case 0:
				return 0;
			default:
				pids[skiist_num+1] = skiist_proc;
				break;
		}	
	}
	int finished_proc, status;
	while((finished_proc = wait(&status)) > 0);
	int bclean = bus_cleanup(&skibus);
	int rclean = route_cleanup(&skitour);
	return (bclean || rclean);
}
