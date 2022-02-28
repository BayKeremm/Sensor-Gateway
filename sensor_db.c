#define DBCONN sqlite3
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <sqlite3.h>
#include "sensor_db.h"
#include "sbuffer.h"

extern char storagemgr_fail_flag;
char connection_established=0;
extern char connmgr_state;

DBCONN * init_connection(char clear_up_flag,sbuffer_t * sbuffer){
    char * log_msg;
    DBCONN * db;
    char * err_msg = 0;
    char sql[200];
    int rc = sqlite3_open(TO_STRING(DB_NAME),&db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
                                    
    }
    #if(DEBUG_MODE==1)
        printf("Connection to SQL server is established\n");
    #endif
    asprintf(&log_msg,"Connection to SQL server established.\n");
    sbuffer_write_fifo(sbuffer,log_msg);
    free(log_msg);
    if(clear_up_flag == 1){
    snprintf(sql,200,"DROP TABLE IF EXISTS "TO_STRING(TABLE_NAME) ";"
                 "CREATE TABLE "TO_STRING(TABLE_NAME)"(id INTEGER PRIMARY KEY ASC AUTOINCREMENT,sensor_id INTEGER,sensor_value DECIMAL(4,2),timestamp TIMESTAMP);");
    #if(DEBUG_MODE==1)
        printf("New table is created the old one is droped\n");
    #endif
    

    }else{
    snprintf(sql,200,"CREATE TABLE IF NOT EXISTS "TO_STRING(TABLE_NAME)";"
                 "CREATE TABLE "TO_STRING(TABLE_NAME)"(id INTEGER PRIMARY KEY ASC AUTOINCREMENT,sensor_id INTEGER,sensor_value DECIMAL(4,2),timestamp TIMESTAMP);");
    asprintf(&log_msg,"New table"TO_STRING(TABLE_NAME)"created");
    sbuffer_write_fifo(sbuffer,log_msg);
    free(log_msg);
        
    } 
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        #if(DEBUG_MODE==1)
            printf("SQL error\n");
        #endif
        return NULL;
    }
    connection_established=1;
    return db;
}

void disconnect(DBCONN * conn,sbuffer_t * sbuffer){
    #if(DEBUG_MODE==1)
        printf("SQL server is being disconnected\n");
    #endif
    char * log_msg;
    sqlite3_close(conn);
    asprintf(&log_msg,"Connection to SQL server lost.\n");
    sbuffer_write_fifo(sbuffer,log_msg);
    free(log_msg);

}
int insert_sensor(DBCONN *conn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts){
    char * err_msg = 0;
    char sql[200];
    snprintf(sql,200,"INSERT INTO "TO_STRING(TABLE_NAME)" (sensor_id,sensor_value,timestamp) VALUES(%hu,%lf,%ld);",id,value,ts);
    int rc = sqlite3_exec(conn,sql,0,0,&err_msg);

    if (rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);        
        sqlite3_close(conn);
        return 1;
    } 
    #if(DEBUG_MODE==1)
        printf("Data is inserted successfuly, %hu,%lf,%ld\n",id,value,ts);
    #endif

    return 0;
}

int insert_sensor_from_data(DBCONN *conn, sbuffer_t ** sbuffer){
    sensor_data_t databuffer;

    void * node = NULL;
    int sbuffer_rc = SBUFFER_SUCCESS;
    while(sbuffer_rc != SBUFFER_FAILURE&&storagemgr_fail_flag == 0){
        sbuffer_rc = sbuffer_read(*sbuffer,&node,&databuffer,STORAGEMGR_INDEX);
        if(connmgr_state==0&&(sbuffer_rc == SBUFFER_NO_DATA||sbuffer_rc == SBUFFER_NO_NEW_DATA))break;
        if(sbuffer_rc == SBUFFER_SUCCESS)
            insert_sensor(conn,databuffer.id,databuffer.value,databuffer.ts);
        

    }

    return 0;
}
int find_sensor_all(DBCONN *conn, callback_t f){

    char * err_msg = 0;
    char sql[100];
    snprintf(sql,100,"SELECT * from %s;",TO_STRING(TABLE_NAME));

    int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
    if (rc != SQLITE_OK ) {
                
        fprintf(stderr, "Failed to select data\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(conn);
                                                
        return 1;
                                                    
    }
        return 0;
}

int find_sensor_by_value(DBCONN *conn, sensor_value_t value, callback_t f){
    char * err_msg = 0;
    char sql[100];
    snprintf(sql,100,"SELECT * FROM %s WHERE sensor_value = %lf;",TO_STRING(TABLE_NAME),value);
    int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
    if (rc != SQLITE_OK ) {
                
        fprintf(stderr, "Failed to select data for value %lf \n",value);
        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(conn);
                                                
        return 1;
                                                    
    }
    return 0;
}
int find_sensor_exceed_value(DBCONN *conn, sensor_value_t value, callback_t f){
    char * err_msg = 0;
    char sql[100];
    snprintf(sql,100,"SELECT * FROM %s WHERE sensor_value > %lf;",TO_STRING(TABLE_NAME),value);
    int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
    if (rc != SQLITE_OK ) {
                
        fprintf(stderr, "Failed to select data larger than value %lf \n",value);
        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(conn);
                                                
        return 1;
                                                    
    }
    return 0;

}
int find_sensor_by_timestamp(DBCONN *conn, sensor_ts_t ts, callback_t f){
    char * err_msg = 0;
    char sql[100];
    snprintf(sql,100,"SELECT * FROM %s WHERE timestamp = %ld;",TO_STRING(TABLE_NAME),ts);
    int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
    if (rc != SQLITE_OK ) {
                
        fprintf(stderr, "Failed to select data timestamp\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(conn);
                                                
        return 1;
                                                    
    }
    return 0;
    
}
int find_sensor_after_timestamp(DBCONN *conn, sensor_ts_t ts, callback_t f){
    char * err_msg = 0;
    char sql[100];
    snprintf(sql,100,"SELECT * FROM %s WHERE timestamp > %ld;",TO_STRING(TABLE_NAME),ts);
    int rc = sqlite3_exec(conn,sql,f,0,&err_msg);
    if (rc != SQLITE_OK ) {
                
        fprintf(stderr, "Failed to select data timestamp\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(conn);
                                                
        return 1;
                                                    
    }
    return 0;

}
