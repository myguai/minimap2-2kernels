#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "minimap.h"
#include "mmpriv.h"
#include "kalloc.h"
#include "chain_hardware.h"


extern float K1_HW, K2_HW, C_HW, K_SW, C_SW;

#ifdef MEASURE_CORE_CHAINING_TIME
extern double core_chaining_time_total;
#endif

static const char LogTable256[256] = {
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
	-1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
	LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
	LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};

static inline int ilog2_32(uint32_t v)
{
	uint32_t t, tt;
	if ((tt = v>>16)) return (t = tt>>8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
	return (t = v>>8) ? 8 + LogTable256[t] : LogTable256[v];
}

mm128_t *mm_chain_dp(int max_dist_x, int max_dist_y, int bw, int max_skip, int max_iter, int min_cnt, int min_sc, float gap_scale, int is_cdna, int n_segs, int64_t n, mm128_t *a, int *n_u_, uint64_t **_u, void *km, int tid)
{ // TODO: make sure this works when n has more than 32 bits
	//fprintf(stderr,"max iter is%d\n",max_iter);
	int32_t k, *t, *v, n_u, n_v;
	mm2int_t * result, * result_mem;
	int64_t i, j, st = 0;
	uint64_t *u, *u2, sum_qspan = 0;
	float avg_qspan_scaled;
	int avg_qspan_fixed;
	mm128_t *b, *w;
	// unsigned char * num_subparts;

	if (_u) *_u = 0, *n_u_ = 0;
	if (n == 0 || a == 0) {
		kfree(km, a);
		return 0;
	}
	result_mem = (mm2int_t*)kmalloc(km, (n+512) * 8);
	result = result_mem+512;
	t = (int32_t*)kmalloc(km, n * 4);
	v = (int32_t*)kmalloc(km, n * 4);
	memset(t, 0, n * 4);

#ifdef MEASURE_CORE_CHAINING_TIME
	double chain_start = realtime();
#endif

	for (i = 0; i < n; ++i) sum_qspan += a[i].y>>32&0xff;
	avg_qspan_scaled = .01 * (float)sum_qspan / n;
	avg_qspan_fixed = int((float)avg_qspan_scaled* 65536*8);//lly601

	/*--------- HW/SW time prediction Start ------------*/
	int32_t * st_record = (int32_t*)malloc(n * sizeof(int32_t));


#ifdef VERIFY_OUTPUT
	long total_trip_count = 0;
	st = 0;
	int64_t total_subparts = 0;
	float hw_time_pred=0;
	float sw_time_pred=0;

	//num_subparts = (unsigned char*)malloc(n+481);

	int32_t * tc = (int32_t*)malloc(n * sizeof(int32_t));

	for (i = 0; i < n; i++) {
		// determine and store the inner loop's trip count (max is INNER_LOOP_TRIP_COUNT_MAX)
		while (st < n && a[st].x <= a[i].x + max_dist_x) {
			if (st - i > MAX_TRIPCOUNT)  
				st_record[st]=st-MAX_TRIPCOUNT;
			else
				st_record[st]=i;
			++st;
			}
		int inner_loop_trip_count = st - i;
		if (inner_loop_trip_count > MAX_TRIPCOUNT) { 
			inner_loop_trip_count = MAX_TRIPCOUNT;
		}
		total_trip_count += inner_loop_trip_count;

		tc[i] = inner_loop_trip_count;


		int subparts = (inner_loop_trip_count) / TRIPCOUNT_PER_SUBPART;
		if (inner_loop_trip_count == 0 || inner_loop_trip_count % TRIPCOUNT_PER_SUBPART > 0) subparts++;
		if(subparts >1)
		{
			a[i+497].y = (a[i+497].y)|(long(subparts-1)<<40);
		}
		
		//subparts_index++;
		// for(int64_t k=subparts_index;k<subparts_index+subparts;k++)
		// {
		// 	num_subparts[k] = (unsigned char)subparts-1;
		// }
		// subparts_index=subparts_index+subparts;
		total_subparts += subparts;
	}





	//fprintf(stderr,"total_subparts/n=%f, subparts is %ld, index is %ld\n",total_subparts/(float)n, total_subparts, subparts_index);
#else
	float hw_time_pred;
	float sw_time_pred;
	long total_trip_count = 0;
	int64_t total_subparts = 0;


	st = 0;
	for (i = 0; i < n; i++) {
		// determine and store the inner loop's trip count (max is INNER_LOOP_TRIP_COUNT_MAX)
		while (st < n && a[st].x <= a[i].x + max_dist_x){
			
			if (st - i > MAX_TRIPCOUNT)  
				st_record[st]=st-MAX_TRIPCOUNT;
			else
				st_record[st]=i;
			st++;
		
		} 
		int inner_loop_trip_count = st - i;
		if (inner_loop_trip_count > MAX_TRIPCOUNT) { 
			inner_loop_trip_count = MAX_TRIPCOUNT;
		}
		total_trip_count += inner_loop_trip_count;

		int subparts = (inner_loop_trip_count) / TRIPCOUNT_PER_SUBPART;
		if (inner_loop_trip_count == 0 || inner_loop_trip_count % TRIPCOUNT_PER_SUBPART > 0) subparts++;
		if(subparts>1)
		{
			a[i+497].y = (a[i+497].y)|(long(subparts-1)<<40);
		}

		total_subparts += subparts;
	}	
	//fprintf(stderr,"total_subparts/n=%f, subparts is %ld, index is %ld\n",total_subparts/(float)n, total_subparts, subparts_index);

	hw_time_pred = K1_HW * n + K2_HW * total_subparts + C_HW;
	sw_time_pred = K_SW * total_trip_count + C_SW;	


#endif
	
	/*--------- HW/SW time prediction End ------------*/

#if defined(VERIFY_OUTPUT) || defined(FIND_HWSW_PARAMS)
	mm2int_t * result_hw_mem = (mm2int_t*)malloc((n + EXTRA_ELEMS + 512) * sizeof(mm2int_t));
	mm2int_t * result_hw = result_hw_mem + 512;
	// int32_t * f_hw = (int32_t*)malloc((n + EXTRA_ELEMS) * sizeof(int32_t));
	// int32_t * p_hw = (int32_t*)malloc((n + EXTRA_ELEMS) * sizeof(int32_t));
	int32_t * v_hw = (int32_t*)malloc((n + EXTRA_ELEMS) * sizeof(int32_t));
#endif

	int q_span_hw= a[0].y>>32&0xff;


#ifndef FIND_HWSW_PARAMS

#ifndef VERIFY_OUTPUT
	if (hw_time_pred < sw_time_pred - 0.8) { // execute on HW
		//fprintf(stderr, "call hardware\n");
		int hw_chain = run_chaining_on_hw(n, max_dist_x, max_dist_y, bw, q_span_hw, avg_qspan_fixed, a, result_mem, total_subparts, NULL, tid, hw_time_pred, sw_time_pred);

#ifdef PROCESS_ON_SW_IF_HW_BUSY
		if (hw_chain == 0) {
			for (i = 0; i < n-32; ++i) {
				int32_t max_f = result[i].f;
				int64_t max_j = result[i].p;
				v[i] = max_j >= 0 && v[max_j] > max_f? v[max_j] : max_f; // v[] keeps the peak score up to i; f[] is the score ending at i, not always the peak	
			}
			for (i = n-32; i < n; ++i) {
				int32_t max_f = 15;
				int64_t max_j = -1;
				v[i] = max_j >= 0 && v[max_j] > max_f? v[max_j] : max_f; // v[] keeps the peak score up to i; f[] is the score ending at i, not always the peak	
			}
		} else {
			// fill the score and backtrack arrays
			//fprintf(stderr, "recall software\n");
			st = 0;
			for (i = 0; i < n; ++i) {
				uint64_t ri = a[i].x;
				int64_t max_j = -1;
				int32_t qi = (int32_t)a[i].y, q_span = a[i].y>>32&0xff; // NB: only 8 bits of span is used!!!
				int32_t max_f = q_span, n_skip = 0, min_d;
				int32_t sidi = (a[i].y & MM_SEED_SEG_MASK) >> MM_SEED_SEG_SHIFT;
				// while (st < i && ri > a[st].x + max_dist_x) ++st;
				// if (i - st > max_iter) st = i - max_iter;
				for (j = i - 1; j >= st_record[i]; --j) {
					int64_t dr = ri - a[j].x;
					int32_t dq = qi - (int32_t)a[j].y, dd, sc, log_dd, gap_cost;
					int32_t sidj = (a[j].y & MM_SEED_SEG_MASK) >> MM_SEED_SEG_SHIFT;
					if ((sidi == sidj && dr == 0) || dq <= 0) continue; // don't skip if an anchor is used by multiple segments; see below
					if ((sidi == sidj && dq > max_dist_y) || dq > max_dist_x) continue;
					dd = dr > dq? dr - dq : dq - dr;
					if (sidi == sidj && dd > bw) continue;
					if (n_segs > 1 && !is_cdna && sidi == sidj && dr > max_dist_y) continue;
					min_d = dq < dr? dq : dr;
					sc = min_d > q_span? q_span : dq < dr? dq : dr;
					log_dd = dd? ilog2_32(dd) : 0;
					gap_cost = 0;
					if (is_cdna || sidi != sidj) {
						int c_log, c_lin;
						c_lin = (int)(dd * avg_qspan_scaled);
						c_log = log_dd;
						if (sidi != sidj && dr == 0) ++sc; // possibly due to overlapping paired ends; give a minor bonus
						else if (dr > dq || sidi != sidj) gap_cost = c_lin < c_log? c_lin : c_log;
						else gap_cost = c_lin + (c_log>>1);
					} else gap_cost = (int)(dd * avg_qspan_scaled) + (log_dd>>1);
					sc -= (int)((double)gap_cost * gap_scale + .499);
					sc += result[j].f;
#ifndef ENABLE_MAX_SKIP_ON_SW
					if (sc > max_f) {
						max_f = sc, max_j = j;
					} 
#else
					if (sc > max_f) {
						max_f = sc, max_j = j;
						if (n_skip > 0) --n_skip;
					} else if (t[j] == i) {
						if (++n_skip > max_skip)
							break;
					}
					if (result[j].p >= 0) t[result[j].p] = i;
#endif
				}
				result[i].f = max_f, result[i].p = max_j;
				v[i] = max_j >= 0 && v[max_j] > max_f? v[max_j] : max_f; // v[] keeps the peak score up to i; f[] is the score ending at i, not always the peak
			}
		}
#else
		for (i = 0; i < n; ++i) {
			int32_t max_f = result[i].f;
			int64_t max_j = result[i].p;
			v[i] = max_j >= 0 && v[max_j] > max_f? v[max_j] : max_f; // v[] keeps the peak score up to i; f[] is the score ending at i, not always the peak
		}
#endif

	} else { // execute on SW
		//fprintf(stderr,"call software\n");
#else 
		run_chaining_on_hw(n, max_dist_x, max_dist_y, bw, q_span_hw, avg_qspan_fixed, a, result_hw_mem, total_subparts, NULL, tid, hw_time_pred, sw_time_pred);

		for (i = 0; i < n; ++i) {
			int32_t max_f = result_hw[i].f;
			int64_t max_j = result_hw[i].p;
			v_hw[i] = max_j >= 0 && v_hw[max_j] > max_f? v_hw[max_j] : max_f; // v[] keeps the peak score up to i; f[] is the score ending at i, not always the peak

		}
#endif
		// fill the score and backtrack arrays
		st = 0;
		for (i = 0; i < n; ++i) {
			uint64_t ri = a[i].x;
			int64_t max_j = -1;
			int32_t qi = (int32_t)a[i].y, q_span = a[i].y>>32&0xff; // NB: only 8 bits of span is used!!!
			int32_t max_f = q_span, n_skip = 0, min_d;
			int32_t sidi = (a[i].y & MM_SEED_SEG_MASK) >> MM_SEED_SEG_SHIFT;

#ifdef VERIFY_OUTPUT
			while (st < i && ri > a[st].x + max_dist_x) ++st;
			if (i - st > max_iter) st = i - max_iter;
			for (j = i - 1; j >= st && j > (i - MAX_TRIPCOUNT - 1); --j) {
#else
			for (j = i - 1; j >= st_record[i]; --j) {
#endif
				int64_t dr = ri - a[j].x;
				int32_t dq = qi - (int32_t)a[j].y, dd, sc, log_dd, gap_cost;
				int32_t sidj = (a[j].y & MM_SEED_SEG_MASK) >> MM_SEED_SEG_SHIFT;
				if ((sidi == sidj && dr == 0) || dq <= 0) continue; // don't skip if an anchor is used by multiple segments; see below
				if ((sidi == sidj && dq > max_dist_y) || dq > max_dist_x) continue;
				dd = dr > dq? dr - dq : dq - dr;
				if (sidi == sidj && dd > bw) continue;
				if (n_segs > 1 && !is_cdna && sidi == sidj && dr > max_dist_y) continue;
				min_d = dq < dr? dq : dr;
				sc = min_d > q_span? q_span : dq < dr? dq : dr;
				log_dd = dd? ilog2_32(dd) : 0;
				gap_cost = 0;
				if (is_cdna || sidi != sidj) {
					int c_log, c_lin;
					c_lin = (int)(dd * avg_qspan_scaled);
					c_log = log_dd;
					if (sidi != sidj && dr == 0) ++sc; // possibly due to overlapping paired ends; give a minor bonus
					else if (dr > dq || sidi != sidj) gap_cost = c_lin < c_log? c_lin : c_log;
					else gap_cost = c_lin + (c_log>>1);
				} else gap_cost = (int)(dd * avg_qspan_scaled) + (log_dd>>1);
				sc -= (int)((double)gap_cost * gap_scale + .499);
				sc += result[j].f;
#if !defined(ENABLE_MAX_SKIP_ON_SW) || defined(VERIFY_OUTPUT)
				if (sc > max_f) {
					max_f = sc, max_j = j;
				} 
#else
				if (sc > max_f) {
					max_f = sc, max_j = j;
					if (n_skip > 0) --n_skip;
				} else if (t[j] == i) {
					if (++n_skip > max_skip)
						break;
				}
				if (result[j].p >= 0) t[result[j].p] = i;
#endif
			}
			result[i].f = max_f, result[i].p = max_j;
			v[i] = max_j >= 0 && v[max_j] > max_f? v[max_j] : max_f; // v[] keeps the peak score up to i; f[] is the score ending at i, not always the peak
		}
		
#ifndef VERIFY_OUTPUT
	}
#endif

#ifdef VERIFY_OUTPUT
	fprintf(stderr,"having verify\n");
	int mismatched = 0;
	for (i = 0; i < n; i++) {
		if (result[i].f != result_hw[i].f || result[i].p != result_hw[i].p || v[i] != v_hw[i]) {
			fprintf(stderr, "n = %ld, total_subparts = %ld, i = %ld | f = %d, f_hw = %d | p = %d, p_hw = %d | v = %d, v_hw = %d | , %d\n", n, total_subparts, i, result[i].f, result_hw[i].f, result[i].p, result_hw[i].p, v[i], v_hw[i], tc[i]);
			exit(1);
			mismatched++;
		}
		//else fprintf(stderr,"correct\n");
	}
	if (mismatched > 0) {
		fprintf(stderr, "mismatched = %d/%ld\n", mismatched, n);
		//fprintf(stderr, "total_trip_count = %d, sw_hw_frac = %f\n", total_trip_count, sw_hw_frac);
	}

	free(result_hw_mem);
	free(v_hw);
	free(tc);
#endif

#else // FIND_HWSW_PARAMS 

    if (tid > 0) {
		fprintf(stderr, "[Error] minimap2 should run only with a single thread (-t 1) when finding HW/SW split parameters\n");
        exit(1);
    }

    double hw_start = realtime();
	run_chaining_on_hw(n, max_dist_x, max_dist_y, bw, q_span_hw, avg_qspan_fixed, a, result_hw_mem, total_subparts, NULL, tid, hw_time_pred, sw_time_pred);
    double hw_time = (realtime() - hw_start) * 1000;
    
    double sw_start = realtime();
    st = 0;
    for (i = 0; i < n; ++i) {
        uint64_t ri = a[i].x;
        int64_t max_j = -1;
        int32_t qi = (int32_t)a[i].y, q_span = a[i].y>>32&0xff; // NB: only 8 bits of span is used!!!
        int32_t max_f = q_span, n_skip = 0, min_d;
        int32_t sidi = (a[i].y & MM_SEED_SEG_MASK) >> MM_SEED_SEG_SHIFT;


#ifdef VERIFY_OUTPUT
        while (st < i && ri > a[st].x + max_dist_x) ++st;
        if (i - st > max_iter) st = i - max_iter;
        for (j = i - 1; j >= st && j > (i - MAX_TRIPCOUNT - 1); --j) {
#else
        for (j = i - 1; j >= st_record[i]; --j) {
#endif
            int64_t dr = ri - a[j].x;
            int32_t dq = qi - (int32_t)a[j].y, dd, sc, log_dd, gap_cost;
            int32_t sidj = (a[j].y & MM_SEED_SEG_MASK) >> MM_SEED_SEG_SHIFT;
            if ((sidi == sidj && dr == 0) || dq <= 0) continue; // don't skip if an anchor is used by multiple segments; see below
            if ((sidi == sidj && dq > max_dist_y) || dq > max_dist_x) continue;
            dd = dr > dq? dr - dq : dq - dr;
            if (sidi == sidj && dd > bw) continue;
            if (n_segs > 1 && !is_cdna && sidi == sidj && dr > max_dist_y) continue;
            min_d = dq < dr? dq : dr;
            sc = min_d > q_span? q_span : dq < dr? dq : dr;
            log_dd = dd? ilog2_32(dd) : 0;
            gap_cost = 0;
            if (is_cdna || sidi != sidj) {
                int c_log, c_lin;
                c_lin = (int)(dd * avg_qspan_scaled);
                c_log = log_dd;
                if (sidi != sidj && dr == 0) ++sc; // possibly due to overlapping paired ends; give a minor bonus
                else if (dr > dq || sidi != sidj) gap_cost = c_lin < c_log? c_lin : c_log;
                else gap_cost = c_lin + (c_log>>1);
            } else gap_cost = (int)(dd * avg_qspan_scaled) + (log_dd>>1);
            sc -= (int)((double)gap_cost * gap_scale + .499);
            sc += result[j].f;

#if !defined(ENABLE_MAX_SKIP_ON_SW) || defined(VERIFY_OUTPUT)
            if (sc > max_f) {
                max_f = sc, max_j = j;
            } 
#else
            if (sc > max_f) {
                max_f = sc, max_j = j;
                if (n_skip > 0) --n_skip;
            } else if (t[j] == i) {
                if (++n_skip > max_skip)
                    break;
            }
            if (result[j].p >= 0) t[result[j].p] = i;
#endif
        }
        result[i].f = max_f, result[i].p = max_j;
    }
    double sw_time = (realtime() - sw_start) * 1000;

    fprintf(stderr, "param %ld\t%ld\t%ld\t%.3f\t%.3f\n", n, total_subparts, total_trip_count, hw_time, sw_time);

    for (i = 0; i < n; ++i) {
        int32_t max_f = result[i].f;
        int64_t max_j = result[i].p;
        v[i] = max_j >= 0 && v[max_j] > max_f? v[max_j] : max_f; // v[] keeps the peak score up to i; f[] is the score ending at i, not always the peak
    }

	free(result_hw_mem);
	free(v_hw);
#endif

#ifdef MEASURE_CORE_CHAINING_TIME
	core_chaining_time_total += (realtime() - chain_start);
#endif

	free(st_record);
	// find the ending positions of chains
	memset(t, 0, n * 4);
	for (i = 0; i < n; ++i)
		if (result[i].p >= 0) t[result[i].p] = 1;
	for (i = n_u = 0; i < n; ++i)
		if (t[i] == 0 && v[i] >= min_sc)
			++n_u;
	if (n_u == 0) {
		kfree(km, a); kfree(km, result_mem); kfree(km, t); kfree(km, v);
		return 0;
	}
	u = (uint64_t*)kmalloc(km, n_u * 8);
	for (i = n_u = 0; i < n; ++i) {
		if (t[i] == 0 && v[i] >= min_sc) {
			j = i;
			while (j >= 0 && result[j].f < v[j]) j = result[j].p; // find the peak that maximizes f[]
			if (j < 0) j = i; // TODO: this should really be assert(j>=0)
			u[n_u++] = (uint64_t)result[j].f << 32 | j;
		}
	}
	radix_sort_64(u, u + n_u);
	for (i = 0; i < n_u>>1; ++i) { // reverse, s.t. the highest scoring chain is the first
		uint64_t t = u[i];
		u[i] = u[n_u - i - 1], u[n_u - i - 1] = t;
	}

	// backtrack
	memset(t, 0, n * 4);
	for (i = n_v = k = 0; i < n_u; ++i) { // starting from the highest score
		int32_t n_v0 = n_v, k0 = k;
		j = (int32_t)u[i];
		do {
			v[n_v++] = j;
			t[j] = 1;
			j = result[j].p;
		} while (j >= 0 && t[j] == 0);
		if (j < 0) {
			if (n_v - n_v0 >= min_cnt) u[k++] = u[i]>>32<<32 | (n_v - n_v0);
		} else if ((int32_t)(u[i]>>32) - result[j].f >= min_sc) {
			if (n_v - n_v0 >= min_cnt) u[k++] = ((u[i]>>32) - result[j].f) << 32 | (n_v - n_v0);
		}
		if (k0 == k) n_v = n_v0; // no new chain added, reset
	}
	*n_u_ = n_u = k, *_u = u; // NB: note that u[] may not be sorted by score here

	// free temporary arrays
	kfree(km, result_mem); kfree(km, t);

	// write the result to b[]
	b = (mm128_t*)kmalloc(km, n_v * sizeof(mm128_t));
	for (i = 0, k = 0; i < n_u; ++i) {
		int32_t k0 = k, ni = (int32_t)u[i];
		for (j = 0; j < ni; ++j)
			b[k] = a[v[k0 + (ni - j - 1)]], ++k;
	}
	kfree(km, v);

	// sort u[] and a[] by a[].x, such that adjacent chains may be joined (required by mm_join_long)
	w = (mm128_t*)kmalloc(km, n_u * sizeof(mm128_t));
	for (i = k = 0; i < n_u; ++i) {
		w[i].x = b[k].x, w[i].y = (uint64_t)k<<32|i;
		k += (int32_t)u[i];
	}
	radix_sort_128x(w, w + n_u);
	u2 = (uint64_t*)kmalloc(km, n_u * 8);
	for (i = k = 0; i < n_u; ++i) {
		int32_t j = (int32_t)w[i].y, n = (int32_t)u[j];
		u2[i] = u[j];
		memcpy(&a[k], &b[w[i].y>>32], n * sizeof(mm128_t));
		k += n;
	}
	if (n_u) memcpy(u, u2, n_u * 8);
	if (k) memcpy(b, a, k * sizeof(mm128_t)); // write _a_ to _b_ and deallocate _a_ because _a_ is oversized, sometimes a lot
	kfree(km, a); kfree(km, w); kfree(km, u2);
	return b;
}
