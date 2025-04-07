#define MM_SEED_SEG_SHIFT  48
#define MM_SEED_SEG_MASK   (0xffULL<<(MM_SEED_SEG_SHIFT))

#define INNER_LOOP_TRIP_COUNT 64

__constant char LogTable256[256] = {
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
	-1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
	LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
	LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};

inline int ilog2_32(unsigned int v){
  unsigned int t,tt;
  if ((tt = v>>16)) return (t = tt>>8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
  return (t = v>>8) ? 8 + LogTable256[t] : LogTable256[v]; 
}


inline int compute_sc(unsigned long a_x, int a_y, unsigned long ri, int qi, int q_span, int avg_qspan, int max_dist_x, int max_dist_y, int bw) 
{
	long dr = ri - a_x;
	int dq = qi - a_y, dd, sc, log_dd;
	if (dr == 0 || dq <= 0 || ri > a_x + max_dist_x ) return 0x80000001; 
	if (dq > max_dist_y || dq > max_dist_x) return 0x80000001;
	dd = (dr > dq)? (dr - dq) : (dq - dr);
	if(dd<0){
		printf("dr is %ld, dq is %d, dd is %d, ri is %lu, a_x is %lu\n",dr, dq, dd, ri, a_x);
	}
	if (dd > bw) return 0x80000001;
	if (dr > max_dist_y) return 0x80000001;
	int min_d = dq < dr? dq : dr;
	sc = min_d > q_span? q_span : min_d;
	log_dd = dd? ilog2_32(dd) : 0;

    int c_lin_part = (dd&0x0003ffff)  * (avg_qspan&0x0003ffff);
	// if(c_lin_part > 0xffffffff){
	// 	printf("overflow, c_lin_part is %lu, dd is %d, log_dd is %d\n",c_lin_part, dd, log_dd);
	// }
    int c_lin = (c_lin_part >> 19) + (((c_lin_part >> 7)&0xfff)==0xfff);
	sc -= (c_lin + (log_dd>>1));

	return sc;
}

