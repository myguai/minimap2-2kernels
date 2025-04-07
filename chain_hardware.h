#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cstring>
#include "CL/opencl.h"
#include "AOCLUtils/aocl_utils.h"

#include <sys/time.h>
#include <sys/resource.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <string>

#include "minimap.h"

// Parameters used for HW/SW split (ONT)
//merge mem and 2kernel
#define ONT_K1_HW 9.560322833667137e-06
#define ONT_K2_HW 7.524389557302505e-06
#define ONT_C_HW 0.4918630445679071
#define ONT_K_SW 4.637303730088228e-06
#define ONT_C_SW -0.1308447100947506
//change direction
// #define ONT_K1_HW 9.1236974869122e-06
// #define ONT_K2_HW 6.7830729270824e-05
// #define ONT_C_HW 0.5385491645561211
// #define ONT_K_SW 5.237303730088228e-06
// #define ONT_C_SW -0.9308447100947506
// #define ONT_K_SW 4.630361471098566e-06   //actual
// #define ONT_C_SW -0.14522514152484778
//origin
// #define ONT_K1_HW 8.429346604594088e-06
// #define ONT_K2_HW 4.219985490696127e-05
// #define ONT_C_HW 0.6198209995910657
// #define ONT_K_SW 5.237303730088228e-06
// #define ONT_C_SW -0.9308447100947506
//correct
// K1_HW   : 9.580990264707604e-06
// K2_HW   : 8.835431564133736e-06
// C_HW    : 0.7688153644445168
// K_SW    : 4.518821683323281e-06
// C_SW    : 0.2094467719078743

// Parameters used for HW/SW split (PacBio CCS)
#define PBCCS_K1_HW 6.23200320406657e-06
#define PBCCS_K2_HW 8.198684740597609e-06
#define PBCCS_C_HW 0.5640757192659418
// #define PBCCS_K_SW 4.268725898267481e-06
// #define PBCCS_C_SW -4.818488045685457
// #define PBCCS_K1_HW 7.437323634999326e-06
// #define PBCCS_K2_HW 4.2380603263944086e-05
// #define PBCCS_C_HW 0.5800485806078544
#define PBCCS_K_SW 6.786967941956902e-06
#define PBCCS_C_SW -8.476899785564488

//#define FIND_HWSW_PARAMS

//#define DEBUG_HW          // chain_hardware.cpp (to print out steps in hardware processing)
//#define VERIFY_OUTPUT       // chain.c (to run both on software and hardware and cross-check the outputs)

// used for measuring the time taken for each chaining task (only the time taken by the section computed on hardware/software)
// this also includes the overhead taken for threshold computation and extra malloc for trip_count
// #define MEASURE_CHAINING_TIME // chain.c

// #define MEASURE_CORE_CHAINING_TIME // chain.c (to measure total time taken for core part of chaining. IMPORTANT: minimap2 should be run with 1 thread to get accurate timing)

//#define MEASURE_CHAINING_TIME_HW_FINE // chain_hardware.cpp (measures chaining time and wait time seperately in hardware chaining)
 
#define EXTRA_ELEMS 0 // added to temporarily fix the issue with parallel execution of OpenCL hardware kernels 
                        // (i.e. all input/output arrays used in hardware chaining are filled with EXTRA_ELEMS no. of elements)

#define PROCESS_ON_SW_IF_HW_BUSY // controls whether to process chaining tasks (chosen for hardware) on software if it's more suitable to do so

#define ENABLE_MAX_SKIP_ON_SW // enables max_skip heuristic for chaining on software

using namespace std;

// Important: don't change the values below unless you recompile hardware code (device/minimap2_opencl.cl)
#define NUM_HW_KERNELS 2
#define TRIPCOUNT_PER_SUBPART 128
#define MAX_SUBPARTS 4
#define MAX_TRIPCOUNT (TRIPCOUNT_PER_SUBPART * MAX_SUBPARTS)

#define DEVICE_MAX_N 332000000
#define BUFFER_MAX_N (DEVICE_MAX_N / 2)
#define BUFFER_N (BUFFER_MAX_N / 8)  // can change the divisor

#define STRING_BUFFER_LEN 1024

typedef struct
{
    int f;
    int p;
} mm2int_t;


int run_chaining_on_hw(cl_long n, cl_int max_dist_x, cl_int max_dist_y, cl_int bw, cl_int q_span, cl_int avg_qspan,
                mm128_t * a, mm2int_t * result, cl_long total_subparts, cl_uchar* num_subparts, int tid, float hw_time_pred, float sw_time_pred);
bool hardware_init(long);
void cleanup();



