Overview
========

A plug-in that provides business processing capabilities.  
The main task of the logic plug-in is to subscribe to the framework for a specified message ID and provide business processing for this message.  

You can refer to Test_Logic(Module_Logic\Test_Logic) I wrote.  

Table of Contents(function info)
================================
 - [load_module](#load_module)
 - [unload_module](#unload_module)
 - [do_module_message](#do_module_message)
 - [module_state](#module_state)
 - [set_output](#set_output)

load_module
===========
Function description: The plugin is loaded by the framework event.  
| name | type | Description |  
| ------ | ------ | ------ |  
| frame_object | object | Interface parameters provided by the framework |  
| module_param | string | Plug-in loading parameters from the configuration file |  

unload_module
=============
Function description: The plugin is unloaded by the framework event.  

do_module_message
=================
Function description: Receive message events from plugin subscriptions.  
| name | type | Description |  
| ------ | ------ | ------ |  
| source | object | IO content corresponding to the source of the message event |  
| recv_packet | message | Receive message packet |  
| send_packet | message | send message packet(Synchronize) |  

module_state
============
Function description: Allow the framework to query the current state of the plug-in (such as whether it is healthy).  

set_output
==========
Function description: Set the interface for synchronous output with the framework (no need to modify the code).  

| name | type | Description |  
| ------ | ------ | ------ |  
| logger | object | Frame's logger object |  
