Overview
========

You can use the configuration file to decide which ports to listen on.  
These ports include tcp, udp and ssl.  
You can configure the number of worker threads of the framework and concurrently process the data received and sent from the IO.

You can create a configuration file yourself, or use the configuration file provided by the framework and modify it.  
If you use the framework's own configuration file, you need to modify it:  
(Build\Linux\server_config.json) or (Build\Windows\server_config.json)  
If you use a custom configuration file, you need to run it:  
./pss_asio server_config_test_gcov.json(your configuration file)  

Take server_config.json as an example:  

 - [work thread](#work-thread)


Work thread
===========
| name | type | Description |  
| ------ | ------ | ------ |  
| linux daemonize | int | Whether the process enters the background execution after starting under Linux |  
| work thread count | int | Number of framework worker threads |  
| work time check | int | Deadlock check period of the worker thread, in seconds |  
| server to server time check | int | The IO link check cycle between servers, in seconds |  
| client connect timeout | int | The period for detecting client IO timeout, in seconds |  
