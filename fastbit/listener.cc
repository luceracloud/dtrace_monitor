/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  listener.cc
 *   Collects ZMQ/protobuf transmitted
 *   messages from a known IP (global
 *   zone). Saves all of the data to
 *   directories by IP-zonename.
 *
 *  CREATED:  8 AUG 2013
 *  EDITED:  14 AUG 2013
 */

#include <zmq.hpp>
#include <iostream>
#include <sstream>
#include <signal.h>

#include "util.hpp"
#include "pckt.pb.h"
#include "zmq.hpp"
#include "fastbit.hpp"

/* Forward declaration */
void usage ();
void sig_handler (int s);
ibis::tablex *find_table (std::string zonename,
                          std::map <size_t, std::string> *Indices,
                          std::map <std::string, ibis::tablex *> *Tables);

/* Global variables */
bool run = true;
char VERBOSE = 0b0;
time_t save_rate = 60;
time_t last_save = 0;

/*
 *  Entry point 
 */
int main (int argc, char **argv) {

  time_t delay = 3600; // Number of packets to wait before printing ALIVE

  /* Handle signals */
  signal (SIGINT, sig_handler);
  signal (SIGTERM, sig_handler);
  signal (SIGQUIT, sig_handler);

  UTIL::red();
  std::cout << "\n Collector program starting.\n" << std::endl;
  UTIL::clear();

  std::string IP = "172.20.0.85";
  std::string PORT = "7211";

  /* Parse command line */
  for (size_t i=0; i<argc; i++) {
    if (VERBOSE) printf ( "Argument %s\n", argv[i]);
    if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "-V")) {
      UTIL::cyan();
      std::cout << " . running in verbose mode\n";
      UTIL::clear();
      VERBOSE = 1;
    }
    if (!strcmp (argv[i], "-vfull")) {
      VERBOSE = 100;
    }
    if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "-H")) {
      UTIL::cyan();
      std::cout << " . printing usage information" << std::endl;
      UTIL::clear();
      usage();
      return 1;
    }
    if (!strcmp (argv[i], "-p") || !strcmp (argv[i], "-P")) {
      PORT = argv[i+1];
      std::cout << "\033[00;36m . using port " << PORT << "\033[00m\n"; 
    }
    if (!strcmp (argv[i], "-a") || !strcmp (argv[i], "-A")) {
      IP = argv[i+1];
      std::cout << "\033[00;36m . using IP " << IP << "\033[00m\n";
    }
    if (!strcmp (argv[i], "-d") || !strcmp (argv[i], "-D")) {
      delay = atoi (argv[i+1]);
      std::cout << "\033[00;36m . printing ALIVE after " << delay << " packets\033[00m\n";
    }
  }

  /* Data */
  std::ostringstream new_line;
  std::map <size_t, std::string> ZoneIndices;
  std::map <std::string, ibis::tablex *> ZoneTables;

  /* Set up ZMQ */  
  zmq::context_t context (1);
  zmq::message_t msg(100);

  /* Set up protobuf stuff */
  GOOGLE_PROTOBUF_VERIFY_VERSION; 
  PBMSG::Packet pckt;

  /* Set up sockets to listen to servers */
  zmq::socket_t listen_socket (context, ZMQ_SUB);
  std::ostringstream CON;
  CON << "tcp://" << IP << ":" << PORT;
  
  listen_socket.connect(CON.str().c_str()); 

  listen_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
  size_t packet_counter = 0;

  uint64_t st = time (NULL);
  UTIL::red();
  std::cout << "\n Server online!";
  printf ("\n Start time was: %2d:%2d:%2d UTC\n\n", 
                (st/3600)%24, (st/60)%60, st%60);
  UTIL::clear();

  /* Start loop */
  while (run) {
    packet_counter++;
    if (VERBOSE) std::cout << "Waiting.." << std::endl;

    if ((packet_counter%delay) == 0) {
      UTIL::white();
      std::cout << "ALIVE, lifetime packets: " << packet_counter << std::endl;
      UTIL::clear();
    }

    /* Wait unless (e.g.) we interrupt */
    try {
      listen_socket.recv (&msg);

      pckt.ParseFromArray (msg.data(), msg.size());

      if (VERBOSE) {
        std::cout << "Packet number " << packet_counter << std::endl;
        std::cout << "Size of previous packet: " << msg.size() << std::endl;
      }

      /* Select proper table */
      if (VERBOSE) std::cout << "Zone name: " << pckt.name() << std::endl;
      ibis::tablex *tbl = find_table (pckt.name(), &ZoneIndices, &ZoneTables);

      /* Read into table */
      bool is_gz = false;
      if (pckt.name() == "global") {
        if (VERBOSE) std::cout << "Zone is global, using global insertion functions." << std::endl;
        is_gz = true;
      }

      new_line.str("");
      new_line.clear();
      FASTBIT::read_gen (&pckt, &new_line, is_gz);
      FASTBIT::read_mem (&pckt, &new_line, is_gz);
      FASTBIT::read_cpu (&pckt, &new_line, is_gz);
      FASTBIT::read_disk (&pckt, &new_line, is_gz);
      FASTBIT::read_net (&pckt, &new_line, is_gz);
      FASTBIT::read_proc (&pckt, &new_line, is_gz);

      tbl->appendRow ((const char *)new_line.str().c_str());

      if (VERBOSE) {
        std::cout << "Table ptr is " << tbl << std::endl;
        std::cout << "Line returned for this table is: \n";
        std::cout << new_line.str() << std::endl;
      }
    } catch (zmq::error_t e) {
      UTIL::yellow();
      std::cout << "ZMQ listening error:\n" << e.what() << std::endl;
    }

    /* Write to database */
    if (time (NULL) - last_save > save_rate && time (NULL)%save_rate < 10) {
      last_save = time (NULL);
      for (size_t i=0; i<ZoneIndices.size(); i++) {
        UTIL::cyan();
        std::ostringstream NAME;
        NAME << IP << "-" << ZoneIndices.at(i);
        FASTBIT::write_to_db (find_table (ZoneIndices.at(i), 
                              &ZoneIndices, 
                              &ZoneTables), 
                              0b0, 
                              time (NULL) - save_rate, 
                              NAME.str());
        std::cout << "Saved table for " << NAME.str() << std::endl;
        UTIL::clear();
      }
      /* If we don't clear here, we could end up
       * with massive overhead, attempting to 
       * access every zone ever created. Admittedly
       * this adds a bit of overhead each save, but
       * it has the potential to save us in the long
       * run.
       */
      ZoneIndices.clear();
      ZoneTables.clear();
    }
  }

  /* Take care of killing functions */
  for (size_t i=0; i<ZoneIndices.size(); i++) {
    UTIL::white();
    std::ostringstream NAME;
    NAME << IP << "-" << ZoneIndices.at(i);
    FASTBIT::write_to_db (find_table (ZoneIndices.at(i),
                          &ZoneIndices,
                          &ZoneTables),
                          0b0,
                          time (NULL) - save_rate,
                          NAME.str(),
                          true);
    std::cout << "Saved table for " << NAME.str() << " in TEMP" << std::endl;
  }

  /* Shutdown protobuf library */
  google::protobuf::ShutdownProtobufLibrary();

  return 0;
}

/* Return a pointer to the appropriate table
 * and create one if it doesn't exist
 */
ibis::tablex *find_table (std::string zonename,
                          std::map <size_t, std::string> *Indices,
                          std::map <std::string, ibis::tablex *> *Tables) {

  if (VERBOSE) std::cout << "\nSearching for table for zone named " << zonename << std::endl;

  if (Tables->find(zonename) == Tables->end()) {
    UTIL::cyan();
    std::cout << "Creating new table for " << zonename << std::endl;
    UTIL::clear();
    Indices->insert (std::make_pair (Indices->size(), zonename));
    ibis::tablex *tbl = ibis::tablex::create();
    // We allow for two different configuration files,
    // one for GZ and one for NGZ.
    std::string column_line;
    if (zonename == "global") {
      FASTBIT::load_config ("gz.conf", &column_line, tbl, &save_rate);
    } else {
      FASTBIT::load_config ("ngz.conf", &column_line, tbl, &save_rate);
    }
    if (VERBOSE) std::cout << "Column line:" << std::endl << column_line << std::endl;
    Tables->insert (std::make_pair (zonename, tbl));
  }

  if (VERBOSE > 1) std::cout << zonename << std::endl;

  return Tables->at (zonename); 
}

/* 
 * Print out message on SIG and
 * also ensures that the table
 * is saved.
 */
void sig_handler (int s) {
  UTIL::purple();
  std::cout << "\n\nReceived signal interrupt\n" << std::endl;
  UTIL::clear();
  run = false;
}

/*
 *  Prints usage information to user
 */ 
void usage () {
  UTIL::white();
  std::cout << "\nusage:  listener [-h] [-v] [-p NUMBER] [-a ADDR] [-d DELAY]"; 
  std::cout << "\n    -h         prints this help/usage page";
  std::cout << "\n    -v         run in verbose mode (print all queries and ZMQ packets)";
  std::cout << "\n    -p PORT    use port PORT";
  std::cout << "\n    -a ADDR    listen to IP address ADDR";
  std::cout << "\n    -d DELAY   how many packets to wait before printing ALIVE message";
  std::cout << "\n\n";
  UTIL::clear();
}

