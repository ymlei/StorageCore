#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include <iostream>
#include <map>

#include <cstdint>
#include <unistd.h>

using namespace std;

// *********************************************
//	Configurations
#define NUMBER_OF_PAGES_IN_A_COMMAND 32
#define REQUEST_SIZE (4096*NUMBER_OF_PAGES_IN_A_COMMAND)
#define LBA_BASE 0x201
// *********************************************

class storage {
	public:
		char l_returnflag;
		char l_linestatus;
		double sum_qty;
		double sum_base_price;
		double sum_disc_price;
		double sum_charge;
		double avg_qty;
		double avg_price;
		double avg_disc;
		double count_order;
};

struct char_cmp {
    bool operator () (const char *a,const char *b) const
    {
//	cout << a << " " << b << " " << (strncmp(a,b,2) == 0) << endl;
        return strncmp(a,b,2) <  0;
    }
};

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

void ReduceDate(char *buffer, unsigned int interval)
{
	if(interval == 60) memcpy(buffer, "19981002", 9);
	else if(interval == 61) memcpy(buffer, "19981001", 9);
	else if((interval >= 62) && (interval <= 91)) {sprintf(buffer, "199809%02u", 91+1-interval);printf("199809%02u\n", 91+1-interval);}
	else if((interval >= 92 && interval <= 120)) {sprintf(buffer, "199808%02u", 31-(interval-92));printf(buffer, "199808%02u\n", 31-(interval-92));}
	return;
}


void tpch_query1_baseline(char *date, int fd, void *buffer, unsigned int pageCount, std::map <std::string, class storage> &mymap)
{
	int err;
	char *l_shipdate = (char *)calloc(9,1);
	double *l_discount = (double *)calloc(1,sizeof(double));
	double *l_quantitiy = (double *)calloc(1,sizeof(double));
	double *l_extendedprice = (double *)calloc(1,sizeof(double));
	double *l_tax = (double *) calloc(1,sizeof(double));
	std::string l_returnflagAndLinestatus;
	l_returnflagAndLinestatus.resize(2);

	std::map<char *, class storage>::iterator it;
	int offset = 0;

	for(unsigned int i=0; i < pageCount; i += NUMBER_OF_PAGES_IN_A_COMMAND)
	{
		unsigned int pageInThisIteration = std::min((unsigned int)NUMBER_OF_PAGES_IN_A_COMMAND, (pageCount-i));
		// copy the file into the buffer:
		//		result = fread (buffer,1,4096,pFile);
		//		if (result != 4096) {break;};

		for(unsigned int j = 0; j < pageInThisIteration; j++)
		{

			while(1)
			{
				memcpy((char *)l_shipdate, (char *)(((char *)buffer)+4096*j+offset+50), 8);
			
				if((compareDates(date, l_shipdate, 0) == 1) or (strcmp(l_shipdate, date) == 0))
				{
					memcpy((char *)l_discount, (char *)(((char *)buffer)+4096*j+offset+32), 8);
					memcpy((char *)l_quantitiy, (char *)(((char *)buffer)+4096*j+offset+16), 8); 
					memcpy((char *)l_extendedprice, (char *)(((char *)buffer)+4096*j+offset+24), 8); 
					memcpy((char *)l_tax, (char *)(((char *)buffer)+4096*j+offset+40), 8);
					memcpy((char *)l_returnflagAndLinestatus.data(), (char *)(((char *)buffer)+4096*j+offset+48), 2);

					if(mymap.find(l_returnflagAndLinestatus) == mymap.end())
					{
						mymap[l_returnflagAndLinestatus];
						mymap[l_returnflagAndLinestatus].l_returnflag = l_returnflagAndLinestatus[0];
						mymap[l_returnflagAndLinestatus].l_linestatus = l_returnflagAndLinestatus[1];
						mymap[l_returnflagAndLinestatus].sum_qty = *l_quantitiy;
						mymap[l_returnflagAndLinestatus].sum_base_price = *l_extendedprice;
						mymap[l_returnflagAndLinestatus].sum_disc_price =  *l_extendedprice * (1 - *l_discount);
						mymap[l_returnflagAndLinestatus].sum_charge = *l_extendedprice * (1 - *l_discount) *(1 + *l_tax);
						mymap[l_returnflagAndLinestatus].avg_qty = *l_quantitiy;
						mymap[l_returnflagAndLinestatus].avg_price = *l_extendedprice;
						mymap[l_returnflagAndLinestatus].avg_disc = *l_discount;
						mymap[l_returnflagAndLinestatus].count_order = 1;
					}
					else {
						mymap[l_returnflagAndLinestatus];
						mymap[l_returnflagAndLinestatus].sum_qty += *l_quantitiy;
						mymap[l_returnflagAndLinestatus].sum_base_price += *l_extendedprice;
						mymap[l_returnflagAndLinestatus].sum_disc_price +=  (*l_extendedprice * (1 - *l_discount));
						mymap[l_returnflagAndLinestatus].sum_charge += (*l_extendedprice * (1 - *l_discount) *(1 + *l_tax));
						mymap[l_returnflagAndLinestatus].avg_qty += (*l_quantitiy);
						mymap[l_returnflagAndLinestatus].avg_price += (*l_extendedprice);
						mymap[l_returnflagAndLinestatus].avg_disc += *l_discount;
						mymap[l_returnflagAndLinestatus].count_order += 1;
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
	
	free(l_shipdate);
	free(l_discount);
	free(l_extendedprice);
	free(l_quantitiy);
	free(l_tax);
//	free(l_returnflagAndLinestatus);
}


unsigned int proc_delay;
unsigned int n_ssd_proc;

unsigned int temp_array[5] = {1,1,1,1,1};
unsigned int interval_array[5] = {90,67,60,68,61};

int main(int argc, char **argv)
{
	unsigned int pageCount;
	int err, fd;

	//pageCount = atoi(argv[1]);
	pageCount =32;
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

	unsigned int NumberOfQueries = 5;

	unsigned int Delta;
	char DateBuffer[50];

	std::map <std::string, class storage > map_groupby;

	cout << "Number of queries = " << NumberOfQueries << endl;

	for(unsigned int IterationForQuery = 0; IterationForQuery < NumberOfQueries; IterationForQuery++)
	{
		unsigned int temp = temp_array[IterationForQuery];
		unsigned int interval = interval_array[IterationForQuery];
		ReduceDate(DateBuffer, interval);
		cout << "==============================================================" << endl;

		tpch_query1_baseline(DateBuffer, fd, buffer, pageCount, map_groupby);

		//cout << pageCount << endl;
		for(std::map <std::string, class storage >::iterator it = map_groupby.begin(); it != map_groupby.end(); it++)
		{
			cout << (it->second).l_returnflag << " " <<  (it->second).l_linestatus << " " <<  (it->second).sum_qty << " " <<  (it->second).sum_base_price << " " <<  (it->second).sum_disc_price << " " <<  (it->second).sum_charge << " " <<  (it->second).avg_qty  /(it->second).count_order << " " <<  (it->second).avg_price  /(it->second).count_order<< " " <<  (it->second).avg_disc /(it->second).count_order  << " " <<  (it->second).count_order << endl;
		}
		cout << "==============================================================" << endl;
		map_groupby.clear();
	}

	free(buffer);

return 0;

//perror:
//	perror(perrstr);

return 1;
}
