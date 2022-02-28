#define _GNU_SOURCE
#include "connmgr.h"
#include "sensor_db.h"
#include"datamgr.h"
#include<pthread.h>
#include<stdio.h>
#include"sbuffer.h"
#include<stdlib.h>
#include"errmacros.h"
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include<poll.h>
#include<string.h>
#define MAX 200

char connmgr_state=1;
char storagemgr_fail_flag=0;

sbuffer_t * sbuffer;


struct parent{
    int port_no;
    pid_t parent_id;
};


void fifo_read();
void*datamgr(void*);
void*storagemgr(void*);
void*connmgr(void*);
void run_child(pid_t);
void run_parent(struct parent);
void print_help(void);

int main(int argc,char*argv[]){

    parent_data pdata;

    if(argc != 2){
        print_help();
        exit(EXIT_SUCCESS);
    }else{
        pdata.port_no =atoi(argv[1]);

    }

    pid_t child_pid;
    pdata.parent_id = getpid();
    child_pid = fork();
    SYSCALL_ERROR(child_pid);
    

    if(child_pid == 0){
        run_child(child_pid);
    }else{
        run_parent(pdata);
    }
    return 0;
}

void run_parent(struct parent data){
    char * log_msg;
    int result = mkfifo(FIFO_NAME,0666);
    CHECK_MKFIFO(result);
    
    sbuffer_init(&sbuffer);

    pthread_t thread_datamgr,thread_connmgr,thread_storagemgr;
    pthread_create(&thread_connmgr,NULL,connmgr,&(data.port_no));
    pthread_create(&thread_datamgr,NULL,datamgr,NULL);
    pthread_create(&thread_storagemgr,NULL,storagemgr,NULL);
    
    pthread_join(thread_connmgr,NULL);
    pthread_join(thread_datamgr,NULL);
    pthread_join(thread_storagemgr,NULL);
    #if(DEBUG_MODE==1)
        printf("All threads joined\n");
    #endif
    
    asprintf(&log_msg,"Sensor_gateway_end");
    sbuffer_write_fifo(sbuffer,log_msg);
    
    free(log_msg);
    
    #if(DEBUG_MODE==1)
        print_sbuffer(sbuffer);
    #endif
    sbuffer_free(&sbuffer);
    
    exit(EXIT_SUCCESS);
}
void run_child(pid_t child_pid){
    int result = mkfifo(FIFO_NAME,0666);
    CHECK_MKFIFO(result);
    fifo_read(); 
    exit(EXIT_SUCCESS);
}



void * datamgr(void*arg){//index 0
    int rc;
    FILE * fp = fopen("room_sensor.map","r");
    FILE_OPEN_ERROR(fp);
    datamgr_parse_sensor_data(fp,&sbuffer);
    datamgr_free();
    rc = fclose(fp);
    FILE_CLOSE_ERROR(rc);
    #if(DEBUG_MODE==1)
        printf("Datamgr is ending\n");
    #endif

    pthread_exit(NULL);
}
void * storagemgr(void *arg){//index 1

    char * log_msg;
    for(int i = 0;i<3;i++){
        DBCONN * strConn;
        strConn = init_connection(1,sbuffer);
        if(strConn != NULL){
            insert_sensor_from_data(strConn,&sbuffer);
            disconnect(strConn,sbuffer);
            #if(DEBUG_MODE==1)
                printf("Storagemgr is ending\n");
            #endif
            pthread_exit(NULL);
        }

        sleep(1);
    }
    #if(DEBUG_MODE==1)
        printf("Storage manager cannot establish connection\n");
    #endif

    storagemgr_fail_flag = 1;
    asprintf(&log_msg,"Unable to connect to SQL server.\n");
    sbuffer_write_fifo(sbuffer,log_msg);
    free(log_msg);
    pthread_exit(NULL);
}
void *connmgr(void*arg){
    int port = *((int *)(arg));
    connmgr_listen(port,&sbuffer);
    connmgr_free();
    sbuffer_signal(sbuffer);
    #if(DEBUG_MODE==1)
        printf("Connmgr is ending\n");
    #endif
    pthread_exit(NULL);
}


void fifo_read(){
	int fd_fifo,nread;
	char buf[MAX];
	fd_fifo = open(FIFO_NAME,O_RDONLY);
	FILE * log = fopen("gateway.log","w");
	while((nread = read(fd_fifo,buf,MAX))>0){
		fprintf(log,"%s\n",buf);
        memset(buf,0,MAX);
	}
    #if(DEBUG_MODE==1)
        printf("Child process is ending\n");
    #endif
	fclose(log);
	close(fd_fifo);
exit(EXIT_SUCCESS);
}
void print_help(void){
    printf("Please enter a port number\n");
}
