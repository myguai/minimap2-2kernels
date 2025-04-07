// Copyright (C) 2013-2018 Altera Corporation, San Jose, California, USA. All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to
// whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
// 
// This agreement shall be governed in all respects by the laws of the State of California and
// by the laws of the United States of America.

// Author: sunjl

#define MM_SEED_SEG_SHIFT  48
#define MM_SEED_SEG_MASK   (0xffULL<<(MM_SEED_SEG_SHIFT))

#define MAX_TRIPCOUNT 512
#define TRIPCOUNT_PER_SUBPART 128

#define BUFFER_SIZE 16

#include "RTL/rtl_lib.h"

inline void chain(int total_subparts,
                         int max_dist_x, int max_dist_y, int bw, int q_span, int avg_qspan_fixed, 
						 __global const ulong2 *restrict a,
                         __global int2 *restrict result
                         )
{

	unsigned long a_x_local0[TRIPCOUNT_PER_SUBPART] = {0};
	int a_y_local0[TRIPCOUNT_PER_SUBPART] = {0};
	int f_local0[TRIPCOUNT_PER_SUBPART];
    int p_local0[TRIPCOUNT_PER_SUBPART];
    //unsigned char subparts_shift0[TRIPCOUNT_PER_SUBPART] = {[0 ... TRIPCOUNT_PER_SUBPART-1]=1};

	unsigned long a_x_local1[TRIPCOUNT_PER_SUBPART] = {0};
	int a_y_local1[TRIPCOUNT_PER_SUBPART] = {0};
	int f_local1[TRIPCOUNT_PER_SUBPART];
    int p_local1[TRIPCOUNT_PER_SUBPART];
    //unsigned char subparts_shift1[TRIPCOUNT_PER_SUBPART] = {[0 ... TRIPCOUNT_PER_SUBPART-1]=1};
    
	unsigned long a_x_local2[TRIPCOUNT_PER_SUBPART] = {0};
	int a_y_local2[TRIPCOUNT_PER_SUBPART] = {0};
	int f_local2[TRIPCOUNT_PER_SUBPART];
    int p_local2[TRIPCOUNT_PER_SUBPART];
    //unsigned char subparts_shift2[TRIPCOUNT_PER_SUBPART] = {[0 ... TRIPCOUNT_PER_SUBPART-1]=1};
    
	unsigned long a_x_local3[TRIPCOUNT_PER_SUBPART+1] = {0};
	int a_y_local3[TRIPCOUNT_PER_SUBPART+1] = {0};
	int f_local3[TRIPCOUNT_PER_SUBPART+1];
    int p_local3[TRIPCOUNT_PER_SUBPART+1];
    //unsigned char subparts_shift3[TRIPCOUNT_PER_SUBPART+1] = {[0 ... TRIPCOUNT_PER_SUBPART]=1};


    unsigned char subparts_processed = 0;// Initialize a counter to keep track of already processed sub-parts for current anchor
    unsigned char subparts_to_process = 0;//Initialize a variable to store the total number of sub-parts for current anchor

    unsigned char subparts_to_process_buffer[BUFFER_SIZE];


	// fill the score and backtrack arrays
    int i = -1;
    #pragma ivdep array(subparts_to_process_buffer)
	for (int g = 0; g < total_subparts + MAX_TRIPCOUNT; ++g) {

        //printf("%ld\t%ld\t%d\t%d\n", total_subparts, i, subparts_processed, subparts_to_process);
        
        if (subparts_processed == 0) {//If all the sub-parts for previous anchor are processed, load next anchor (into FIFO A[]) and its sub-part count
            i++;
            ulong2 a_local = __prefetching_load(&a[i]);

            a_x_local0[0] = a_local.x;
            a_y_local0[0] = (int)a_local.y;
            unsigned char subparts_local = (a_local.y>>44)&0x0f;

            

            f_local0[0] = q_span;
            p_local0[0] = -1;

            if(i>15)
            {
                subparts_to_process = subparts_to_process_buffer[((unsigned char)i)&0xf];
            }
            subparts_to_process_buffer[((unsigned char)i+15)&0xf] = subparts_local;
        }
        
		unsigned long ri = a_x_local3[TRIPCOUNT_PER_SUBPART];
		int qi = a_y_local3[TRIPCOUNT_PER_SUBPART];


		#pragma unroll
		for (int j = TRIPCOUNT_PER_SUBPART - 1; j >= 0; j--) {

            unsigned long a_x_local_j;
            int a_y_local_j;
            int f_local_j;

            if (subparts_processed == 0) {
                a_x_local_j = a_x_local3[j];
                a_y_local_j = a_y_local3[j];
                f_local_j = f_local3[j];
            }

            if (subparts_processed == 1) {
                a_x_local_j = a_x_local2[j];
                a_y_local_j = a_y_local2[j];
                f_local_j = f_local2[j];
            }

            if (subparts_processed == 2) {
                a_x_local_j = a_x_local1[j];
                a_y_local_j = a_y_local1[j];
                f_local_j = f_local1[j];
            }

            if (subparts_processed == 3) {
                a_x_local_j = a_x_local0[j];
                a_y_local_j = a_y_local0[j];
                f_local_j = f_local0[j];
            }

            //change direction so ri qi--ax ay exchange
            int sc = compute_sc(ri, qi, a_x_local_j, a_y_local_j, q_span, avg_qspan_fixed, max_dist_x, max_dist_y, bw);
			sc += f_local3[TRIPCOUNT_PER_SUBPART];

            if(sc >=f_local_j && sc != q_span)
            {
                if(subparts_processed ==0)
                {
                    p_local3[j]=i-MAX_TRIPCOUNT;
                    f_local3[j] = sc;                    
                }
                if(subparts_processed ==1)
                {
                    p_local2[j]=i-MAX_TRIPCOUNT;
                    f_local2[j] = sc;                    
                }
                if(subparts_processed ==2)
                {
                    p_local1[j]=i-MAX_TRIPCOUNT;
                    f_local1[j] = sc;                    
                }
                if(subparts_processed ==3)
                {
                    p_local0[j]=i-MAX_TRIPCOUNT;
                    f_local0[j] = sc;                    
                }
            }
		}



        //subparts_processed++;//Increment the processed sub-parts counter

        if(subparts_processed == subparts_to_process)
        {
            int2 result_local;
            result_local.x=f_local3[TRIPCOUNT_PER_SUBPART];
            result_local.y=p_local3[TRIPCOUNT_PER_SUBPART]; 
            result[i] = result_local;         
        }

        //Shift locally stored anchors and chaining scores in the FIFO buffers if all the sub-parts for the current anchor are processed  
        #pragma unroll
        for (int reg = TRIPCOUNT_PER_SUBPART; reg > 0; reg--) {
            if (subparts_processed == subparts_to_process) {
                f_local3[reg] = f_local3[reg - 1];
                p_local3[reg] = p_local3[reg - 1];
                a_x_local3[reg] = a_x_local3[reg - 1];
                a_y_local3[reg] = a_y_local3[reg - 1];
                //subparts_shift3[reg] = subparts_shift3[reg - 1];
            }
        }

        if (subparts_processed == subparts_to_process) {
            f_local3[0] = f_local2[TRIPCOUNT_PER_SUBPART - 1];
            p_local3[0] = p_local2[TRIPCOUNT_PER_SUBPART - 1];
            a_x_local3[0] = a_x_local2[TRIPCOUNT_PER_SUBPART - 1];
            a_y_local3[0] = a_y_local2[TRIPCOUNT_PER_SUBPART - 1];
            //subparts_shift3[0] = subparts_shift2[TRIPCOUNT_PER_SUBPART -1];
        }

        #pragma unroll
        for (int reg = TRIPCOUNT_PER_SUBPART - 1; reg > 0; reg--) {
            if (subparts_processed == subparts_to_process) {
                f_local2[reg] = f_local2[reg - 1];
                p_local2[reg] = p_local2[reg - 1];
                a_x_local2[reg] = a_x_local2[reg - 1];
                a_y_local2[reg] = a_y_local2[reg - 1];
                //subparts_shift2[reg] = subparts_shift2[reg - 1];
            }
        }

        if (subparts_processed == subparts_to_process) {
            f_local2[0] = f_local1[TRIPCOUNT_PER_SUBPART - 1];
            p_local2[0] = p_local1[TRIPCOUNT_PER_SUBPART - 1];
            a_x_local2[0] = a_x_local1[TRIPCOUNT_PER_SUBPART - 1];
            a_y_local2[0] = a_y_local1[TRIPCOUNT_PER_SUBPART - 1];
            //subparts_shift2[0] = subparts_shift1[TRIPCOUNT_PER_SUBPART -1];
        }
        
		#pragma unroll
        for (int reg = TRIPCOUNT_PER_SUBPART - 1; reg > 0; reg--) {
            if (subparts_processed == subparts_to_process) {
                f_local1[reg] = f_local1[reg - 1];
                p_local1[reg] = p_local1[reg - 1];
                a_x_local1[reg] = a_x_local1[reg - 1];
                a_y_local1[reg] = a_y_local1[reg - 1];
                //subparts_shift1[reg] = subparts_shift1[reg - 1];
            }
        }

        if (subparts_processed == subparts_to_process) {
            f_local1[0] = f_local0[TRIPCOUNT_PER_SUBPART -1];
            p_local1[0] = p_local0[TRIPCOUNT_PER_SUBPART -1];
            a_x_local1[0] = a_x_local0[TRIPCOUNT_PER_SUBPART -1];
            a_y_local1[0] = a_y_local0[TRIPCOUNT_PER_SUBPART -1];
            //subparts_shift1[0] = subparts_shift0[TRIPCOUNT_PER_SUBPART -1];
        }

        #pragma unroll
        for (int reg = TRIPCOUNT_PER_SUBPART -1; reg > 0; reg--) {
            if (subparts_processed == subparts_to_process) {
                f_local0[reg] = f_local0[reg - 1];
                p_local0[reg] = p_local0[reg - 1];
                a_x_local0[reg] = a_x_local0[reg - 1];
                a_y_local0[reg] = a_y_local0[reg - 1];
                //subparts_shift0[reg] = subparts_shift0[reg - 1];
            }
        }



        if (subparts_processed == subparts_to_process) {// lly-601:fix the subparts may speed up?
			subparts_processed = 0;
        }
        else
        {
            subparts_processed ++;
        }
	}

}


__kernel __attribute__((uses_global_work_offset(0)))  void minimap2_opencl0(long total_subparts,
                         int max_dist_x, int max_dist_y, int bw, int q_span, int avg_qspan_fixed, 
						 __global const ulong2 *restrict a,
                         __global int2 *restrict result
                         )
{
    chain(total_subparts, max_dist_x, max_dist_y, bw, q_span, avg_qspan_fixed, a, result);
}


__kernel __attribute__((uses_global_work_offset(0))) void minimap2_opencl1(long total_subparts,
                         int max_dist_x, int max_dist_y, int bw, int q_span, int avg_qspan_fixed, 
						 __global const ulong2 *restrict a,
                         __global int2 *restrict result
                         )
{
    chain(total_subparts, max_dist_x, max_dist_y, bw, q_span, avg_qspan_fixed, a, result);
}
