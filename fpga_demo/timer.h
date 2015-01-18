#include <sys/time.h>
#include <string.h>

#define TS_INIT(name, num) struct timeval name ## _timeval[num]

#define TS_STAMP(name, num) gettimeofday(& name ## _timeval[num], NULL)

#define TS_TO_MS(name, num) (((double) name ## _timeval[num].tv_sec*1000.0) + ((double) name ## _timeval[num].tv_usec/1000.0))

#define TS_ELAPSED_MS(name, e, s) (TS_TO_MS(name, e) - TS_TO_MS(name, s))



#define TC_INIT(name, num) double name ## _totaltime[num] = {}; long name ## _count[num] = {}

#define TC_ACCRUE(name, num, e, s) name ## _totaltime[num] += TS_ELAPSED_MS(name, e, s); name ## _count[num] = name ## _count[num] + 1

#define TC_SUM(name, num) name ## _totaltime[num]

#define TC_ITERS(name, num) name ## _count[num]

#define TC_ITER_AVG(name, num) TC_SUM(name, num)/TC_ITERS(name, num)

