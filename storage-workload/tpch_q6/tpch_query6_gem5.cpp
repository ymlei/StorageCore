#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <cstdint>
#include <unistd.h>
//#include <poll.h>

#include "util.h"

using namespace std;

#define FLASH_ACCESS_TIME 20 * 1e3
//#include "nvme_tpch_query6.h"
#define SD_PROC_MODE 0

// ***********************************
//	Configurations
#define NUMBER_OF_PAGES_IN_A_COMMAND 32
#define REQUEST_SIZE (4096*NUMBER_OF_PAGES_IN_A_COMMAND)
#define LBA_BASE 0x201
// ***********************************

// these functions are used for baseline
void convert(char *date, int &year, int &month, int &day)
{
	char *Year = (char *)calloc(5,1);
	memcpy((void *)Year, (void *)date, 4);
	year = atoi(Year);
	char *Month = (char *)calloc(3,1);
	memcpy((void *)Month, (void *)(date+4), 2);
	month = atoi(Month);
	char *Day = (char *)calloc(3,1);
	memcpy((void *)Day, (void *)(date+6), 2);
	day = atoi(Day);
}

bool compareDates(char *str1, char *str2, int interval) // When interval is 0, this return 1 if the str2 date is less than str1 date
{
	int year1, year2, month1, month2, day1, day2;
	convert(str1, year1, month1, day1);
	convert(str2, year2, month2, day2);
	if(interval == 0)
	{
		if(year2 < year1) return 1;
		else if(year2 > year1) return 0;
		else {
			if(month2 < month1) return 1;
			else if(month2 > month1) return 0;
			else {
				if(day2 < day1) return 1;
				else if(day2 > day1) return 0;
				else {
					return 0;
				}
			}
		}
	}
	else {
		if(year2 < (year1+interval)) return 1;
		else if(year2 > (year1+interval)) return 0;
		else {
			if(month2 < month1) return 1;
			else if(month2 > month1) return 0;
			else {
				if(day2 < day1) return 1;
				else if(day2 > day1) return 0;
				else {
					return 0;
				}
			}
		}
	}
}

void my_sleep(int usec)
{
	struct timeval start, end;
	gettimeofday(&start, NULL);
	gettimeofday(&end, NULL);

	double time;
	time = ((double)end.tv_sec - (double)start.tv_sec) * GIGA;
	time += ((double)end.tv_usec - (double)start.tv_usec);

	while(time < usec) {
		gettimeofday(&end, NULL);
		time = ((double)end.tv_sec - (double)start.tv_sec) * GIGA;
		time += ((double)end.tv_usec - (double)start.tv_usec);
	}
}
unsigned int proc_delay;
unsigned int n_ssd_proc;

extern unsigned int proc_delay;

void tpch_query6_baseline(char *date, double discount, unsigned int quantity, int fd, void *buffer, unsigned int pageCount, double &revenue)
{
	int err;
	char *l_shipdate = (char *)calloc(9,1);
	double *l_discount = (double *)calloc(1,sizeof(double));
	double *l_quantitiy = (double *)calloc(1,sizeof(double));
	double *l_extendedprice = (double *)calloc(1,sizeof(double));

	revenue = 0;
	int offset = 0;

	exec_time_t et_io;
	char et_name[64];
	strcpy(et_name, "IO");
	init_exec_time(&et_io, et_name);

	exec_time_t et_calc;
	strcpy(et_name, "calc");
	init_exec_time(&et_calc, et_name);

	for(unsigned int i=0; i < pageCount; i += NUMBER_OF_PAGES_IN_A_COMMAND)
	{
		unsigned int pageInThisIteration = std::min((unsigned int)NUMBER_OF_PAGES_IN_A_COMMAND, (pageCount-i));
		// copy the file into the buffer:
		//		result = fread (buffer,1,4096,pFile);
		//		if (result != 4096) {break;}
		
		set_start_time(&et_io);
		my_sleep(FLASH_ACCESS_TIME);
		add_exec_time(&et_io);

		set_start_time(&et_calc);
		for(unsigned int j = 0; j < pageInThisIteration; j++)
		{
			while(1)
			{
				memcpy((char *)l_shipdate, (char *)(((char *)buffer)+4096*j+offset+50), 8);
				memcpy((char *)l_discount, (char *)(((char *)buffer)+4096*j+offset+32), 8);
				memcpy((char *)l_quantitiy, (char *)(((char *)buffer)+4096*j+offset+16), 8); 

				//			cout << compareDates(l_shipdate, "19940101", 0) << " " << compareDates("19940101", l_shipdate, 1) << " " << strcmp(l_shipdate, "19940101") << endl;
				if( (((compareDates(l_shipdate, date, 0) == 1) and (compareDates(date, l_shipdate, 1) == 1)) or (strcmp(l_shipdate, date) == 0)) and (*l_discount > (discount - 0.01) ) and (*l_discount < (discount + 0.01)) and (*l_quantitiy < quantity))
				{
					//			cout << "Discount comparison is " << ((*l_discount > (0.06 - 0.01) )) << " " << ((*l_discount < (0.06 + 0.01))) << endl;
					memcpy((char *)l_extendedprice, (char *)(((char *)buffer)+4096*j+offset+24), 8);
					revenue += *l_extendedprice * *l_discount;
					//			cout << "Revenue is = " << revenue << endl;
				}

				offset += 153;
				if((offset + 153) >= (4096) )
				{
					offset = 0;
					break;
				}
			}
		}
		add_exec_time(&et_calc);
	}

	print_exec_time(&et_io);
	print_exec_time(&et_calc);

	free(l_shipdate);
	free(l_discount);
	free(l_extendedprice);
	free(l_quantitiy);
}

    unsigned int temp_array[5] = {6,6,6,6,6};
    char date_array[5][20] = {"19930101","19970101","19940101","19970101","19960101"};
    double discount_array[5] = {0.05, 0.06, 0.05, 0.03, 0.04};
    unsigned int quantity_array[5] = {24, 24, 25, 24, 24};

int main(int argc, char **argv)
{
	exec_time_t et_init, et_all;
	char et_name[64];

	strcpy(et_name, "init program");
	init_exec_time(&et_init, et_name);

	strcpy(et_name, "whole program");
	init_exec_time(&et_all, et_name);

	static const char *perrstr;
	unsigned int pageCount;
	int err, fd;
/*
	if (argc != 4) {
		fprintf(stderr, "Usage: %s <device> <pageCount> <proc_delay>\n", argv[0]);
		return 1;
	}
*/
	pageCount = 1023;
	proc_delay = 0;

	if (pageCount > 0) {
		cout << "Page count: " << pageCount << endl;
	} else {
		cout << "Page count should be bigger than 0" << endl;
		exit(1);
	}
	n_ssd_proc = 0;

	// DB data should be stored in SSD

	void *buffer;
	
	buffer = calloc(1, REQUEST_SIZE);

	/*Read the values written and process each block by block*/
	/*Things to do: 1) Experiment with various read lengths */

	unsigned int NumberOfQueries = 1;

	char date[20];
	double discount;
	unsigned int quantity;

	cout << "Number of queries = " << NumberOfQueries << endl;
	cout << "FLASH_ACCESS_TIME: " << FLASH_ACCESS_TIME << endl;
	add_exec_time(&et_init);
	print_exec_time(&et_init);

	for(unsigned int IterationForQuery = 0; IterationForQuery < NumberOfQueries; IterationForQuery++)
	{
		unsigned int temp;
        temp = temp_array[IterationForQuery];
        memcpy(date, date_array[IterationForQuery],20);
        discount = discount_array[IterationForQuery];
        quantity = quantity_array[IterationForQuery];

		cout << "==============================================================" << endl;
		double revenue = 0;
	#if (SSD_PROC_MODE==0)	// baseline
		tpch_query6_baseline(date, discount, quantity, fd, buffer, pageCount, revenue);
	#endif

		//cout << pageCount << endl;
		cout << IterationForQuery << " revenue = " << revenue << endl;
		cout << "==============================================================" << endl;
	}

	free(buffer);

	add_exec_time(&et_all);
	print_exec_time(&et_all);
    return 0;
}
