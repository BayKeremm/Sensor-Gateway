# Sensor-Gateway
The goal of the project is to stimulate a sensor network in a building where rooms contain temperature sensors throughout.\
The TCP server runs and accepts TCP connection from the running sensor nodes.\
Program generates 3 threads namely data manager, connection manager, and storage manager.\
Data manager is the internal decision making of the program where the running average is calculated and necessary warnings are generated.\
Connection manager accepts incoming TCP connections and drops the inactive TCP connections. \
Storage manager stores the values in a local SQL server. 

All three threads share a datastructure (linked list) where it is made thread-safe.
