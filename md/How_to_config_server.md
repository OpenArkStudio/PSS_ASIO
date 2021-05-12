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

Table of Contents
=================
 - [work thread](#work-thread)
 - [packet parse library](#packet-parse-library)
 - [logic library](#logic-library)
 - [tcp server](#tcp-server)
 - [udp server](#udp-server)
 - [tty server](#tty-server)
 - [console output](#console-output)


Work thread
===========
| name | type | Description |  
| ------ | ------ | ------ |  
| linux daemonize | int | Whether the process enters the background execution after starting under Linux |  
| work thread count | int | Number of framework worker threads |  
| work time check | int | Deadlock check period of the worker thread, in seconds |  
| server to server time check | int | The IO link check cycle between servers, in seconds |  
| client connect timeout | int | The period for detecting client IO timeout, in seconds |  

packet parse library
====================
NOTE: You can set up multiple packet parse plug-ins to load and bind and analyze different IO ports. Please note that your "packet parse id" is not repeatable.  
| name | type | Description |  
| ------ | ------ | ------ |  
| packet parse id | int | ID of a data parser (globally unique) |  
| packet parse path | string | The path of the dynamic library of the data parser |  
| packet parse file | string | The file name of the dynamic library of the data parser |  

logic library
=============
NOTE: You can mount multiple logic module dynamic libraries according to your needs.  
| name | type | Description |  
| ------ | ------ | ------ |  
| logic path | string | The path where the logic module dynamic library is located |  
| logic file | string | The file name of the logic module dynamic library |  
| logic param | string | When the plug-in is loaded, this parameter will be transmitted to the function interface initialized by the plug-in |  

tcp server
==========
NOTE: You can monitor multiple TCP ports at the same time. And for each port, please configure "packet parse id".  
| name | type | Description |  
| ------ | ------ | ------ |  
| tcp ip | string | IP address monitored by tcp |  
| tcp port | int |  IP port monitored by tcp |  
| packet parse id | int | ID of a data parser |  
| recv buff size | int | The maximum size of the received data packet. The unit is byte  |  
| send buff size | int | The maximum size of the send data packet. The unit is byte |  
| ssl server password(Optional settings) | string | ssl pem file password |  
| ssl server pem file(Optional settings) | string | The location of the ssl server pem file |  
| ssl dh pem file(Optional settings) | string | The location of the ssl dh pem file |  

udp server
==========
NOTE: You can monitor multiple UDP ports at the same time. And for each port, please configure "packet parse id".  
| name | type | Description |  
| ------ | ------ | ------ |  
| udp ip | string | IP address monitored by udp |  
| udp port | int |  IP port monitored by udp |  
| packet parse id | int | ID of a data parser |  
| recv buff size | int | The maximum size of the received data packet. The unit is byte  |  
| send buff size | int | The maximum size of the send data packet. The unit is byte |  

tty server
==========
NOTE: You can monitor multiple tty ports at the same time. And for each port, please configure "packet parse id".  
| name | type | Description |  
| ------ | ------ | ------ |  
| port name | string | serial port name |  
| serial port | int |  Port baud rate |  
| char size | int |  port byte size |  
| packet parse id | int | ID of a data parser |  
| recv buff size | int | The maximum size of the received data packet. The unit is byte  |  
| send buff size | int | The maximum size of the send data packet. The unit is byte |  

console output
==============
| name | type | Description |  
| ------ | ------ | ------ |  
| file write | boolean | Whether to output the output of the screen to a file |  
| log file count | int | Number of log file cycles |  
| max log file size | int | Maximum size of a single log file |  
| file name | int | The path and file name of the log file |  
| output level | int | Output log level: you can choose info,debug,warn,error |  
