/**
 * \author Kerem Okyay
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_
#define FIFO_NAME "logFifo"
#define DATAMGR_INDEX 0
#define STORAGEMGR_INDEX 1

#include <stdint.h>
#include <time.h>
/*#ifndef TIMEOUT
    #error TIMEOUT not set
#endif
#ifndef SET_MAX_TEMP
    #error SET_MAX_TEMP not set
#endif
#ifndef SET_MIN_TEMP
    #error SET_MIN_TEMP not set
#endif*/
#ifndef RUN_AVG_LENGTH
    #define RUN_AVG_LENGTH 5
#endif
#ifndef DEBUG_MODE
    #define DEBUG_MODE 0
#endif


typedef struct parent parent_data;

typedef uint16_t sensor_id_t;
typedef double sensor_value_t;
typedef time_t sensor_ts_t;         // UTC timestamp as returned by time() - notice that the size of time_t is different on 32/64 bit machine

typedef struct {
    sensor_id_t id;
    sensor_value_t value;
    sensor_ts_t ts;
} sensor_data_t;


#endif /* _CONFIG_H_ */
