/**
 * \author Kerem Okyay
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"
#include <assert.h>

/*
 * definition of error codes
 * */
#define DPLIST_NO_ERROR 0
#define DPLIST_MEMORY_ERROR 1 // error due to mem alloc failure
#define DPLIST_INVALID_ERROR 2 //error due to a list operation applied on a NULL list 

#ifdef DEBUG
#define DEBUG_PRINTF(...) 									                                        \
        do {											                                            \
            fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);	    \
            fprintf(stderr,__VA_ARGS__);								                            \
            fflush(stderr);                                                                         \
                } while(-1)
#else
#define DEBUG_PRINTF(...) (void)0
#endif


#define DPLIST_ERR_HANDLER(condition, err_code)                         \
    do {                                                                \
            if ((condition)) DEBUG_PRINTF(#condition " failed\n");      \
            assert(!(condition));                                       \
        } while(0)


/*
 * The real definition of struct list / struct node
 */

struct dplist_node {
    dplist_node_t *prev, *next;
    void *element;
};

struct dplist {
    dplist_node_t *head;

    void *(*element_copy)(void *src_element);

    void (*element_free)(void **element);

    int (*element_compare)(void *x, void *y);
};


dplist_t *dpl_create(// callback functions
        void *(*element_copy)(void *src_element),
        void (*element_free)(void **element),
        int (*element_compare)(void *x, void *y)
) {
    dplist_t *list;
    list = malloc(sizeof(struct dplist));
    DPLIST_ERR_HANDLER(list == NULL, DPLIST_MEMORY_ERROR);
    list->head = NULL;
    list->element_copy = element_copy;
    list->element_free = element_free;
    list->element_compare = element_compare;
    return list;
}

void dpl_free(dplist_t **list, bool free_element) {

    //TODO: add your code here
    
    int i;
    if(*list == NULL){
        return;
    }
    dplist_node_t *ptr ;
    dplist_t *dummylist = *list;
    int size = dpl_size(dummylist);
    if(dummylist->head == NULL){
        *list = NULL;
        return;
    }
    ptr = dummylist->head;
    for(i = 0;i<size;i++){
        if(free_element==true){
            dummylist->element_free((&ptr->element));
        }
        dplist_node_t *tbfreed = ptr; 
        ptr = ptr->next;
        free(tbfreed);
        tbfreed = NULL;
    }
    *list = NULL;
    free(dummylist);
    dummylist=NULL;
}

dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy) {

    //TODO: add your code here
    //cases for insert --> beginning of the list, at the end of the list, middle of the list. 
    //stuff to be careful--> negative index, index larger than the size. 
    int size = dpl_size(list);
    int max_index = size -1;
    if(list == NULL)return NULL;
    if(element == NULL)return list;

    dplist_node_t *ref_at_index,*node;
    node = malloc(sizeof(dplist_node_t));
    DPLIST_ERR_HANDLER(node == NULL,DPLIST_MEMORY_ERROR);

    if(insert_copy == true){
        node->element = list->element_copy(element);

    }else{
        node->element = element;
    }
    //handles empty list insertion
    if(list->head == NULL){
        node->prev = NULL;
        node->next = NULL;
        list->head = node;
    }
    //handles negative index 
    else if( index <= 0){
        node->prev = NULL;
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }
    else{  
        if(index < size-1){
            ref_at_index = dpl_get_reference_at_index(list,index);
            assert(ref_at_index != NULL);
            node->prev = ref_at_index->prev;
            node->next = ref_at_index;
            ref_at_index->prev->next = node;
            ref_at_index->prev=node;
        }else{
            ref_at_index = dpl_get_reference_at_index(list,max_index);
            assert(ref_at_index != NULL);
            assert(ref_at_index->next == NULL);
            ref_at_index->next = node;
            node->prev = ref_at_index;
            node->next = NULL;
        }
    }
    return list;
}

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element) {

    //TODO: add your code here
    int size = dpl_size(list);
    int max_index = size -1;
    dplist_node_t *ref_at_index, *ptr;
    DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);

    if(list->head == NULL)return list;

    if(size ==1){//size 1 has special case
        dplist_node_t * pointer = list->head;
        if(free_element == true)list->element_free(&(pointer->element));
        free(pointer);
        list->head = NULL;
        
                            
    }
    else if(index<=0){
        //beginning
        ptr = list->head;
        assert(ptr->next->prev != NULL);
        ptr->next->prev =NULL;
        list->head = ptr->next;
        if(free_element == true)list->element_free(&(ptr->element));
        free(ptr);
                                                    
        }else{
            
            if(index < size-1){
                //in between
                ref_at_index = dpl_get_reference_at_index(list,index);
                assert(ref_at_index != NULL);
                ref_at_index->prev->next = ref_at_index->next;
                ref_at_index->next->prev = ref_at_index->prev;
                if(free_element == 1)list->element_free(&(ref_at_index->element));
                free(ref_at_index);
                                                        
            }else{
                //at the end
                ref_at_index = dpl_get_reference_at_index(list,max_index);
                assert(ref_at_index != NULL);
                assert(ref_at_index->next == NULL);
                ref_at_index->prev->next = NULL;
                if(free_element == true)list->element_free(&(ref_at_index->element));
                free(ref_at_index);
                                                          
            }                        
            }

    return list;
}

int dpl_size(dplist_t *list) {

    //TODO: add your code here
    int size =0;
    if(list == NULL){
        return -1;
    }
    else if(list->head != NULL){
        dplist_node_t * ptr = list->head;
        while(ptr != NULL){
            ptr = ptr->next;
            size++;
        }
        return size;
    }
return size;
}

void *dpl_get_element_at_index(dplist_t *list, int index) {

    //TODO: add your code here
    int size = dpl_size(list);
    int max_index = size -1;
    dplist_node_t * ptr = NULL;
    if(list == NULL)return NULL;
    else if(list->head == NULL)return NULL;

    else if(index <= 0){
        ptr = list->head;
        return ptr->element;

                                    
    }else if(index >max_index ){
        int i = 0;
        ptr = list->head;
        while(ptr!=NULL){
            if(i == max_index) return ptr->element;
            ptr=ptr->next;
            i++;
        }

    }else{
        int i = 0;
        ptr = list->head;
        while(ptr != NULL){
            if(i == index)return ptr->element;
                ptr = ptr->next;
                i++;                
            }}
    return ptr;


}
int dpl_get_index_of_element(dplist_t *list, void *element) {

    //TODO: add your code here
    DPLIST_ERR_HANDLER(list == NULL,DPLIST_INVALID_ERROR);
    if(element == NULL)return -1;//element is null 
    dplist_node_t * ptr = NULL;
    ptr = list->head;
    int i = 0;
    while(ptr != NULL){
        if(list->element_compare(element,ptr->element)==0){
            return i;
        }
        ptr = ptr->next;
        i++;
    }

    return -1;

}

dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {

    //TODO: add your code here
    int size = dpl_size(list);
    int count;

    dplist_node_t *ptr = NULL;
    DPLIST_ERR_HANDLER(list == NULL,DPLIST_INVALID_ERROR);

    if(list->head == NULL)return NULL;
    else if( index < 0 )return list->head;
    else if(index > size -1){
        for(ptr = list->head, count =0; ptr->next != NULL; ptr = ptr->next,count++){
            if(count == size)return ptr; 

    }
    }
    else{
        for(ptr = list->head, count =0; ptr->next != NULL; ptr = ptr->next,count++){
            if(count >= index)return ptr; 
    }
    }
    return ptr;

}

void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference) {

    //TODO: add your code here
    DPLIST_ERR_HANDLER(list == NULL,DPLIST_INVALID_ERROR);//invalid list
    if(list->head == NULL)return NULL;//empty list
    if(reference == NULL)return NULL;// NULL reference
    dplist_node_t * ptr = list->head;
    while(ptr!=NULL){
        if(reference == ptr)return ptr->element;
        ptr = ptr->next;
    }
    

    return NULL;
}



