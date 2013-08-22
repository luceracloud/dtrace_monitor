fastbit collector/listener service
==================================
Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com

### issues
No known issues at this time.

### overview
This program sits on a network that has a broadcasting statistics server, collects all broadcasted zone packets and saves them into zone-specific tables. It can listen for ZMQ SUBscribe-able traffic on any port and IP. Its database is configurable by the `gz.conf` and `ngz.conf` files (loaded at runtime) located in the same directory as the executable.

It is designed to be run in any non-global zone on SmartOS systems; however, with a bit of reconfiguration it can be made to run in GZ. It is meant to be run continuously.

#### usage
```bash
  listener [-h] [-v] [-p NUMBER] [-a ADDR] [-d DELAY]
     -h         prints this help/usage page
     -v         run in verbose mode (print all queries and ZMQ packets)
     -p PORT    use port PORT
     -a ADDR    listen to IP address ADDR
     -d DELAY   how many packets to wait before printing ALIVE message
```

### quickstart
A shell script has been included to automate the install/build process. To run it simply copy the following into a terminal. The issuing shell must have root priviledges.
```bash
curl https://raw.github.com/luceracloud/dtrace_monitor/master/fastbit/install.sh > install.sh
sh install.sh
```

___


### download & build

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

#### [FastBit](https://sdm.lbl.gov/fastbit/)
```bash
curl -klO https://codeforge.lbl.gov/frs/download.php/401/fastbit-ibis1.3.5.tar.gz
tar zxvf fastbit-ibis1.3.5.tar.gz
cd fastbit-ibis1.3.5
./configure --prefix /opt/local
make          # make generally takes a long time for FastBit
make install  #
make check    # optional check to make sure you've installed everything correctly
```

#### Build
```bash
make rel
```

### run

###### Configuration
Running `make rel` should generate a binary called `listener` and move it into the `./bin/`. Before running, you might want to edit the two configuration files that define the way the FastBit databases are set up and saved.

Two configuration files come with this distribution, `gz.conf` and `ngz.conf`, detailing the configurations for the global and non-global zones, respectively.

The general format of the configuration file is as follows:
```
$DB_DIRECTORY
./db
$SAVE_RATE
3600

$GEN
repeats
number_of_values
value1:type1
value2:type2
value3:type3
value4:type4
value5:type5

$GROUP
...

```

`$DB_DIRECTORY` specifies the path to the save folder. Its default is `./db`, which means that by default the database will be saved into a directory called `db` within the `bin` directory.

`$SAVE_RATE` specifies the number of seconds between database saves. Its default is 3600, which means that the database is saved (and wiped) once per hour by default.

The following groups `$GROUP` represent segments of code within in the `fastbit.hpp` header. The first number following the group name is the number of times the group is repeated. For example, if you are monitoring 3 network devices in the global zone, you would set this number to `3` in the `gz.conf` file. The second number is simiply a count of the number of values to be saved for that group. In the case above, this number would be 5.

Following is a list of data columns and their respective types, separated by colons. Allowed types are
* TEXT
* ULONG
* UINT

Any additions to existing groups (assuming numbers are properly updated) will be automatically reflected in the database. New groups require new code to be written.

###### Running
A shell script has been included in this repository that automatically loads necessary libraries and runs the listener program. To run it
```bash
cd ./bin/lib
sh libraries
```

In addition to the libraries, you'll need to know where to listen for the incoming data. This data can be found using `ifconfig -a` and looking for the first IP that isn't localhost.

Then to run the program
```bash
# Navigate to the ./fastbit/bin directory
cd ./../   # if you just loaded the libraries
sh start.sh 72.2.113.62  # replace the IP address listed here with the IP you're interested in
```

If you want to keep the listener running, you can use screen
```bash
screen
sh start.sh 72.2.113.62
```
Then to detach the screen (and keep it running), hit Ctrl-A followed by Ctrl-D.


### more information

* The listener program saves databases into the directory specified by the db.conf files with the path `./<db>/IP-zonename/DD-MM-YYYY/HH` by hour. This can be changed in the `fastbit.hpp` header.
* Killing the program with an interrupt (Ctrl-C) causes the data to be saved into a `temp` directory by the second. Thus, data would be saved into `./<db>_temp/IP-zonename/DD-MM-YYYY/HHhMMmSS`.
* In the event that data is saved twice in the same hour (automatically, not by an interrupt), the program will notice and save the second group of data into a `collide` directory, e.g. `./<db>_collide/IP-zonename/DD-MM-YYYY/HH`. This functionality will only work once. All subsequent data saved within the same hour will overwrite the collide folder.
* Two FastBit/csv utilites have been included in the repository and can be built with `make csv_util`
  * make_csv &mdash; Generates .csv files from FastBit database. Run this program from a directory that has FastBit directories within it. Either input the name of the directory as first argument and name of the csv (without the .csv extension) as the second argument, or be prompted for this information at runtime.
  * combine_csv &mdash; Combine any number of .csv files into one. This program loads files given by command line arguments (in order, with .csv appended) and combines them into one large .csv.


### release notes
22 AUGUST 2013 &mdash; v1.0.0

