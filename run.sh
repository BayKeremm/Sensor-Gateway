
osascript -e 'tell app "Terminal"
    do script "cd myStuff/university/systemsoftware/lab_final;
                ./sensor_gateway 1111"
    end tell'

osascript -e 'tell app "Terminal"
    do script "cd myStuff/university/systemsoftware/lab_final;
               ./sensor_node 21 1 127.0.0.1 1111"
    end tell'

osascript -e 'tell app "Terminal"
    do script "cd myStuff/university/systemsoftware/lab_final;
               ./sensor_node 15 2 127.0.0.1 1111"
    end tell'
osascript -e 'tell app "Terminal"
    do script "cd myStuff/university/systemsoftware/lab_final;
               ./sensor_node 37 3 127.0.0.1 1111"
    end tell'
osascript -e 'tell app "Terminal"
    do script "cd myStuff/university/systemsoftware/lab_final;
               ./sensor_node 112 2 127.0.0.1 1111"
    end tell'
osascript -e 'tell app "Terminal"
    do script "cd myStuff/university/systemsoftware/lab_final;
               ./sensor_node 129 5 127.0.0.1 1111"
    end tell'
