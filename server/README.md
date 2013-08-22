dtrace / kstat generator & server
=================================
Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com

### issues
No known issues at this time.

### overview
This program acts as a simple server that collects machine statistics from Solaris's "kstat" (Kernal STATistics) and custom DTrace scripts, wraps them in a custom protobuf format, and then publishes them on any user-specified port using the ZMQ PUBlish method.

It is designed to run in either the global or non-global zone of SmartOS systems and collects/sorts gathered statistics by zone. It is meant to be run continuously.

#### usage
```
  generator [-h] [-p PORT] [-v] [-vlite] [-d DELAY] -q
     -h         prints this help/usage page
     -p PORT    use port PORT
     -v         run in verbose mode (print all queries and ZMQ packets)
     -vlite     prints time (and dtrace ticks (tick-4999)) for each sent message
     -d DELAY   wait DELAY<1000 ms instead of default 1000
     -q         quiet mode, prints diagnostic information and does not send messages
```

### quickstart
A shell script has been included to automate the install/build process. To run it simply copy the following into a terminal. The issuing shell must have root priviledges.
```bash
curl https://raw.github.com/luceracloud/dtrace_monitor/master/server/install.sh > install.sh
sh install.sh
```

### download & build
All of this needs to be done in a non-global zone. After everything is settled, you'll have to move the binary and libraries to the global zone manually.

#### git
```bash
pkgin install scmgit-base
```

#### [gcc](http://gcc.gnu.org/)
```bash
pkgin install gcc47-4.7.2nb3 gmake   # option 1
pkgin install gcc47-4.7.2 gmake      # option 2 if option 1 does not work
```

#### [ØMQ](http://zeromq.org/)
```bash
curl -klO http://download.zeromq.org/zeromq-2.2.0.tar.gz
tar zxf zeromq-2.2.0.tar.gz
cd zeromq-2.2.0
./configure --prefix /opt/local
make
make install
```

#### Google's [Protocol Buffers](https://developers.google.com/protocol-buffers/docs/overview)
```bash
curl -klO https://protobuf.googlecode.com/files/protobuf-2.5.0.tar.gz
tar zxvf protobuf-2.5.0.tar.gz
cd protobuf-2.5.0
./configure --prefix /opt/local
make
make install
```

#### Build
```bash
make rel
```

### run

###### Configuration & Run (Non-global zone)
Because everything was compiled in a non-global zone, running everything is relatively simple. The necessary libraries (protobuf & zmq) have been packaged in the `./metric/lib/` folder, included in this repository. Running `make rel` in the previous step should have created a zone-based binary that is copied into the `./metric/bin/` folder. A shell script that loads everything and runs the program is distributed with this code in the `./metric/bin/` folder.
```bash
# Change directories
cd ./metric/bin
sh ngz_start.sh
```

###### Configuration & Run (Global zone)
You want to run this in the global zone, but if you've followed instructions thus far, everything should be residing happily somewhere in a non-global zone. The necessary libraries (protobuf, zmq, stdlib, etc.) have been packaged in the `./metric/lib/` folder, included in this repository. Running `make rel` should have compiled the server and copied a version of the binary into the `./metric/bin/` folder, so all that needs to be done is move the entire `./metric/` folder into the GZ.

The start shell script that has been included assumes the final location of the meter within the GZ will be `/opt/meter/`, so the following instructions cater to that end. Modify the following as necessary.

```bash
# If necessary, create the metric folder in the GZ:
mkdir /opt/meter

# Copy the ./metric/ into the GZ like so:
cp -rf /zones/<zonename>/root/<path-to-dtrace-server>/metric/* /opt/meter/
# If you're unsure what the zonename is, you can run
# "zoneadm list" and if that doesn't help you, you can
# always run the more risky
# "find / | grep metric/bin/server_release"
```

Running the program is then as simple as navigating to the directory and running the `start.sh` script that has been included.
```bash
cd /opt/meter/bin
sh start.sh
```

###### Keep alive
Of course, you might want to keep the program running after you leave your terminal session. To do so, you can run the server instance in a `screen` session
```bash
screen
sh start.sh
```
and then press Ctrl-A, Ctrl-D to detach the instance.


### more information
###### background
This program relies on the libdtrace and libkstat API's that are a native part of Solaris/SmartOS. In theory, any kstat queries and or new DTrace scripts can be added to the program; however, such updates would require rewriting of both the proto file, zone class, and kstat/DTrace parsing scripts.

In the above instructions, we assume you are starting from a bare computer, thus we provide instructions to start from scratch. The two libraries are set to install to your `/opt/local` directory, which will by default install `include` and `lib` files into `/opt/local/include` and `opt/local/lib` respectively. This setting can be changed by altering the `--prefix` in the `./configure` lines for each library. It is, however, recommended that you install to these locations as the path has been configured to automatically look there in both the Makefile and the start.sh script.

###### External Libraries
Google's [Protocol Buffers](https://developers.google.com/protocol-buffers/docs/overview) or "protobufs" provides an alternative to JSON for the organization, creation, and serialization of packets of information to provide cross-platform/cross-language support with very little overhead.

[ØMQ](http://zeromq.org/) or "Zero MQ" is the socket library we use to publish our packets of information (in the form of the above protobufs) to the network. 

###### Notes about headers
The included kstat header allows queries to the kstat chain; however, does not currently support return types other than `KSTAT_IO_TYPE` and `KSTAT_NAMED_TYPE`.

The included DTrace header provides easy-to-implement functions for outsourcing DTrace script setup, compilation, and aggregate walking.

The `zone.hpp` header acts as a wrapper around the protobuf object which allows protocol buffers to be used in a key-value map while providing the ability to edit/write/access data without keeping track of indices.

`scripts.hpp` is the collection of DTrace and kstat scripts ran or used for lookup by the master program. While editing this file will cause more lookups to be performed, it will not change the zone-packet contents, as this is implemented separately.

`util.hpp` is a custom library used for colorizing output.

###### Design
Because this is a system-monitoring service, it was designed to have as low a profile as possible while at the same time not being entirely obfuscated. As such, many of the functions are&mdash;while abstracted&mdash;not particularily modular. This especially applies to the DTrace scripts for which each requires its own parsing method. Editing is certainly possible, but likely requires editing of at least three files: `zone.hpp`, `scripts.hpp`, and either `kstat.hpp` or `dtrace.hpp`, depending on the type of script.

###### command line arguments and defaults
**flags** | **arg** | description
--- | --- | ---
-h |     | Prints the help/usage page.
-p | PORT | Allows you to change the port on which the server broadcasts. The default port is 7211. 
-v |     | Toggles full verbose mode. See below for profile of verbose message.
-vlite |     | Prints out UTC time each time a packet is sent.
-d | DELAY | Changes delay to DELAY ms. Default delay is 999~1000 ms.
-q |     | Toggles "quiet" mode, where the server will not attempt to bind to a socket and will not attempt to send messages.

In full verbose mode:
* calls to libkstat will print out
  * "Retreived kstat from " MODULE ":" VALUE, in the event of a string
  * or "KSTAT" MODULE NAME STATISTIC INSTANCE
* calls to libdtrace will (generally) print aggregate bin values
* protobuf "packets" (one per zone) will be printed in debug-string mode, in the same order as they are defined in pckt.proto, each preceeded by "BEGIN Zone Packet:"
* the phrase "Packet sent" will be printed each time a packet is successfully sent (or would be sent, in quiet mode)

###### dependencies

* libdtrace (native on SmartOS)
* libkstat (native on SmartOS)
* libprotobuf - Google's Protocol Buffers
  * latest release from: [https://code.google.com/p/protobuf/downloads/list](https://code.google.com/p/protobuf/downloads/list)
  * v2.5.0 was used to develop the code in this repository
* libzmq - ZeroMQ
  * latest release from [http://www.zeromq.org/intro:get-the-software](http://www.zeromq.org/intro:get-the-software)
  * v2.2.0 was used to develop the code in this repository

###### expected source files

* scripts.hpp
* dtrace.hpp
* kstat.hpp
* util.hpp
* zone.hpp
* pckt.proto
* server.cpp


### release notes
22 AUGUST 2013 &mdash; v1.0.0
