/**
 * \author Kerem Okyay
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include "sbuffer.h"
#include<pthread.h>
#include<string.h>

static int seq_no = 0;
extern char connmgr_state;
/**
 * basic node for the buffer, these nodes are linked together to create the buffer
 */
typedef struct sbuffer_node {
    struct sbuffer_node *next;  /**< a pointer to the next node*/
    sensor_data_t data;         /**< a structure containing the data */
    int read_by[READERS];
} sbuffer_node_t;

/**
 * a structure to keep track of the buffer
 */
struct sbuffer {
    sbuffer_node_t *head;       /**< a pointer to the first node in the buffer */
    sbuffer_node_t *tail;       /**< a pointer to the last node in the buffer */
    pthread_mutex_t * lock;
    pthread_mutex_t * fifo_lock;
    FILE * fifo;
    pthread_cond_t * cond;
};

static int read_by_all(sbuffer_node_t*);

int sbuffer_init(sbuffer_t **buffer) {
    *buffer = malloc(sizeof(sbuffer_t));
    if (*buffer == NULL) return SBUFFER_FAILURE;
    (*buffer)->fifo = fopen(FIFO_NAME,"w");
    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;
	(*buffer)->lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    (*buffer)->fifo_lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    (*buffer)->cond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
	pthread_mutex_init((*buffer)->lock,NULL);
    pthread_mutex_init((*buffer)->fifo_lock,NULL);
    pthread_cond_init((*buffer)->cond,NULL);
    #if(DEBUG_MODE ==1)
        printf("Sbuffer is initialized\n");
    #endif

    return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **buffer) {

    sbuffer_node_t *dummy;
    if ((buffer == NULL) || (*buffer == NULL)) {
        return SBUFFER_FAILURE;
    }
    fclose((*buffer)->fifo);
    pthread_cond_destroy((*buffer)->cond);
    pthread_mutex_destroy((*buffer)->lock);
    pthread_mutex_destroy((*buffer)->fifo_lock);
	free((*buffer)->lock);
	free((*buffer)->fifo_lock);
	free((*buffer)->cond);
    while ((*buffer)->head) {
        dummy = (*buffer)->head;
        (*buffer)->head = (*buffer)->head->next;
        free(dummy);
    }
    free(*buffer);
    *buffer = NULL;
    #if(DEBUG_MODE ==1)
        printf("Sbuffer is freed\n");
    #endif
    return SBUFFER_SUCCESS;
}

int sbuffer_remove(sbuffer_t *buffer) {
    sbuffer_node_t *dummy;
    if (buffer == NULL) return SBUFFER_FAILURE;
    if (buffer->head == NULL) return SBUFFER_NO_DATA;
    dummy = buffer->head;
    if (buffer->head == buffer->tail) // buffer has only one node
    {
        buffer->head = buffer->tail = NULL;
    } else  // buffer has many nodes empty
    {
        buffer->head = buffer->head->next;
    }
    free(dummy);
    return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data) {
    sbuffer_node_t *dummy;
    if (buffer == NULL) return SBUFFER_FAILURE;
    dummy = malloc(sizeof(sbuffer_node_t));
    if (dummy == NULL) return SBUFFER_FAILURE;
    dummy->data = *data;
    dummy->next = NULL;
    for(int i=0;i<READERS;i++)dummy->read_by[i]=0;
    pthread_mutex_lock(buffer->lock);
    if (buffer->tail == NULL&&buffer->head == NULL) // buffer empty (buffer->head should also be NULL
    {
        buffer->head = buffer->tail = dummy;
    } else // buffer not empty
    {
        buffer->tail->next = dummy;
        buffer->tail = buffer->tail->next;
    }
    #if(DEBUG_MODE ==1)
        printf("New element is inserted with id %d and value %f\n",data->id,data->value);
    #endif
    pthread_mutex_unlock(buffer->lock);
    pthread_cond_broadcast(buffer->cond);
    return SBUFFER_SUCCESS;
}
int sbuffer_read(sbuffer_t * buffer, void ** node, sensor_data_t * data, int index){
    if(buffer == NULL){
        return SBUFFER_FAILURE;
    }
    if(buffer->head == NULL){
        return SBUFFER_NO_DATA;
    }
    pthread_mutex_lock(buffer->lock);
    if(connmgr_state==1)
    pthread_cond_wait(buffer->cond,buffer->lock);
    if((*node)==NULL)(*node)=buffer->head;
    
    if(((sbuffer_node_t*)(*node))->read_by[index]==1){
        pthread_mutex_unlock(buffer->lock);
        return SBUFFER_NO_NEW_DATA;
    }
    *data = ((sbuffer_node_t*)(*node))->data;
    ((sbuffer_node_t*)(*node))->read_by[index]=1;
    if(((sbuffer_node_t*)(*node))->next != NULL){

        (*node)=((sbuffer_node_t*)(*node))->next;

    }
    if(read_by_all(buffer->head)==1){
        #if(DEBUG_MODE ==1)
            printf("Buffer head is removed with id %d and value %f\n",data->id,data->value);
        #endif
        sbuffer_remove(buffer);
        (*node)=NULL;
     
    }
    pthread_mutex_unlock(buffer->lock);
    if(connmgr_state == 1)
    pthread_cond_broadcast(buffer->cond);
    return SBUFFER_SUCCESS;
    

}
static int read_by_all(sbuffer_node_t * node){
    if(node->read_by[0]==1 && node->read_by[1]==1){
        return 1;
    }
    return 0;
}
void print_sbuffer(sbuffer_t * buffer){
    pthread_mutex_lock(buffer->lock);
    if(buffer->head == NULL && buffer->tail == NULL){
        printf("empty sbuffer\n");
    }

    sbuffer_node_t * node = buffer->head;
    int i = 0;
    while(node != NULL){
        printf("\nindex %d-id %d reader1 %d reader2 %d\n",i,node->data.id,node->read_by[0],node->read_by[1]);
        node = node->next;
        i++;
    }
    pthread_mutex_unlock(buffer->lock);
}
int sbuffer_size(sbuffer_t * buffer){
    pthread_mutex_lock(buffer->lock);
    if(buffer->head == NULL){
        pthread_mutex_unlock(buffer->lock);
        return 0;
    }

    sbuffer_node_t * node = buffer->head;
    int i = 0;
    while(node != NULL){
        node = node->next;
        i++;
    }
    pthread_mutex_unlock(buffer->lock);
    return i;
    
}

void sbuffer_write_fifo(sbuffer_t * buffer,char * log_txt){
    pthread_mutex_lock(buffer->fifo_lock);
    if(strlen(log_txt)!=0){
        char * buff;
        asprintf(&buff, "%d: %s", seq_no++, log_txt);
        if(fputs(buff,buffer->fifo)==EOF){
            exit(EXIT_FAILURE);
        }
        free(buff);

    }
    pthread_mutex_unlock(buffer->fifo_lock);

}
void sbuffer_signal(sbuffer_t * buffer){
    pthread_cond_broadcast(buffer->cond);
}





