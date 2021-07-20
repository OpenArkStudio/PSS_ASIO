[![Github Action](https://github.com/freeeyes/PSS_alpha_test/workflows/PSS_ASIO_CMake/badge.svg)](https://github.com/freeeyes/PSS_alpha_test/actions)
[![C++11need](https://img.shields.io/badge/language-C%2B%2B11-blue.svg)](https://isocpp.org)
[![MIT](https://img.shields.io/apm/l/vim-mode.svg)](https://opensource.org/licenses/MIT")
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=ArkNX_PSS_ASIO&metric=alert_status)](https://sonarcloud.io/dashboard?id=ArkNX_PSS_ASIO)  

<p align="center"><img src="https://raw.githubusercontent.com/freeeyes/PSS/gh-pages/_images/pss.svg?sanitize=true" alt="pss" width="380"/></p>

Table of Contents
=================

 - [Overview](#overview)
 - [Download](#download)
 - [Build and Install](#build-and-install)
 - [Documentation](#documentation)
 - [Support](#support)
 - [Contributing](#contributing)

Overview
========

PSS is a plug-in management system for different IO logic. You can ignore the details of IO establishment and build your own logic application.  
PSS encapsulates a unified interface for Tcp, udp, serial port, http, websocket, and ssl.  
You can use Configure files or unified interfaces to create and use them.  
The logic plug-in is to complete the logic processing after the data arrives,  
all loaded in the form of a dynamic library,  
which separates the coupling of the IO and the logic itself.  
Simple logic development.  

This project consists of three parts  
(1) Main frame  
(2) Data packet analysis plug-in  
(3) Logic processing plug-in  

You can implement the latter two plug-ins to complete your business logic deployment.  

Download
========
You can get the download here  
[pss curr version](https://github.com/ArkNX/PSS_ASIO/releases/).

Build and Install
=================
 * [Notes for WINDOWS-like platforms](NOTES-WINDOWS.md)  
 * [Notes for UNIX-like platforms](NOTES-LINUX.md)  

Documentation
=============
 * [How to make packetparse plugin](md/How_to_make_packetparse_plugin.md)
 * [How to make logic plugin](md/How_to_make_logic_plugin.md)
 * [How to config server](md/How_to_config_server.md)
 * [How to use framework api in logic plugin](md/How_to_use_framework_api_in_logic_plugin.md)
 * [How to use tools make logic plugin](md/How_use_tools_make_logic_plugin.md) 

Support
=======
You can submit your question to the GitHub issue for answers,  
or, if you have QQ, you can join the group 540961187, and you have received technical support.

Contributing
============
You can tell us how to do better according to the functions of the framework and the requirements of your logic processing functions.  
Your thoughts are welcome to settle here to provide convenience to more people.  

