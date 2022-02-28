#define _GNU_SOURCE
#include<pthread.h>
#include <stdio.h>
#include "lib/dplist.h"
#include "datamgr.h"
#include "config.h"
#include <stdlib.h>
#include "sbuffer.h"
#include "errmacros.h"


#define NO_ERROR "a"
#define MEMORY_ERROR "m"
#define INVALID_ERROR "i"


extern char connection_established;
extern char storagemgr_fail_flag;
extern char connmgr_state;
static dplist_t * room_map_list;

void element_free(void**);
int element_compare(void *x,void*y);
void* element_copy(void*);
void check_room_temperature(void);


typedef struct sensor_node {
    uint16_t sensor_id;
    uint16_t room_id;
    double running_avg;
    time_t last_modified;
    double temperatures[RUN_AVG_LENGTH];
}sensor_t;



void datamgr_parse_sensor_data(FILE * fp_sensor_map,sbuffer_t ** sbuffer){
    room_map_list = dpl_create(element_copy,element_free,element_compare);
    uint16_t roomidbuffer;
    uint16_t sensoridbuffer;
    char * log_msg;
    while(fscanf(fp_sensor_map, "%hu %hu",&roomidbuffer,&sensoridbuffer)>0){

        #if(DEBUG_MODE == 1)
        printf("roomid %hu sensorid %hu\n",roomidbuffer,sensoridbuffer);
        #endif
        sensor_t * sensor;
        sensor = malloc(sizeof(sensor_t));
        ERROR_HANDLER(sensor == NULL,MEMORY_ERROR);

        sensor->room_id = roomidbuffer;
        sensor->sensor_id = sensoridbuffer;


        for(int i = 0;i <RUN_AVG_LENGTH;i++)sensor->temperatures[i]=0;//array of -99s
        dpl_insert_at_index(room_map_list,(void*)sensor,dpl_size(room_map_list),true); // most recent modified will end up at the end 
        free(sensor);
    }  

    sensor_data_t databuffer;
    void * node = NULL;
    int sbuffer_rc = SBUFFER_SUCCESS;
    while(sbuffer_rc != SBUFFER_FAILURE&&storagemgr_fail_flag==0){//NO_DATA means empty the loop will go on, alive state = 1
        
        if(connection_established == 0)continue;
        sbuffer_rc = sbuffer_read(*sbuffer,&node,&databuffer,DATAMGR_INDEX);
        if(connmgr_state == 0&&(sbuffer_rc == SBUFFER_NO_DATA||sbuffer_rc == SBUFFER_NO_NEW_DATA))break;
        if(sbuffer_rc == SBUFFER_SUCCESS){
            int index;  
            index = dpl_get_index_of_element(room_map_list,(void*)(&databuffer));
            if(index == -1){
                #if(DEBUG_MODE == 1)
                    printf("invalid sensor id with %d\n",databuffer.id);
                #endif
                   asprintf(&log_msg,"Received sensor data with invalid sensor node ID %d\n",databuffer.id);

                   sbuffer_write_fifo(*sbuffer,log_msg);
                   free(log_msg);
            
            }else{
                #if(DEBUG_MODE == 1)
                    printf("Sensor id with %d read and value %f by datamgr\n",databuffer.id,databuffer.value);
                #endif
                sensor_t * sensor = (sensor_t*)dpl_get_element_at_index(room_map_list,index);
                sensor->last_modified = databuffer.ts;
                double new_sum =0.0; 
                for(int i = RUN_AVG_LENGTH-1;i>0;i--){ 
                    sensor->temperatures[i] = sensor->temperatures[i-1];
                    new_sum+=sensor->temperatures[i];
            }
            sensor->temperatures[0] = databuffer.value;
            new_sum+=databuffer.value;
            sensor->running_avg = new_sum/RUN_AVG_LENGTH;
            if(sensor->temperatures[RUN_AVG_LENGTH-1]!=0){
                if(sensor->running_avg < SET_MIN_TEMP){
                    #if(DEBUG_MODE == 1)
                        printf("The sensor node with %d reports it's too cold %f \n",sensor->sensor_id,sensor->running_avg);
                    #endif
                   
                   asprintf(&log_msg,"The sensor node with %d reports it's too cold %f\n",sensor->sensor_id,sensor->running_avg);
                   sbuffer_write_fifo(*sbuffer,log_msg);
                   free(log_msg);
                    
                }
                else if (sensor->running_avg > SET_MAX_TEMP){
                    //log event
                    #if(DEBUG_MODE == 1)
                        printf("The sensor node with %d reports it's too hot %f \n",sensor->sensor_id,sensor->running_avg);
                    #endif
                   asprintf(&log_msg,"The sensor node with %d reports it's too hot %f\n",sensor->sensor_id,sensor->running_avg);
                   sbuffer_write_fifo(*sbuffer,log_msg);
                   free(log_msg);
                }
            }

            }
        }
    }
}
void datamgr_free(){
    #if(DEBUG_MODE == 1)
        printf("Datamgr is being freed\n");
    #endif
    dpl_free(&room_map_list,true);
}

uint16_t datamgr_get_room_id(sensor_id_t sensor_id){
    uint16_t room_id = 0;
    sensor_t dummy;
    dummy.sensor_id = sensor_id;
    
    sensor_t * sensor = (sensor_t*)dpl_get_element_at_index(room_map_list,dpl_get_index_of_element(room_map_list,&dummy));

    if(sensor == NULL)
    ERROR_HANDLER(sensor == NULL,INVALID_ERROR);
    room_id = sensor->room_id;
    return room_id;

}

sensor_value_t datamgr_get_avg(sensor_id_t sensor_id){
    double run_avg = 0.0;
    sensor_t dummy;
    dummy.sensor_id = sensor_id;
    
    sensor_t * sensor = (sensor_t*)dpl_get_element_at_index(room_map_list,dpl_get_index_of_element(room_map_list,&dummy));

    if(sensor == NULL)
    ERROR_HANDLER(sensor == NULL,INVALID_ERROR);
    run_avg = sensor->running_avg;
    return run_avg;
}

time_t datamgr_get_last_modified(sensor_id_t sensor_id){
    time_t last_mod;
    sensor_t dummy;
    dummy.sensor_id = sensor_id;
    
    sensor_t * sensor = (sensor_t*)dpl_get_element_at_index(room_map_list,dpl_get_index_of_element(room_map_list,&dummy));

    if(sensor == NULL)
    ERROR_HANDLER(sensor == NULL,INVALID_ERROR);

    last_mod = sensor->last_modified;
    return last_mod;
}
int datamgr_get_total_sensors(){
    return dpl_size(room_map_list);


}

//CALL BACK FUNCTIONS
void *element_copy(void*element){
    sensor_t*sensor = (sensor_t*)element;
    sensor_t * copy = malloc(sizeof(sensor_t));
    copy->sensor_id = sensor->sensor_id;
    copy->room_id = sensor->room_id;
    copy->running_avg = sensor->running_avg;
    copy->last_modified = sensor->last_modified;
    for(int i = 0; i < RUN_AVG_LENGTH; i++){
        copy->temperatures[i]=sensor->temperatures[i];
    }
    return (void*)copy;

}
void element_free(void**element){
    sensor_t * dummy = *element;
    free(dummy);


}
int element_compare(void *x,void*y){
    return ((((sensor_t*)x)->sensor_id < ((sensor_t*)y)->sensor_id) ? -1:(((sensor_t*)x)->sensor_id == ((sensor_t*)y)->sensor_id)? 0:1);
        

}
