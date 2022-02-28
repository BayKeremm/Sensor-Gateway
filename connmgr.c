#define _GNU_SOURCE
#include "connmgr.h"
#include <stdio.h>
#include "lib/dplist.h"
#include "config.h"
#include <stdlib.h>
#include <poll.h>
#include "lib/tcpsock.h"
#include <inttypes.h>
#include <time.h>


extern char storagemgr_fail_flag;
extern char connmgr_state;

void element_free_c(void**);
int element_compare_c(void *x,void*y);
void* element_copy_c(void*);
void update_fdlist(int,bool);

typedef struct {
    time_t last_active;
    tcpsock_t *socket;
    int sensor_id;

}connection_t;

static dplist_t *conn_list;
// init poll struct
static struct pollfd * fd_list;

void connmgr_listen(int port_number,sbuffer_t ** sbuffer){
    tcpsock_t *server;
    connection_t server_conn;
    sensor_data_t data;
    int i,bytes,result;
    char * log_msg;

    conn_list = dpl_create(&element_copy_c,&element_free_c,&element_compare_c);
    //init fd array
    fd_list = (struct pollfd *)malloc(sizeof(struct pollfd));

    // init listening server.
    if(tcp_passive_open(&server, port_number) != TCP_NO_ERROR)exit(EXIT_FAILURE);

    // get the server's file descriptor at position 0
    if(tcp_get_sd(server,&(fd_list[0].fd)) != TCP_NO_ERROR)exit(EXIT_FAILURE);
    fd_list[0].events = POLLIN;


    server_conn.last_active = time(NULL);
    server_conn.socket = server;


    conn_list = dpl_insert_at_index(conn_list,(void*)(&server_conn),-1,true);

    char alive = 1;

    // infinite loop
    while(alive&&storagemgr_fail_flag == 0){

        int size = dpl_size(conn_list);
        #if(DEBUG_MODE == 1)
            printf("Amount of active connections %d\n",size);
        #endif
        connection_t * curr_conn;
        int rc = poll(fd_list,size,TIMEOUT*1000);

        if(rc > 0){
            if(fd_list[0].revents & POLLIN){
                
                #if(DEBUG_MODE == 1)
                    printf("New connection to the server\n");
                #endif

                tcpsock_t * client;
                connection_t new_conn;
                if(tcp_wait_for_connection(server,&client) != TCP_NO_ERROR)exit(EXIT_FAILURE);
                new_conn.socket = client;
                conn_list = dpl_insert_at_index(conn_list,(void*)(&new_conn),999,true);
                update_fdlist(1,0);
                int conn_count = dpl_size(conn_list);
                if(tcp_get_sd(client,&(fd_list[conn_count-1].fd)) != TCP_NO_ERROR)exit(EXIT_FAILURE);
                fd_list[conn_count-1].events = POLLIN | POLLHUP;
                new_conn.sensor_id = 0;
                continue;
            }
            for(i=1;i<dpl_size(conn_list);i++){
                curr_conn = dpl_get_element_at_index(conn_list,i);
                if(fd_list[i].revents & POLLIN){
                    bytes = sizeof(data.id);
                    result = tcp_receive(curr_conn->socket, (void *) &(data.id), &bytes);
                    bytes = sizeof(data.value);
                    result = tcp_receive(curr_conn->socket, (void *) &data.value, &bytes);
                    bytes = sizeof(data.ts);
                    result = tcp_receive(curr_conn->socket, (void *) &data.ts, &bytes);
                    if((result == TCP_NO_ERROR) && bytes) {
                        if(curr_conn->sensor_id == 0){
                            curr_conn->sensor_id = data.id;
                            asprintf(&log_msg,"A sensor node with %d has opened a new connection\n",curr_conn->sensor_id);
                            sbuffer_write_fifo(*sbuffer,log_msg);
                            free(log_msg);
                            
                        }
                        sbuffer_insert(*sbuffer,&data);
                        
                        curr_conn->last_active = time(NULL);
                    } else if(result == TCP_CONNECTION_CLOSED) {
                        fd_list[i].events = -1;
                    }
                }

                if(fd_list[i].revents & POLLHUP || fd_list[i].events == -1 ){
                    #if(DEBUG_MODE == 1)
                        printf("Sensor node %d closed connection\n",curr_conn->sensor_id);
                    #endif
                    asprintf(&log_msg,"The sensor node with %d has closed the connection\n",curr_conn->sensor_id);
                    sbuffer_write_fifo(*sbuffer,log_msg);
                    free(log_msg);
                    tcp_close(&(curr_conn->socket));
                    update_fdlist(i,1);
                    conn_list = dpl_remove_at_index(conn_list,i,true);
                }
            }//end of for loop
                // NOW CHECK FOR THE TIMEOUTS
                if(dpl_size(conn_list)>=2){
                int j;
                for(j = 1; j<dpl_size(conn_list);j++){
                    curr_conn = dpl_get_element_at_index(conn_list,j);
                    if(time(NULL)-curr_conn->last_active>TIMEOUT){
                        #if(DEBUG_MODE == 1)
                            printf("Sensor node %d is closed due to timeout\n",curr_conn->sensor_id);
                        #endif
                        tcp_close(&(curr_conn->socket));
                        update_fdlist(j,true);
                        conn_list = dpl_remove_at_index(conn_list,j,true);
                    }
                }
                }
        }
        if(rc == 0){
            //printf("closing server inactive for some time\n");

            if(dpl_size(conn_list) == 1){
            #if(DEBUG_MODE == 1)
                printf("Server is closing\n\n");
            #endif
            tcp_close(&(server_conn.socket));
            connmgr_free();
            // free the used structure and close the file
            free(fd_list);
            alive = 0;
            connmgr_state = 0;

            }
            if(dpl_size(conn_list)>=2){
            int j;
            for(j = 1; j<dpl_size(conn_list);j++){
                curr_conn = dpl_get_element_at_index(conn_list,j);
                if(time(NULL)-curr_conn->last_active>TIMEOUT){
                    #if(DEBUG_MODE == 1)
                        printf("Sensor node %d is closed due to timeout\n",curr_conn->sensor_id);
                    #endif
                    tcp_close(&(curr_conn->socket));
                    update_fdlist(j,true);
                    conn_list = dpl_remove_at_index(conn_list,j,true);
                }
            }
            }






        }
    }
}

void update_fdlist(int position, bool shift_flag){
    
    int n;
    if(shift_flag == true){
    for(n = 0; n < dpl_size(conn_list);n++){
        if(n >= position && n+1 < dpl_size(conn_list)){
            fd_list[n] = fd_list[n+1];
        }
    }
    }
    fd_list = (struct pollfd *)realloc(fd_list,sizeof(struct pollfd)*dpl_size(conn_list));
    if(fd_list == NULL){
        free(fd_list);
        exit(EXIT_FAILURE);
    }
    return;

}


void connmgr_free(){
    dpl_free(&conn_list,true);

}



//CALL BACK FUNCTIONS
void *element_copy_c(void*element){
    connection_t * copy = malloc(sizeof(connection_t));
    copy->socket = ((connection_t * )element)->socket;
    copy->last_active = ((connection_t *)element)->last_active;
    return (void*)copy;

}
void element_free_c(void**element){
    free(*element);

}
int element_compare_c(void *x,void*y){

    return 1;
}
