#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <map>
#include <unordered_map>

#include <cstdint>
#include <unistd.h>

using namespace std;

// *********************************************
//	Configurations
#define NUMBER_OF_PAGES_IN_A_COMMAND 32
#define REQUEST_SIZE (4096*NUMBER_OF_PAGES_IN_A_COMMAND)
#define LBA_BASE 0x201
// *********************************************


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

bool compareDates(char *str1, char *str2, int interval)
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
		if(year2 < (year1)) return 1;
		else if(year2 > (year1)) return 0;
		else {
			if(month2 < (month1+interval)) return 1;
			else if(month2 > (month1+interval)) return 0;
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

unsigned int proc_delay;
unsigned int n_ssd_proc;


void tpch_query14_baseline(char *date, int fd, void *buffer, unsigned int pageCount1, unsigned int pageCount2, double & promo_revenue, double & revenue)
{
	int err;
	char *l_shipdate = (char *)calloc(9,1);
	double *l_discount = (double *)calloc(1,sizeof(double));
	double *l_quantitiy = (double *)calloc(1,sizeof(double));
	double *l_extendedprice = (double *)calloc(1,sizeof(double));
	unsigned int *l_partkey = (unsigned int *) calloc(1, sizeof(unsigned int));
	unsigned long long int *p_partkey = (unsigned long long int *) calloc(1, sizeof(unsigned long long int));

	char *p_type;
	
	std::unordered_map<unsigned long long int, char *> hashtable;

	int offset2=0;

	for(unsigned int i2 = pageCount1; i2 < pageCount2; i2+=NUMBER_OF_PAGES_IN_A_COMMAND)
	{
		unsigned int pageInThisIteration2 = std::min((unsigned int)NUMBER_OF_PAGES_IN_A_COMMAND, (pageCount2-i2));

		for(unsigned int j2 = 0; j2 < pageInThisIteration2; j2++)
		{
			while(1)
			{
				memcpy((char *)p_partkey, (char *)(((char *)buffer)+4096*j2+offset2+0), 8);
				p_type = (char *)calloc(1,6);
//				std::string p_type((((char *)buffer)+4096*j2+offset2+98) ,(((char *)buffer)+4096*j2+offset2+103));
				memcpy(p_type, (char *)(((char *)buffer)+4096*j2+offset2+99), 5);
				hashtable.emplace(*p_partkey, p_type);

				offset2 += 168;
				if((offset2 + 168) >= 4096)
				{
					offset2 = 0;
					break;
				}
			}
		}
	}
	int offset = 0;
	for(unsigned int i=0; i < pageCount1; i += NUMBER_OF_PAGES_IN_A_COMMAND)
	{
		unsigned int pageInThisIteration = std::min((unsigned int)NUMBER_OF_PAGES_IN_A_COMMAND, (pageCount1-i));

		for(unsigned int j = 0; j < pageInThisIteration; j++)
        {
			while(1)
			{
				memcpy((char *)l_shipdate, (char *)(((char *)buffer)+4096*j+offset+50), 8);

				if(((compareDates(l_shipdate, date, 0) == 1) or (strcmp(l_shipdate, date) == 0)) and ((compareDates(date, l_shipdate, 1) == 1)))
				{
					memcpy((char *)l_partkey, (char *)(((char *)buffer)+4096*j+offset+4), 4);
					memcpy((char *)l_discount, (char *)(((char *)buffer)+4096*j+offset+32), 8);
					memcpy((char *)l_extendedprice, (char *)(((char *)buffer)+4096*j+offset+24), 8); 
					std::unordered_map<unsigned long long int, char *>::iterator it;
					it = hashtable.find(*l_partkey);
					if(it != hashtable.end())
					{
						if(strlen(it->second) >= 5)
						{
							if(strncmp(it->second, "PROMO",5) == 0)
							{
								promo_revenue += *l_extendedprice * (1 - *l_discount);
							}
						}
						revenue += *l_extendedprice * (1 - *l_discount);
					}
				}

				offset += 153;
				if((offset + 153) >= (4096) )
				{
					offset = 0;
					break;
				}
			}
		}
	}

	for ( auto local_it = hashtable.begin(); local_it != hashtable.end(); ++local_it )
		free(local_it->second);
	hashtable.clear();

	free(l_shipdate);
	free(l_discount);
	free(l_extendedprice);
	free(l_quantitiy);
	free(l_partkey);
	free(p_partkey);
}


unsigned int temp_array[5] = {14,14,14,14,14};
char DateBuffer_array[5][20] = {"19950901","19931101","19930801","19971101","19970201"};

int main(int argc, char **argv)
{
	static const char *perrstr;
	unsigned int pageCount1, pageCount2;
	int err, fd;

	pageCount1 = 8;
	pageCount2 = 8;

	if (pageCount1 > 0) {
		cout << "Page count1: " << pageCount1 << endl;
	} else {
		cout << "Page count1 should be bigger than 0" << endl;
		exit(1);
	}
	
	if (pageCount2 > 0) {
		cout << "Page count2: " << pageCount2 << endl;
	} else {
		cout << "Page count2 should be bigger than 0" << endl;
		exit(1);
	}
	n_ssd_proc = 0;

	// DB data should be stored in SSD

	void *buffer;
	
	buffer = calloc(1, REQUEST_SIZE);

	/*Read the values written and process each block by block*/
	/*Things to do: 1) Experiment with various read lengths */


	unsigned int NumberOfQueries = 5;
	char DateBuffer[50];
	double promo_revenue, revenue;

	cout << "Number of queries = " << NumberOfQueries << endl;

	for(unsigned int IterationForQuery = 0; IterationForQuery < NumberOfQueries; IterationForQuery++)
	{
		promo_revenue = 0;
		revenue = 0;

		unsigned int temp = temp_array[IterationForQuery];
        memcpy(DateBuffer, DateBuffer_array[IterationForQuery],20);
		cout << "==============================================================" << endl;

		tpch_query14_baseline(DateBuffer, fd, buffer, pageCount1, pageCount2, promo_revenue, revenue);

		//cout << pageCount << endl;
		cout << pageCount1 << " " << pageCount2 << " " <<  promo_revenue << " " << revenue << " " << (100 * promo_revenue ) / revenue << endl;
		cout << "==============================================================" << endl;
	}
	free(buffer);

return 0;

//perror:
//	perror(perrstr);

return 1;
}
