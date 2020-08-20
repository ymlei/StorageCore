
#ifndef TOTAL_DATA_SIZE
#define TOTAL_DATA_SIZE 2*1000
#endif

#define ee_printf printf
#define MULTITHREAD 1

/* Configuration : CORE_TICKS
        Define type of return from the timing functions.
 */
#include <time.h>
typedef clock_t CORE_TICKS;

/* Definitions : COMPILER_VERSION, COMPILER_FLAGS, MEM_LOCATION
        Initialize these strings per platform
*/


/* Data Types :
        To avoid compiler issues, define the data types that need ot be used for 8b, 16b and 32b in <core_portme.h>.

        *Imprtant* :
        ee_ptr_int needs to be the data type used to hold pointers, otherwise coremark may fail!!!
*/
typedef signed short ee_s16;
typedef unsigned short ee_u16;
typedef signed int ee_s32;
typedef double ee_f32;
typedef unsigned char ee_u8;
typedef unsigned int ee_u32;
typedef ee_u32 ee_ptr_int;
typedef size_t ee_size_t;

typedef ee_u32 secs_ret;
/* align_mem :
        This macro is used to align an offset to point to a 32b value. It is used in the Matrix algorithm to initialize the input memory blocks.
*/
#define align_mem(x) x

/* Configuration : SEED_METHOD
        Defines method to get seed values that cannot be computed at compile time.

        Valid values :
        SEED_ARG - from command line.
        SEED_FUNC - from a system function.
        SEED_VOLATILE - from volatile variables.
*/
#ifndef SEED_METHOD
#define SEED_METHOD SEED_VOLATILE
#endif
/* Variable : default_num_contexts
        Not used for this simple port, must cintain the value 1.
*/
extern ee_u32 default_num_contexts;

typedef struct CORE_PORTABLE_S {
        ee_u8   portable_id;
} core_portable;

int main(int argc, char *argv[]);

/* Algorithm IDS */
#define ID_LIST         (1<<0)

#define NUM_ALGORITHMS 1

/* list data structures */
typedef struct list_data_s {
        ee_s16 data16;
        ee_s16 idx;
} list_data;

typedef struct list_head_s {
        struct list_head_s *next;
        struct list_data_s *info;
} list_head;


/* Helper structure to hold results */
typedef struct RESULTS_S {
        /* inputs */
        ee_s16  seed1;          /* Initializing seed */
        ee_s16  seed2;          /* Initializing seed */
        ee_s16  seed3;          /* Initializing seed */
        void    *memblock[4];   /* Pointer to safe memory location */
        ee_u32  size;           /* Size of the data */
        ee_u32 iterations;              /* Number of iterations to execute */
        ee_u32  execs;          /* Bitmask of operations to execute */
        struct list_head_s *list;
        //mat_params mat;
        /* outputs */
        ee_u16  crc;
        ee_u16  crclist;
        ee_u16  crcmatrix;
        ee_u16  crcstate;
        ee_s16  err;
        /* ultithread specific */
        core_portable port;
} core_results;

/* Function: crc*
        Service functions to calculate 16b CRC code.

*/
ee_u16 crcu8(ee_u8 data, ee_u16 crc )
{
        ee_u8 i=0,x16=0,carry=0;

        for (i = 0; i < 8; i++)
    {
                x16 = (ee_u8)((data & 1) ^ ((ee_u8)crc & 1));
                data >>= 1;

                if (x16 == 1)
                {
                   crc ^= 0x4002;
                   carry = 1;
                }
                else
                        carry = 0;
                crc >>= 1;
                if (carry)
                   crc |= 0x8000;
                else
                   crc &= 0x7fff;
    }
        return crc;
}
ee_u16 crcu16(ee_u16 newval, ee_u16 crc) {
        crc=crcu8( (ee_u8) (newval)                             ,crc);
        crc=crcu8( (ee_u8) ((newval)>>8)        ,crc);
        return crc;
}
ee_u16 crc16(ee_s16 newval, ee_u16 crc) {
        return crcu16((ee_u16)newval, crc);
}
ee_u16 crcu32(ee_u32 newval, ee_u16 crc) {
        crc=crc16((ee_s16) newval               ,crc);
        crc=crc16((ee_s16) (newval>>16) ,crc);
        return crc;
}

/* list benchmark functions */
list_head *core_list_init(ee_u32 blksize, list_head *memblock, ee_s16 seed);
ee_u16 core_bench_list(core_results *res, ee_s16 finder_idx);

/*
Topic: Description
        Benchmark using a linked list.

        Linked list is a common data structure used in many applications.

        For our purposes, this will excercise the memory units of the processor.
        In particular, usage of the list pointers to find and alter data.

        We are not using Malloc since some platforms do not support this library.

        Instead, the memory block being passed in is used to create a list,
        and the benchmark takes care not to add more items then can be
        accomodated by the memory block. The porting layer will make sure
        that we have a valid memory block.

        All operations are done in place, without using any extra memory.

        The list itself contains list pointers and pointers to data items.
        Data items contain the following:

        idx - An index that captures the initial order of the list.
        data - Variable data initialized based on the input parameters. The 16b are divided as follows:
        o Upper 8b are backup of original data.
        o Bit 7 indicates if the lower 7 bits are to be used as is or calculated.
        o Bits 0-2 indicate type of operation to perform to get a 7b value.
        o Bits 3-6 provide input for the operation.

*/

/* local functions */

list_head *core_list_find(list_head *list,list_data *info);
list_head *core_list_reverse(list_head *list);
list_head *core_list_remove(list_head *item);
list_head *core_list_undo_remove(list_head *item_removed, list_head *item_modified);
list_head *core_list_insert_new(list_head *insert_point
        , list_data *info, list_head **memblock, list_data **datablock
        , list_head *memblock_end, list_data *datablock_end);
typedef ee_s32(*list_cmp)(list_data *a, list_data *b, core_results *res);
list_head *core_list_mergesort(list_head *list, list_cmp cmp, core_results *res);

ee_s16 calc_func(ee_s16 *pdata, core_results *res) {
        ee_s16 data=*pdata;
        ee_s16 retval;
        ee_u8 optype=(data>>7) & 1; /* bit 7 indicates if the function result has been cached */
        if (optype) /* if cached, use cache */
                return (data & 0x007f);
        else { /* otherwise calculate and cache the result */
                ee_s16 flag=data & 0x7; /* bits 0-2 is type of function to perform */
                ee_s16 dtype=((data>>3) & 0xf); /* bits 3-6 is specific data for the operation */
                dtype |= dtype << 4; /* replicate the lower 4 bits to get an 8b value */

                //linked list
                retval=data;

                res->crc=crcu16(retval,res->crc);
                retval &= 0x007f;
                *pdata = (data & 0xff00) | 0x0080 | retval; /* cache the result */
                return retval;
        }
}
/* Function: cmp_complex
        Compare the data item in a list cell.

        Can be used by mergesort.
*/
ee_s32 cmp_complex(list_data *a, list_data *b, core_results *res) {
        ee_s16 val1=calc_func(&(a->data16),res);
        ee_s16 val2=calc_func(&(b->data16),res);
        return val1 - val2;
}

/* Function: cmp_idx
        Compare the idx item in a list cell, and regen the data.

        Can be used by mergesort.
*/
ee_s32 cmp_idx(list_data *a, list_data *b, core_results *res) {
        if (res==NULL) {
                a->data16 = (a->data16 & 0xff00) | (0x00ff & (a->data16>>8));
                b->data16 = (b->data16 & 0xff00) | (0x00ff & (b->data16>>8));
        }
        return a->idx - b->idx;
}

void copy_info(list_data *to,list_data *from) {
        to->data16=from->data16;
        to->idx=from->idx;
}

/* Benchmark for linked list:
        - Try to find multiple data items.
        - List sort
        - Operate on data from list (crc)
        - Single remove/reinsert
        * At the end of this function, the list is back to original state
*/
ee_u16 core_bench_list(core_results *res, ee_s16 finder_idx) {
        ee_u16 retval=0;
        ee_u16 found=0,missed=0;
        list_head *list=res->list;
        ee_s16 find_num=res->seed3;
        list_head *this_find;
        list_head *finder, *remover;
        list_data info;
        ee_s16 i;

        info.idx=finder_idx;
        /* find <find_num> values in the list, and change the list each time (reverse and cache if value found) */
        for (i=0; i<find_num; i++) {
                info.data16= (i & 0xff) ;
                this_find=core_list_find(list,&info);
                list=core_list_reverse(list);
                if (this_find==NULL) {
                        missed++;
                        retval+=(list->next->info->data16 >> 8) & 1;
                }
                else {
                        found++;
                        if (this_find->info->data16 & 0x1) /* use found value */
                                retval+=(this_find->info->data16 >> 9) & 1;
                        /* and cache next item at the head of the list (if any) */
                        if (this_find->next != NULL) {
                                finder = this_find->next;
                                this_find->next = finder->next;
                                finder->next=list->next;
                                list->next=finder;
                        }
                }
                if (info.idx>=0)
                        info.idx++;
#if CORE_DEBUG
        ee_printf("List find %d: [%d,%d,%d]\n",i,retval,missed,found);
#endif
        }
        retval+=found*4-missed;
        /* sort the list by data content and remove one item*/
        if (finder_idx>0)
                list=core_list_mergesort(list,cmp_complex,res);
        remover=core_list_remove(list->next);
        /* CRC data content of list from location of index N forward, and then undo remove */
        finder=core_list_find(list,&info);
        if (!finder)
                finder=list->next;
        while (finder) {
                retval=crc16(list->info->data16,retval);
                finder=finder->next;
        }
#if CORE_DEBUG
        ee_printf("List sort 1: %04x\n",retval);
#endif
        remover=core_list_undo_remove(remover,list->next);
        /* sort the list by index, in effect returning the list to original state */
        list=core_list_mergesort(list,cmp_idx,NULL);
        /* CRC data content of list */
        finder=list->next;
        while (finder) {
                retval=crc16(list->info->data16,retval);
                finder=finder->next;
        }
#if CORE_DEBUG
        ee_printf("List sort 2: %04x\n",retval);
#endif
        return retval;
}
/* Function: core_list_init
        Initialize list with data.

        Parameters:
        blksize - Size of memory to be initialized.
        memblock - Pointer to memory block.
        seed -  Actual values chosen depend on the seed parameter.
                The seed parameter MUST be supplied from a source that cannot be determined at compile time

        Returns:
        Pointer to the head of the list.

*/
list_head *core_list_init(ee_u32 blksize, list_head *memblock, ee_s16 seed) {
        /* calculated pointers for the list */
        ee_u32 per_item=16+sizeof(struct list_data_s);
        ee_u32 size=(blksize/per_item)-2; /* to accomodate systems with 64b pointers, and make sure same code is executed, set max list elements */
        list_head *memblock_end=memblock+size;
        list_data *datablock=(list_data *)(memblock_end);
        list_data *datablock_end=datablock+size;
        /* some useful variables */
        ee_u32 i;
        list_head *finder,*list=memblock;
        list_data info;

        /* create a fake items for the list head and tail */
        list->next=NULL;
        list->info=datablock;
        list->info->idx=0x0000;
        list->info->data16=(ee_s16)0x8080;
        memblock++;
        datablock++;
        info.idx=0x7fff;
        info.data16=(ee_s16)0xffff;
        core_list_insert_new(list,&info,&memblock,&datablock,memblock_end,datablock_end);

        /* then insert size items */
        for (i=0; i<size; i++) {
                ee_u16 datpat=((ee_u16)(seed^i) & 0xf);
                ee_u16 dat=(datpat<<3) | (i&0x7); /* alternate between algorithms */
                info.data16=(dat<<8) | dat;             /* fill the data with actual data and upper bits with rebuild value */
                core_list_insert_new(list,&info,&memblock,&datablock,memblock_end,datablock_end);
        }
        /* and now index the list so we know initial seed order of the list */
        finder=list->next;
        i=1;
        while (finder->next!=NULL) {
                if (i<size/5) /* first 20% of the list in order */
                        finder->info->idx=i++;
                else {
                        ee_u16 pat=(ee_u16)(i++ ^ seed); /* get a pseudo random number */
                        finder->info->idx=0x3fff & (((i & 0x07) << 8) | pat); /* make sure the mixed items end up after the ones in sequence */
                }
                finder=finder->next;
        }
        list = core_list_mergesort(list,cmp_idx,NULL);
#if CORE_DEBUG
        ee_printf("Initialized list:\n");
        finder=list;
        while (finder) {
                ee_printf("[%04x,%04x]",finder->info->idx,(ee_u16)finder->info->data16);
                finder=finder->next;
        }
        ee_printf("\n");
#endif
        return list;
}

/* Function: core_list_insert
        Insert an item to the list

        Parameters:
        insert_point - where to insert the item.
        info - data for the cell.
        memblock - pointer for the list header
        datablock - pointer for the list data
        memblock_end - end of region for list headers
        datablock_end - end of region for list data

        Returns:
        Pointer to new item.
*/
list_head *core_list_insert_new(list_head *insert_point, list_data *info, list_head **memblock, list_data **datablock
        , list_head *memblock_end, list_data *datablock_end) {
        list_head *newitem;

        if ((*memblock+1) >= memblock_end)
                return NULL;
        if ((*datablock+1) >= datablock_end)
                return NULL;

        newitem=*memblock;
        (*memblock)++;
        newitem->next=insert_point->next;
        insert_point->next=newitem;

        newitem->info=*datablock;
        (*datablock)++;
        copy_info(newitem->info,info);

        return newitem;
}

/* Function: core_list_remove
        Remove an item from the list.

        Operation:
        For a singly linked list, remove by copying the data from the next item
        over to the current cell, and unlinking the next item.

        Note:
        since there is always a fake item at the end of the list, no need to check for NULL.

        Returns:
        Removed item.
*/
list_head *core_list_remove(list_head *item) {
        list_data *tmp;
        list_head *ret=item->next;
        /* swap data pointers */
        tmp=item->info;
        item->info=ret->info;
        ret->info=tmp;
        /* and eliminate item */
        item->next=item->next->next;
        ret->next=NULL;
        return ret;
}

/* Function: core_list_undo_remove
        Undo a remove operation.

        Operation:
        Since we want each iteration of the benchmark to be exactly the same,
        we need to be able to undo a remove.
        Link the removed item back into the list, and switch the info items.

        Parameters:
        item_removed - Return value from the <core_list_remove>
        item_modified - List item that was modified during <core_list_remove>

        Returns:
        The item that was linked back to the list.

*/
list_head *core_list_undo_remove(list_head *item_removed, list_head *item_modified) {
        list_data *tmp;
        /* swap data pointers */
        tmp=item_removed->info;
        item_removed->info=item_modified->info;
        item_modified->info=tmp;
        /* and insert item */
        item_removed->next=item_modified->next;
        item_modified->next=item_removed;
        return item_removed;
}

/* Function: core_list_find
        Find an item in the list

        Operation:
        Find an item by idx (if not 0) or specific data value

        Parameters:
        list - list head
        info - idx or data to find

        Returns:
        Found item, or NULL if not found.
*/
list_head *core_list_find(list_head *list,list_data *info) {
        if (info->idx>=0) {
                while (list && (list->info->idx != info->idx))
                        list=list->next;
                return list;
        } else {
                while (list && ((list->info->data16 & 0xff) != info->data16))
                        list=list->next;
                return list;
        }
}
/* Function: core_list_reverse
        Reverse a list

        Operation:
        Rearrange the pointers so the list is reversed.

        Parameters:
        list - list head
        info - idx or data to find

        Returns:
        Found item, or NULL if not found.
*/

list_head *core_list_reverse(list_head *list) {
        list_head *next=NULL, *tmp;
        while (list) {
                tmp=list->next;
                list->next=next;
                next=list;
                list=tmp;
        }
        return next;
}
/* Function: core_list_mergesort
        Sort the list in place without recursion.

        Description:
        Use mergesort, as for linked list this is a realistic solution.
        Also, since this is aimed at embedded, care was taken to use iterative rather then recursive algorithm.
        The sort can either return the list to original order (by idx) ,
        or use the data item to invoke other other algorithms and change the order of the list.

        Parameters:
        list - list to be sorted.
        cmp - cmp function to use

        Returns:
        New head of the list.

        Note:
        We have a special header for the list that will always be first,
        but the algorithm could theoretically modify where the list starts.

 */
list_head *core_list_mergesort(list_head *list, list_cmp cmp, core_results *res) {
    list_head *p, *q, *e, *tail;
    ee_s32 insize, nmerges, psize, qsize, i;

    insize = 1;

    while (1) {
        p = list;
        list = NULL;
        tail = NULL;

        nmerges = 0;  /* count number of merges we do in this pass */

        while (p) {
            nmerges++;  /* there exists a merge to be done */
            /* step `insize' places along from p */
            q = p;
            psize = 0;
            for (i = 0; i < insize; i++) {
                psize++;
                            q = q->next;
                if (!q) break;
            }

            /* if q hasn't fallen off end, we have two lists to merge */
            qsize = insize;

            /* now we have two lists; merge them */
            while (psize > 0 || (qsize > 0 && q)) {

                                /* decide whether next element of merge comes from p or q */
                                if (psize == 0) {
                                    /* p is empty; e must come from q. */
                                    e = q; q = q->next; qsize--;
                                } else if (qsize == 0 || !q) {
                                    /* q is empty; e must come from p. */
                                    e = p; p = p->next; psize--;
                                } else if (cmp(p->info,q->info,res) <= 0) {
                                    /* First element of p is lower (or same); e must come from p. */
                                    e = p; p = p->next; psize--;
                                } else {
                                    /* First element of q is lower; e must come from q. */
                                    e = q; q = q->next; qsize--;
                                }

                        /* add the next element to the merged list */
                                if (tail) {
                                    tail->next = e;
                                } else {
                                    list = e;
                                }
                                tail = e;
                }

                        /* now p has stepped `insize' places along, and q has too */
                        p = q;
        }

            tail->next = NULL;

        /* If we have done only one merge, we're finished. */
        if (nmerges <= 1)   /* allow for nmerges==0, the empty list case */
            return list;

        /* Otherwise repeat, merging lists twice the size */
        insize *= 2;
    }
#if COMPILER_REQUIRES_SORT_RETURN
        return list;
#endif
}
/*
Author : Shay Gal-On, EEMBC

This file is part of  EEMBC(R) and CoreMark(TM), which are Copyright (C) 2009
All rights reserved.

EEMBC CoreMark Software is a product of EEMBC and is provided under the terms of the
CoreMark License that is distributed with the official EEMBC COREMARK Software release.
If you received this EEMBC CoreMark Software without the accompanying CoreMark License,
you must discontinue use and download the official release from www.coremark.org.

Also, if you are publicly displaying scores generated from the EEMBC CoreMark software,
make sure that you are in compliance with Run and Reporting rules specified in the accompanying readme.txt file.

EEMBC
4354 Town Center Blvd. Suite 114-200
El Dorado Hills, CA, 95762
*/
/* File: core_main.c
        This file contains the framework to acquire a block of memory, seed initial parameters, tun t he benchmark and report the results.
*/
//#include "coremark.h"

/* Function: iterate
        Run the benchmark for a specified number of iterations.

        Operation:
        For each type of benchmarked algorithm:
                a - Initialize the data block for the algorithm.
                b - Execute the algorithm N times.

        Returns:
        NULL.
*/
static ee_u16 list_known_crc[]   =      {(ee_u16)0xd4b0,(ee_u16)0x3340,(ee_u16)0x6a79,(ee_u16)0xe714,(ee_u16)0xe3c1};
static ee_u16 matrix_known_crc[] =      {(ee_u16)0xbe52,(ee_u16)0x1199,(ee_u16)0x5608,(ee_u16)0x1fd7,(ee_u16)0x0747};
static ee_u16 state_known_crc[]  =      {(ee_u16)0x5e47,(ee_u16)0x39bf,(ee_u16)0xe5a4,(ee_u16)0x8e3a,(ee_u16)0x8d84};
void *iterate(void *pres) {
        ee_u32 i;
        ee_u16 crc;
        core_results *res=(core_results *)pres;
        ee_u32 iterations=res->iterations;
        res->crc=0;
        res->crclist=0;
        res->crcmatrix=0;
        res->crcstate=0;

        for (i=0; i<iterations; i++) {
                crc=core_bench_list(res,1);
                res->crc=crcu16(crc,res->crc);
                crc=core_bench_list(res,-1);
                res->crc=crcu16(crc,res->crc);
                if (i==0) res->crclist=res->crc;
        }
        return NULL;
}

#if (SEED_METHOD==SEED_ARG)
ee_s32 get_seed_args(int i, int argc, char *argv[]);
#define get_seed(x) (ee_s16)get_seed_args(x,argc,argv)
#define get_seed_32(x) get_seed_args(x,argc,argv)
#else /* via function or volatile */
ee_s32 get_seed_32(int i);
#define get_seed(x) (ee_s16)get_seed_32(x)
#endif

#if (MEM_METHOD==MEM_STATIC)
ee_u8 static_memblk[TOTAL_DATA_SIZE];
#endif
char *mem_name[3] = {"Static","Heap","Stack"};
/* Function: main
        Main entry routine for the benchmark.
        This function is responsible for the following steps:

        1 - Initialize input seeds from a source that cannot be determined at compile time.
        2 - Initialize memory block for use.
        3 - Run and time the benchmark.
        4 - Report results, testing the validity of the output if the seeds are known.

        Arguments:
        1 - first seed  : Any value
        2 - second seed : Must be identical to first for iterations to be identical
        3 - third seed  : Any value, should be at least an order of magnitude less then the input size, but bigger then 32.
        4 - Iterations  : Special, if set to 0, iterations will be automatically determined such that the benchmark will run between 10 to 100 secs

*/

/*
Author : Shay Gal-On, EEMBC

This file is part of  EEMBC(R) and CoreMark(TM), which are Copyright (C) 2009
All rights reserved.

EEMBC CoreMark Software is a product of EEMBC and is provided under the terms of the
CoreMark License that is distributed with the official EEMBC COREMARK Software release.
If you received this EEMBC CoreMark Software without the accompanying CoreMark License,
you must discontinue use and download the official release from www.coremark.org.

Also, if you are publicly displaying scores generated from the EEMBC CoreMark software,
make sure that you are in compliance with Run and Reporting rules specified in the accompanying readme.txt file.

EEMBC
4354 Town Center Blvd. Suite 114-200
El Dorado Hills, CA, 95762
*/
//#include "coremark.h"
/* Function: get_seed
        Get a values that cannot be determined at compile time.

        Since different embedded systems and compilers are used, 3 different methods are provided:
        1 - Using a volatile variable. This method is only valid if the compiler is forced to generate code that
        reads the value of a volatile variable from memory at run time.
        Please note, if using this method, you would need to modify core_portme.c to generate training profile.
        2 - Command line arguments. This is the preferred method if command line arguments are supported.
        3 - System function. If none of the first 2 methods is available on the platform,
        a system function which is not a stub can be used.

        e.g. read the value on GPIO pins connected to switches, or invoke special simulator functions.
*/
#if (SEED_METHOD==SEED_VOLATILE)
        extern volatile ee_s32 seed1_volatile;
        extern volatile ee_s32 seed2_volatile;
        extern volatile ee_s32 seed3_volatile;
        extern volatile ee_s32 seed4_volatile;
        extern volatile ee_s32 seed5_volatile;
        ee_s32 get_seed_32(int i) {
                ee_s32 retval;
                switch (i) {
                        case 1:
                                retval=seed1_volatile;
                                break;
                        case 2:
                                retval=seed2_volatile;
                                break;
                        case 3:
                                retval=seed3_volatile;
                                break;
                        case 4:
                                retval=seed4_volatile;
                                break;
                        case 5:
                                retval=seed5_volatile;
                                break;
                        default:
                                retval=0;
                                break;
                }
                return retval;
        }
#elif (SEED_METHOD==SEED_ARG)
ee_s32 parseval(char *valstring) {
        ee_s32 retval=0;
        ee_s32 neg=1;
        int hexmode=0;
        if (*valstring == '-') {
                neg=-1;
                valstring++;
        }
        if ((valstring[0] == '0') && (valstring[1] == 'x')) {
                hexmode=1;
                valstring+=2;
        }
                /* first look for digits */
        if (hexmode) {
                while (((*valstring >= '0') && (*valstring <= '9')) || ((*valstring >= 'a') && (*valstring <= 'f'))) {
                        ee_s32 digit=*valstring-'0';
                        if (digit>9)
                                digit=10+*valstring-'a';
                        retval*=16;
                        retval+=digit;
                        valstring++;
                }
        } else {
                while ((*valstring >= '0') && (*valstring <= '9')) {
                        ee_s32 digit=*valstring-'0';
                        retval*=10;
                        retval+=digit;
                        valstring++;
                }
        }
        /* now add qualifiers */
        if (*valstring=='K')
                retval*=1024;
        if (*valstring=='M')
                retval*=1024*1024;

        retval*=neg;
        return retval;
}

ee_s32 get_seed_args(int i, int argc, char *argv[]) {
        if (argc>i)
                return parseval(argv[i]);
        return 0;
}

#elif (SEED_METHOD==SEED_FUNC)
/* If using OS based function, you must define and implement the functions below in core_portme.h and core_portme.c ! */
ee_s32 get_seed_32(int i) {
        ee_s32 retval;
        switch (i) {
                case 1:
                        retval=portme_sys1();
                        break;
                case 2:
                        retval=portme_sys2();
                        break;
                case 3:
                        retval=portme_sys3();
                        break;
                case 4:
                        retval=portme_sys4();
                        break;
                case 5:
                        retval=portme_sys5();
                        break;
                default:
                        retval=0;
                        break;
        }
        return retval;
}
#endif


ee_u8 check_data_types() {
        ee_u8 retval=0;
        if (sizeof(ee_u8) != 1) {
                ee_printf("ERROR: ee_u8 is not an 8b datatype!\n");
                retval++;
        }
        if (sizeof(ee_u16) != 2) {
                ee_printf("ERROR: ee_u16 is not a 16b datatype!\n");
                retval++;
        }
        if (sizeof(ee_s16) != 2) {
                ee_printf("ERROR: ee_s16 is not a 16b datatype!\n");
                retval++;
        }
        if (sizeof(ee_s32) != 4) {
                ee_printf("ERROR: ee_s32 is not a 32b datatype!\n");
                retval++;
        }
        if (sizeof(ee_u32) != 4) {
                ee_printf("ERROR: ee_u32 is not a 32b datatype!\n");
                retval++;
        }
        if (sizeof(ee_ptr_int) != sizeof(int *)) {
                ee_printf("ERROR: ee_ptr_int is not a datatype that holds an int pointer!\n");
                retval++;
        }
        if (retval>0) {
                ee_printf("ERROR: Please modify the datatypes in core_portme.h!\n");
        }
        return retval;
}
/*
        File : core_portme.c
*/
/*
        Author : Shay Gal-On, EEMBC
        Legal : TODO!
*/
#include <stdio.h>
#include <stdlib.h>
//#include "coremark.h"

#if VALIDATION_RUN
        volatile ee_s32 seed1_volatile=0x3415;
        volatile ee_s32 seed2_volatile=0x3415;
        volatile ee_s32 seed3_volatile=0x66;
#endif
#if PERFORMANCE_RUN
        volatile ee_s32 seed1_volatile=0x0;
        volatile ee_s32 seed2_volatile=0x0;
        volatile ee_s32 seed3_volatile=0x66;
#endif
#if PROFILE_RUN
        volatile ee_s32 seed1_volatile=0x8;
        volatile ee_s32 seed2_volatile=0x8;
        volatile ee_s32 seed3_volatile=0x8;
#endif
        volatile ee_s32 seed4_volatile=0;
        volatile ee_s32 seed5_volatile=0;


/* Porting : Timing functions
        How to capture time and convert to seconds must be ported to whatever is supported by the platform.
        e.g. Read value from on board RTC, read value from cpu clock cycles performance counter etc.
        Sample implementation for standard time.h and windows.h definitions included.
*/
/* Define : TIMER_RES_DIVIDER
        Divider to trade off timer resolution and total time that can be measured.

        Use lower values to increase resolution, but make sure that overflow does not occur.
        If there are issues with the return value overflowing, increase this value.
        */
//#define NSECS_PER_SEC CLOCKS_PER_SEC
#define NSECS_PER_SEC 1000000000
#define CORETIMETYPE clock_t
//#define GETMYTIME(_t) (*_t=clock())
#define GETMYTIME(_t) (*_t=0)
#define MYTIMEDIFF(fin,ini) ((fin)-(ini))
#define TIMER_RES_DIVIDER 1
#define SAMPLE_TIME_IMPLEMENTATION 1
//#define EE_TICKS_PER_SEC (NSECS_PER_SEC / TIMER_RES_DIVIDER)

#define EE_TICKS_PER_SEC 1000

/** Define Host specific (POSIX), or target specific global time variables. */
static CORETIMETYPE start_time_val, stop_time_val;

/* Function : start_time
        This function will be called right before starting the timed portion of the benchmark.

        Implementation may be capturing a system timer (as implemented in the example code)
        or zeroing some system parameters - e.g. setting the cpu clocks cycles to 0.
*/
void start_time(void) {
uint32_t mcyclel;
        asm volatile ("csrr %0,mcycle"  : "=r" (mcyclel) );
        start_time_val = mcyclel;
}
/* Function : stop_time
        This function will be called right after ending the timed portion of the benchmark.

        Implementation may be capturing a system timer (as implemented in the example code)
        or other system parameters - e.g. reading the current value of cpu cycles counter.
*/
void stop_time(void) {
uint32_t mcyclel;
        asm volatile ("csrr %0,mcycle"  : "=r" (mcyclel) );
        stop_time_val = mcyclel;
}
/* Function : get_time
        Return an abstract "ticks" number that signifies time on the system.

        Actual value returned may be cpu cycles, milliseconds or any other value,
        as long as it can be converted to seconds by <time_in_secs>.
        This methodology is taken to accomodate any hardware or simulated platform.
        The sample implementation returns millisecs by default,
        and the resolution is controlled by <TIMER_RES_DIVIDER>
*/
CORE_TICKS get_time(void) {
        CORE_TICKS elapsed=(CORE_TICKS)(MYTIMEDIFF(stop_time_val, start_time_val));
        return elapsed;
}
/* Function : time_in_secs
        Convert the value returned by get_time to seconds.

        The <secs_ret> type is used to accomodate systems with no support for floating point.
        Default implementation implemented by the EE_TICKS_PER_SEC macro above.
*/
secs_ret time_in_secs(CORE_TICKS ticks) {
        secs_ret retval=((secs_ret)ticks) / (secs_ret)EE_TICKS_PER_SEC;
        return retval;
}

ee_u32 default_num_contexts=1;

/* Function : portable_init
        Target specific initialization code
        Test for some common mistakes.
*/
void portable_init(core_portable *p, int *argc, char *argv[])
{
        if (sizeof(ee_ptr_int) != sizeof(ee_u8 *)) {
                ee_printf("ERROR! Please define ee_ptr_int to a type that holds a pointer!\n");
        }
        if (sizeof(ee_u32) != 4) {
                ee_printf("ERROR! Please define ee_u32 to a 32b unsigned type!\n");
        }
        p->portable_id=1;
}
/* Function : portable_fini
        Target specific final code
*/
void portable_fini(core_portable *p)
{
        p->portable_id=0;
}


#include <stdarg.h>


int main(int argc, char *argv[]) {

        ee_u16 i,j=0,num_algorithms=0;
        ee_s16 known_id=-1,total_errors=0;
        ee_u16 seedcrc=0;
        CORE_TICKS total_time;
        core_results results[1];

        ee_u8 stack_memblock[TOTAL_DATA_SIZE];

        /* first call any initializations needed */
        portable_init(&(results[0].port), &argc, argv);
        /* First some checks to make sure benchmark will run ok */
        if (sizeof(struct list_head_s)>128) {
                ee_printf("list_head structure too big for comparable data!\n");
                return 0;
        }
        results[0].seed1=get_seed(1);
        results[0].seed2=get_seed(2);
        results[0].seed3=get_seed(3);
        results[0].iterations=get_seed_32(4);
#if CORE_DEBUG
        results[0].iterations=1;
#endif
        results[0].execs=ID_LIST;

                /* put in some default values based on one seed only for easy testing */
        if ((results[0].seed1==0) && (results[0].seed2==0) && (results[0].seed3==0)) { /* validation run */
                results[0].seed1=0;
                results[0].seed2=0;
                results[0].seed3=0x66;
        }
        if ((results[0].seed1==1) && (results[0].seed2==0) && (results[0].seed3==0)) { /* perfromance run */
                results[0].seed1=0x3415;
                results[0].seed2=0x3415;
                results[0].seed3=0x66;
        }

        results[0].memblock[0]=(void *)static_memblk;
        results[0].size=TOTAL_DATA_SIZE;
        results[0].err=0;

        /* Data init */
        /* Find out how space much we have based on number of algorithms */
        for (i=0; i<NUM_ALGORITHMS; i++) {
                if ((1<<(ee_u32)i) & results[0].execs)
                        num_algorithms++;
        }
        for (i=0 ; i<MULTITHREAD; i++)
                results[i].size=results[i].size/num_algorithms;
        /* Assign pointers */
        for (i=0; i<NUM_ALGORITHMS; i++) {
                ee_u32 ctx;
                if ((1<<(ee_u32)i) & results[0].execs) {
                        for (ctx=0 ; ctx<MULTITHREAD; ctx++)
                                results[ctx].memblock[i+1]=(char *)(results[ctx].memblock[0])+results[0].size*j;
                        j++;
                }
        }
        /* call inits */
        for (i=0 ; i<MULTITHREAD; i++) {
                if (results[i].execs & ID_LIST) {
                        results[i].list=core_list_init(results[0].size,results[i].memblock[1],results[i].seed1);
                }
        }

        /* automatically determine number of iterations if not set */
        if (results[0].iterations==0) {
                secs_ret secs_passed=0;
                ee_u32 divisor;
                results[0].iterations=1;
                while (secs_passed < (secs_ret)1) {
                        results[0].iterations*=10;
                        start_time();
                        iterate(&results[0]);
                        stop_time();
                        secs_passed=time_in_secs(get_time());
                }
                /* now we know it executes for at least 1 sec, set actual run time at about 10 secs */
                divisor=(ee_u32)secs_passed;
                if (divisor==0) /* some machines cast float to int as 0 since this conversion is not defined by ANSI, but we know at least one second passed */
                        divisor=1;
                results[0].iterations*=1+10/divisor;
        }
        /* perform actual benchmark */
        start_time();

        __asm("__perf_start:");

        iterate(&results[0]);

        __asm("__perf_end:");

        stop_time();
        total_time=get_time();

        /* and report results */
        ee_printf("CoreMark Size    : %u\n",(ee_u32)results[0].size);
        ee_printf("Total ticks      : %u\n",(ee_u32)total_time);

        ee_printf("Total time (secs): %d\n",time_in_secs(total_time));
        if (time_in_secs(total_time) > 0)
//              ee_printf("Iterations/Sec   : %d\n",default_num_contexts*results[0].iterations/time_in_secs(total_time));
                ee_printf("Iterat/Sec/MHz   : %d.%d\n",1000*default_num_contexts*results[0].iterations/time_in_secs(total_time),
                             100000*default_num_contexts*results[0].iterations/time_in_secs(total_time) % 100);
        if (time_in_secs(total_time) < 10) {
                ee_printf("ERROR! Must execute for at least 10 secs for a valid result!\n");
                total_errors++;
        }

        ee_printf("Iterations       : %u\n",(ee_u32)default_num_contexts*results[0].iterations);

        /* output for verification */
        ee_printf("seedcrc          : 0x%04x\n",seedcrc);
        if (results[0].execs & ID_LIST)
                for (i=0 ; i<default_num_contexts; i++)
                        ee_printf("[%d]crclist       : 0x%04x\n",i,results[i].crclist);
        for (i=0 ; i<default_num_contexts; i++)
                ee_printf("[%d]crcfinal      : 0x%04x\n",i,results[i].crc);
        if (total_errors==0) {
                ee_printf("Correct operation validated. See readme.txt for run and reporting rules.\n");

                        ee_printf("\n");
                }
        return 0;
}      