/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  server.cpp
 *    Pushes kstat & dtrace statistics
 *    to network via ZMQ & protocol
 *    buffers.
 *
 *    COMPILE WITH:
 *      g++ server.cpp pckt.pb.cc -ldtrace
 *        -lzmq -lkstat -lprotobuf -O2 
 *        -o generator
 *
 *    CREATED:  16 JUL 2013
 *    UPDATED:  14 AUG 2013
 */

#include <string>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sstream>

#include <zmq.hpp>
#include "pckt.pb.h"

#include "util.hpp"
#include "scripts.hpp"
#include "kstat.hpp"
#include "zone.hpp"
#include "dtrace.hpp"

/*
 *  Forward declarations 
 */
int send_message (PBMSG::Packet pckt, zmq::socket_t *socket);
void sig_handler (int s);
void splash ();
void usage ();

/* 
 *  Global verbose & running variables
 */
bool do_loop = 1;
bool VERBOSE = 0; 

/*
 *  Entry point
 */
int main (int argc, char **argv) {

    /* Handle signals (for cleanup) */
    signal (SIGINT, sig_handler);
    signal (SIGTERM, sig_handler);
    signal (SIGQUIT, sig_handler);

    splash();

    /* Variable declarations */
    kstat_ctl_t *kc;        // Pointer to kstat chain
    bool V_LITE = false;
    bool QUIET = false;
    time_t millisec = 999;  // Values >= 1000 effect weird behavior
    const char *port = "7211";
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_PUB);
    std::map <std::string, Zone *> ZoneData;
    std::map <size_t, std::string> ZoneIndices;

    /* Parse command line */
    for (size_t i=0; i<argc; i++) {
      if (VERBOSE) printf ( "Argument %s\n", argv[i]);
      if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "-V")) {
        std::cout << "\033[00;36m . running in verbose mode\033[00m" << std::endl;
        VERBOSE = true;
      }
      if (!strcmp (argv[i], "-vlite")) {
        std::cout << "\033[00;36m . running in light verbose mode\n\033[00m";
        V_LITE = true;
      }
      if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "-H")) {
        std::cout << "\033[00;36m . printing usage information\n\033[00m";
        usage();
        return 1;
      }
      if (!strcmp (argv[i], "-p") || !strcmp (argv[i], "-P")) {
        port = argv[i+1];
        std::cout << "\033[00;36m . using port " << port << "\033[00m\n"; 
      }
      if (!strcmp(argv[i], "-q") || !strcmp (argv[i], "-Q")) {
        QUIET = 1;
        std::cout << "\033[00;36m . running in quiet mode\033[00m\n";
      }
      if (!strcmp(argv[i], "-d") || !strcmp (argv[i], "-D")) {
        millisec = atoi(argv[i+1]);
        if (millisec > 999) millisec = 999;
        std::cout << "\033[00;36m . using delay of " << argv[i+1] << "\033[00m\n";
      }
    }
 
    /* Set up zmq socket */
    char buf[15];
    sprintf(buf, "tcp://*:%s", port);
    if (!QUIET) {
      socket.bind (buf);
    }

    /* Protobuf stuff */
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    
    /* Dtrace init */
    dtrace_hdl_t *g_dtp[DTRACE::number]; 
    for (size_t i=0; i<DTRACE::number; i++) {
      DTRACE::init (&g_dtp[i], DTRACE::dtrace[i]);
      DTRACE::setopts (&g_dtp[i]);
      if (dtrace_go (g_dtp[i])) {
        UTIL::red();
        std::cout << "ERROR: Unable to start dtrace script" << std::endl;
        UTIL::clear();
      }
      (void) dtrace_status (g_dtp[i]);
    } 

    /* Kstat init */
    kc = kstat_open();
    uint64_t st = time (NULL);  // start time

    /* Allow us to pass values in and out */
    uint64_t value;
    std::vector<uint64_t> values;
    std::vector<std::string> names;
    std::vector<std::string> zones;

    UTIL::red();
    std::cout << "\n Server online!";
    printf ("\n \033[00;31mStart time was: %2d:%2d:%2d UTC\033[00m\n\n", 
                (st/3600)%24, (st/60)%60, st%60);
    UTIL::clear();

    /* Collect and send data in loop */
    while (do_loop) {
     
      /* Clean up from last cycle and updates */
      ZoneData.clear();
      ZoneIndices.clear();
 
      /* Custom format to add zones to ZoneData &
       * to populate the repeated zonename in the GZ
       * zone/packet
       */
      if (KSTAT::retreive_multiple_kstat (kc, std::string("zones"), 
                                          std::string("zonename"), &values, 
                                          &names, &zones)) {
        UTIL::red();
        std::cout << "Something went terribly wrong. We cannot populate the\nlist of zones. "
                      "Skipping current sample, attempting next time." << std::endl;
        UTIL::clear();
      } else {
        /* Init GZ first to hold other zones & set as type:GZ */
        size_t z_index = 0;
#ifdef ZONE
        for (size_t i=0; i<zones.size(); i++) {
#else
        ZoneData.insert (std::make_pair (std::string("global"), 
                              new Zone ("global", true)));
        ZoneIndices.insert (std::make_pair (z_index, "global"));
        for (size_t i=0; i<zones.size(); i++) {
          ZoneData["global"]->add_zone (&zones.at (i));
#endif
          if (names.at(i)!="global") {
            ZoneData.insert (std::make_pair (zones.at(i), 
                              new Zone (std::string( zones.at (i)), false)));
            ZoneIndices.insert (std::make_pair (z_index, zones.at (i)));
          }
          z_index++;
        }
      }

      /* The kstat chain should be updated occasionally */
      if (kstat_chain_update ( kc )) {
        if (VERBOSE) {
          UTIL::cyan();
          std::cout << " . kstat chain updated" << std::endl;
          UTIL::clear();
        }
      }
     
      /*
       * Grab memory statistics, first
       * from GZ, then from elsewhere.
       */
#ifdef ZONE
#else
      for (size_t i=0; i<MEM::GZ_size; i++) {
        if (KSTAT::retreive_kstat (kc, MEM::GZ_modl[i], MEM::GZ_name[i],
                                    MEM::GZ_stat[i], -1, &value)) {
          std::cout << "Unable to grab memory statistic\n";
        } else {
          ZoneData["global"]->add_mem (&MEM::GZ_stat[i], value);
        }
      }
#endif
      for (size_t i=0; i<MEM::size; i++) {
        if (KSTAT::retreive_multiple_kstat (kc, MEM::modl[i], MEM::stat[i], 
                                            &values, &names, &zones)) {
          std::cout << "Unable to retreive expected kstats for " << MEM::modl[i] << " " <<
                        MEM::stat[i] << __LINE__ << std::endl;
        } else {
          for (size_t j=0; j<names.size(); j++) {
            ZoneData[zones.at(j)]->add_mem (&MEM::stat[i], values.at (j));
          }
        }
      }

      /*
       * Grab network statistics, we
       * only care about NGZ stats,
       * as there are no GZ-specific
       * ones
       */
      for (size_t i=0; i<NET::size; i++) {
        if (KSTAT::retreive_multiple_kstat (kc, NET::modl[i], NET::stat[i], 
                                            &values, &names, &zones)) {
          std::cout << "Unable to retreive expected kstat for " << NET::modl[i] << " " <<
                       NET::stat[i] << __LINE__ << std::endl;
        } else {
          for (size_t j=0; j<values.size(); j++) {
            ZoneData[zones.at (j)]->add_network (&names.at (j), &NET::stat[i], values.at (j));
          }
        }
      }

      /*
       * Grab disk statistics. This
       * one works a bit differently
       * because of the way that I/O
       * kstats are stored. Still
       * returns a vector like the others.
       */
#ifdef ZONE
#else
      for (size_t instance=0; instance<13; instance++) {
        for (size_t i=0; i<DISK::GZ_size; i++) {
          std::ostringstream GZ_name;
          GZ_name << "sd" << instance;
          std::string GZ_name_str = GZ_name.str();
          if (DISK::GZ_modl[i]=="sderr") GZ_name << ",err";
          if (KSTAT::retreive_kstat (kc, DISK::GZ_modl[i], GZ_name.str(),
                                  DISK::GZ_stat[i], -1, &value)) {
            std::cout << "Unable to retreive expected GZ kstat for " << 
                          DISK::GZ_modl[i] << " " << " " << DISK::GZ_stat[i] << 
                          "server.cpp:" << __LINE__ << std::endl;
          } else {
            ZoneData["global"]->add_disk (&GZ_name_str, &DISK::GZ_stat[i], value);
          }
        }
      }
#endif
      for (size_t i=0; i<DISK::size; i++) {
        if (KSTAT::retreive_multiple_kstat (kc, DISK::modl[i], DISK::stat[i], 
                                            &values, &names, &zones)) {
          std::cout << "Unable to retreive expected kstat for " << DISK::modl[i] << " " <<
                       DISK::stat[i] << " server.cpp:" << __LINE__ << std::endl;
        } else {
          for (size_t j=0; j<values.size(); j++) {
            ZoneData[zones.at (j)]->add_disk (&DISK::modl[i], &DISK::stat[i], values.at (j));
          }
        }
      }

      /* Do dtrace look-ups */
      for (size_t i=0; i<DTRACE::number; i++) {
        (void) dtrace_status (g_dtp[i]);
        if (dtrace_aggregate_snap (g_dtp[i]) != 0) {
          UTIL::yellow();
          std::cout << "WARN: Failed to snap aggregate" << std::endl;
          UTIL::clear();      
        }
        if (dtrace_aggregate_walk_valsorted (g_dtp[i], DTRACE::aggwalk, &ZoneData) != 0) {
          UTIL::yellow();
          std::cout << "WARN: Failed to walk aggregate" << std::endl;
          UTIL::clear();  
        }
        if (VERBOSE) std::cout << "===========================================" << std::endl;
      }

      /*
       *  Debug print / send packets
       */
      if (VERBOSE) {
        for (size_t i=0; i<ZoneData.size(); i++) {
          UTIL::blue();
          std::cout << std::endl << "BEGIN Zone Packet:" << std::endl;
          UTIL::clear();
          ZoneData.at(ZoneIndices.at(i))->print_zone();
        } 
      }

      for (size_t i=0; i<ZoneData.size(); i++) {
        ZoneData.at (ZoneIndices.at(i))->set_time( time(NULL) );
        Zone *z = ZoneData.at (ZoneIndices.at (i));
        PBMSG::Packet *pckt = z->ReturnPacket();
        if (!QUIET) send_message (*pckt, &socket);
        if (V_LITE) std::cout << "UTC " << (time(NULL)/3600)%24 << ":" <<
                      (time(NULL)/60)%60 << ":" << time(NULL)%60 << std::endl;
        if (VERBOSE) std::cout << "Packet sent" << std::endl;
      }
        
      struct timespec req = {0};
      req.tv_sec = 0;
      req.tv_nsec = millisec * 1000000L;
      nanosleep(&req, (struct timespec *)NULL);
    }

    /* Kill DTrace scripts */
    for (size_t scpt=0; scpt<DTRACE::number; scpt++) {
      dtrace_close (g_dtp[scpt]); 
    }
    UTIL::cyan();
    std::cout << " . dtrace scripts killed" << std::endl;
    UTIL::clear();    

    /* Shutdown ProtoBuf library */
    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}

/*
 * This function sends a message 
 * over the established zmq socket
 */
int send_message (PBMSG::Packet pckt, zmq::socket_t *socket) {
  try {
    std::string pckt_serialized;
    pckt.SerializeToString (&pckt_serialized);
    zmq::message_t msg (pckt_serialized.size());
    memcpy ((void *)msg.data(), pckt_serialized.c_str(), pckt_serialized.size());
    socket->send (msg, ZMQ_NOBLOCK);
  } catch (int e) {
    std::cout << "Error number " << e << std::endl;
    return -1;
  }
  return 0;
}

/*
 * Handle signals so that we don't have to
 * worry about errant dtrace scripts
 */
void sig_handler (int s) {
  do_loop = 0;
  UTIL::cyan();
  std::cout << "\n\nSafely killing program...\nStopping dtrace script interface\n";
  UTIL::clear();
}

/*
 * Prints splash screen to user
 */
void splash () {
  UTIL::white();
  std::cout << "\n Statistics Server\n";
  std::cout << "\n  Reports dtrace and kstat statistics\n";
  std::cout << "  using Google Protocol Buffers and\n";
  std::cout << "  ZMQ (0MQ) sockets.\n\n";
  UTIL::clear();
}

/*
 * Prints usage information to user
 */
void usage () {
  UTIL::white();
  std::cout << "\nusage:  server [-h] [-p NUMBER] [-v] [-vlite] [-d DELAY] [-q]"; 
  std::cout << "\n    -h         prints this help/usage page";
  std::cout << "\n    -p PORT    use port PORT";
  std::cout << "\n    -v         run in verbose mode (print all queries and ZMQ packets)";
  std::cout << "\n    -vlite     prints time (and dtrace ticks (tick-4999)) for each sent message";
  std::cout << "\n    -d DELAY   wait DELAY<1000 ms instead of default 1000";
  std::cout << "\n    -q         quiet mode, prints diagnostic information and does not send messages";
  std::cout << "\n\n";
  UTIL::clear();
}
