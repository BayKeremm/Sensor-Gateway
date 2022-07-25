# Sensor-Gateway

## Overview
<img width="589" alt="image" src="https://user-images.githubusercontent.com/82160210/180779257-73a0a627-cad8-4e11-9d8e-6ff59dfe4b57.png">



<img width="669" alt="image" src="https://user-images.githubusercontent.com/82160210/180779131-a91481ca-25a8-414c-990b-09e54c580192.png">

The goal of the project is to stimulate a sensor network in a building where rooms contain temperature sensors throughout.\
The TCP server runs and accepts TCP connection from the running sensor nodes.\
Program generates 3 threads namely data manager, connection manager, and storage manager.\
Data manager is the internal decision making of the program where the running average is calculated and necessary warnings are generated.\
Connection manager accepts incoming TCP connections and drops the inactive TCP connections. \
Storage manager stores the values in a local SQL server. 

All three threads share a datastructure (linked list) where it is made thread-safe.
