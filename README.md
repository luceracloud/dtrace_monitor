dtrace / kstat statistic services
=================================
Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com

general
-------

The included code source makes up the various components of a monitoring service that can be run on SmartOS machines, such as those running on the Joyent Public Cloud (www.joyent.com) or on Lucera (www.luceraHQ.com). 

In general, the service is made of two parts, the __generator__ and the __collector__.

The __generator__ runs as a background process and queries kernel statistics while running DTrace scripts or Kstats outputs. The gathered information is wrapped into protocol buffers by zone and broadcast using the ZMQ publish method. The __collector__ also runs as a background process and collectors the zone data as published by the __generator__. It saves the collected information into databases for later access and querying.

![alt text](https://github.com/luceracloud/dtrace_monitor/blob/master/imgs/overview.png "overview of process")

The source for the __generator__ is included in the `./server` folder of this directory. Two versions that can be used as __collector__ are included in the `./fastbit` and `./redis` directories. 

A shell script that will automatically install the __generator__ and the FastBit __collector__ is included in this directory. It can be downloaded and run by using 
```bash
curl https://raw.github.com/luceracloud/dtrace/master/install.sh > install.sh
sh install.sh
```
Alternatively, to install only the generator, run
```bash
curl https://raw.github.com/luceracloud/dtrace_monitor/master/server/install.sh > install.sh
sh install.sh
```
Similarly, to install only the listener, run
```bash
curl https://raw.github.com/luceracloud/dtrace_monitor/master/fastbit/install.sh > install.sh
sh install.sh
``` 

Note that the build process for the dependencies (especially FastBit) may take a long time.

Directions for how to run the __generator__ and __collector__ can be found in their respective directories.


Related dependencies and build information for code below can be found in README files in respective directories.
___

server (generator)
------------------
Source code for statistics server. This program collects statistics from both kstat and custom DTrace scripts, encodes them using Google's Protocol Buffers and ZMQ, then broadcasts over a user-specified port.

fastbit (listener)
------------------
Source code for [FastBit](https://sdm.lbl.gov/fastbit/)-based collector. Listens to statistics server (see [here](#server)) and aggregates data by IP-zone.


scripts/cpu
-----------
Repository of DTrace scripts that run in the non-global zone and allow you to trace potential CPU/process performance issues.

scripts/misc
----
Repository of DTrace scripts and kstat shell scripts that can be run in the non-global zone that perform various miscellaneous functions. 

scripts/net
---
Repository of kstat shell scripts that allow for monitoring of network resources for identification of potential problems.

