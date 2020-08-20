#include<stdio.h>

/* Configuration: TOTAL_DATA_SIZE
        Define total size for data algorithms will operate on
*/
#ifndef TOTAL_DATA_SIZE
#define TOTAL_DATA_SIZE 2*1000
#endif

/* Configuration : HAS_PRINTF
        Define to 1 if the platform has stdio.h and implements the printf function.
*/
#define ee_printf printf

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

/* align_mem :
        This macro is used to align an offset to point to a 32b value. It is used in the Matrix algorithm to initialize the input memory blocks.
*/
#define align_mem(x) x

ee_u8 static_memblk[TOTAL_DATA_SIZE];

/* state machine related stuff */
/* List of all the possible states for the FSM */
typedef enum CORE_STATE {
        CORE_START=0,
        CORE_INVALID,
        CORE_S1,
        CORE_S2,
        CORE_INT,
        CORE_FLOAT,
        CORE_EXPONENT,
        CORE_SCIENTIFIC,
        NUM_CORE_STATES
} core_state_e ;

enum CORE_STATE core_state_transition( ee_u8 **instr , ee_u32 *transition_count);



/* Function: core_bench_state
        Benchmark function

        Go over the input twice, once direct, and once after introducing some corruption.
*/
void core_bench_state(ee_u32 blksize, ee_u8 *memblock,
                ee_s16 seed1, ee_s16 seed2, ee_s16 step)
{
        ee_u32 final_counts[NUM_CORE_STATES];
        ee_u32 track_counts[NUM_CORE_STATES];
        ee_u8 *p=memblock;
        ee_u32 i;


#if CORE_DEBUG
        ee_printf("State Bench: %d,%d,%d,%04x\n",seed1,seed2,step);
#endif
        for (i=0; i<NUM_CORE_STATES; i++) {
                final_counts[i]=track_counts[i]=0;
        }
        /* run the state machine over the input */
        while (*p!=0) {
                enum CORE_STATE fstate=core_state_transition(&p,track_counts);
                final_counts[fstate]++;
#if CORE_DEBUG
        ee_printf("%d,",fstate);
        }
        ee_printf("\n");
#else
        }
#endif
        p=memblock;
        while (p < (memblock+blksize)) { /* insert some corruption */
                if (*p!=',')
                        *p^=(ee_u8)seed1;
                p+=step;
        }
        p=memblock;
        /* run the state machine over the input again */
        while (*p!=0) {
                enum CORE_STATE fstate=core_state_transition(&p,track_counts);
                final_counts[fstate]++;
#if CORE_DEBUG
        ee_printf("%d,",fstate);
        }
        ee_printf("\n");
#else
        }
#endif
        p=memblock;
        while (p < (memblock+blksize)) { /* undo corruption is seed1 and seed2 are equal */
                if (*p!=',')
                        *p^=(ee_u8)seed2;
                p+=step;
        }
}

/* Default initialization patterns */
static ee_u8 *intpat[4]  ={(ee_u8 *)"5012",(ee_u8 *)"1234",(ee_u8 *)"-874",(ee_u8 *)"+122"};
static ee_u8 *floatpat[4]={(ee_u8 *)"35.54400",(ee_u8 *)".1234500",(ee_u8 *)"-110.700",(ee_u8 *)"+0.64400"};
static ee_u8 *scipat[4]  ={(ee_u8 *)"5.500e+3",(ee_u8 *)"-.123e-2",(ee_u8 *)"-87e+832",(ee_u8 *)"+0.6e-12"};
static ee_u8 *errpat[4]  ={(ee_u8 *)"T0.3e-1F",(ee_u8 *)"-T.T++Tq",(ee_u8 *)"1T3.4e4z",(ee_u8 *)"34.0e-T^"};

/* Function: core_init_state
        Initialize the input data for the state machine.

        Populate the input with several predetermined strings, interspersed.
        Actual patterns chosen depend on the seed parameter.

        Note:
        The seed parameter MUST be supplied from a source that cannot be determined at compile time
*/
void core_init_state(ee_u32 size, ee_s16 seed, ee_u8 *p) {
        ee_u32 total=0,next=0,i;
        ee_u8 *buf=0;
#if CORE_DEBUG
        ee_u8 *start=p;
        ee_printf("State: %d,%d\n",size,seed);
#endif
        size--;
        next=0;
        while ((total+next+1)<size) {
                if (next>0) {
                        for(i=0;i<next;i++)
                                *(p+total+i)=buf[i];
                        *(p+total+i)=',';
                        total+=next+1;
                }
                seed++;
                switch (seed & 0x7) {
                        case 0: /* int */
                        case 1: /* int */
                        case 2: /* int */
                                buf=intpat[(seed>>3) & 0x3];
                                next=4;
                        break;
                        case 3: /* float */
                        case 4: /* float */
                                buf=floatpat[(seed>>3) & 0x3];
                                next=8;
                        break;
                        case 5: /* scientific */
                        case 6: /* scientific */
                                buf=scipat[(seed>>3) & 0x3];
                                next=8;
                        break;
                        case 7: /* invalid */
                                buf=errpat[(seed>>3) & 0x3];
                                next=8;
                        break;
                        default: /* Never happen, just to make some compilers happy */
                        break;
                }
        }
        size++;
        while (total<size) { /* fill the rest with 0 */
                *(p+total)=0;
                total++;
        }
#if CORE_DEBUG
        ee_printf("State Input: %s\n",start);
#endif
}

static ee_u8 ee_isdigit(ee_u8 c) {
        ee_u8 retval;
        retval = ((c>='0') & (c<='9')) ? 1 : 0;
        return retval;
}

/* Function: core_state_transition
        Actual state machine.

        The state machine will continue scanning until either:
        1 - an invalid input is detcted.
        2 - a valid number has been detected.

        The input pointer is updated to point to the end of the token, and the end state is returned (either specific format determined or invalid).
*/

enum CORE_STATE core_state_transition( ee_u8 **instr , ee_u32 *transition_count) {
        ee_u8 *str=*instr;
        ee_u8 NEXT_SYMBOL;
        enum CORE_STATE state=CORE_START;
        for( ; *str && state != CORE_INVALID; str++ ) {
                NEXT_SYMBOL = *str;
                if (NEXT_SYMBOL==',') /* end of this input */ {
                        str++;
                        break;
                }
                switch(state) {
                case CORE_START:
                        if(ee_isdigit(NEXT_SYMBOL)) {
                                state = CORE_INT;
                        }
                        else if( NEXT_SYMBOL == '+' || NEXT_SYMBOL == '-' ) {
                                state = CORE_S1;
                        }
                        else if( NEXT_SYMBOL == '.' ) {
                                state = CORE_FLOAT;
                        }
                        else {
                                state = CORE_INVALID;
                                transition_count[CORE_INVALID]++;
                        }
                        transition_count[CORE_START]++;
                        break;
                case CORE_S1:
                        if(ee_isdigit(NEXT_SYMBOL)) {
                                state = CORE_INT;
                                transition_count[CORE_S1]++;
                        }
                        else if( NEXT_SYMBOL == '.' ) {
                                state = CORE_FLOAT;
                                transition_count[CORE_S1]++;
                        }
                        else {
                                state = CORE_INVALID;
                                transition_count[CORE_S1]++;
                        }
                        break;
                case CORE_INT:
                        if( NEXT_SYMBOL == '.' ) {
                                state = CORE_FLOAT;
                                transition_count[CORE_INT]++;
                        }
                        else if(!ee_isdigit(NEXT_SYMBOL)) {
                                state = CORE_INVALID;
                                transition_count[CORE_INT]++;
                        }
                        break;
                case CORE_FLOAT:
                        if( NEXT_SYMBOL == 'E' || NEXT_SYMBOL == 'e' ) {
                                state = CORE_S2;
                                transition_count[CORE_FLOAT]++;
                        }
                        else if(!ee_isdigit(NEXT_SYMBOL)) {
                                state = CORE_INVALID;
                                transition_count[CORE_FLOAT]++;
                        }
                        break;
                case CORE_S2:
                        if( NEXT_SYMBOL == '+' || NEXT_SYMBOL == '-' ) {
                                state = CORE_EXPONENT;
                                transition_count[CORE_S2]++;
                        }
                        else {
                                state = CORE_INVALID;
                                transition_count[CORE_S2]++;
                        }
                        break;
                case CORE_EXPONENT:
                        if(ee_isdigit(NEXT_SYMBOL)) {
                                state = CORE_SCIENTIFIC;
                                transition_count[CORE_EXPONENT]++;
                        }
                        else {
                                state = CORE_INVALID;
                                transition_count[CORE_EXPONENT]++;
                        }
                        break;
                case CORE_SCIENTIFIC:
                        if(!ee_isdigit(NEXT_SYMBOL)) {
                                state = CORE_INVALID;
                                transition_count[CORE_INVALID]++;
                        }
                        break;
                default:
                        break;
                }
        }
        *instr=str;
        return state;
}

int main(int argc, char *argv[]) {

        void *memblock = (void *)static_memblk;
        ee_u32 size = TOTAL_DATA_SIZE;
        core_init_state(size, 0, memblock);
        core_bench_state(size, (ee_u8*)memblock, 0, 0, 1);
        return 0;
}
