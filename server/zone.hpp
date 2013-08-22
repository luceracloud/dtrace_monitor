/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  zone.hpp
 *   Defines the datatype for
 *   the "zone" struct, which
 *   can be toggled either GZ
 *   or NGZ.
 *
 *   These are (unfortuntately)
 *   necessary because of the
 *   way that data is returned
 *   from kstat & dtrace.
 * 
 *  CREATED:   6 AUG 2013
 *  EDITED:    8 AUG 2013
 */

#include "pckt.pb.h"

class Zone {
  public:
    Zone (std::string n, bool g) {
      packet = (PBMSG::Packet *)(new PBMSG::Packet);
      packet->set_name (n);
      packet->add_mem();
      this->set_threads (0);
      this->set_processes (0);
      if (g) {
        this->set_global (true);
      } else {
        this->set_global (false);
        packet->add_net();
      }
    }
    ~Zone () {
      free (&(this->packet));
    }
    
    /* General Statistics */ 
    void set_time (uint64_t t) {
      this->packet->set_time (t); 
    }
    void set_ticks (uint64_t t) {
      this->packet->set_ticks (t);
    }
    void set_threads (uint32_t t) {
      this->packet->set_threads (t);
    }
    void set_processes (uint32_t p) {
      this->packet->set_processes (p);
    }
    void add_process () {
      this->packet->set_processes (this->packet->processes() + 1);
    }
    void add_thread () {
      this->packet->set_threads (this->packet->threads() + 1);
    }
    void set_global (bool is_global) {
      global_zone = is_global;
    }

    /* Memory additions */
    void add_mem (std::string *n, uint64_t x) {
      if (*n == "rss") {
        this->packet->mutable_mem(0)->set_rss (x);
      } else if (*n == "physcap") {
        this->packet->mutable_mem(0)->set_physcap (x);
      } else if (*n == "swap") {
        this->packet->mutable_mem(0)->set_swap (x); 
      } else if (*n == "swapcap") {
        this->packet->mutable_mem(0)->set_swapcap (x);
      } else if (*n == "physmem") {
        this->packet->mutable_mem(0)->set_physmem (x);
      } else if (*n == "pp_kernel") {
        this->packet->mutable_mem(0)->set_pp_kernel (x);
      } else if (*n == "freemem") {
        this->packet->mutable_mem(0)->set_freemem (x);
      } else if (*n == "nalloc_calls") {
        this->packet->mutable_mem(0)->set_nalloc_calls (x);
      } else if (*n == "nfree_calls") {
        this->packet->mutable_mem(0)->set_nfree_calls (x);
      } else if (*n == "maj_fault") {
        this->packet->mutable_mem(0)->set_maj_fault (x);
      } else if (*n == "as_fault") {
        this->packet->mutable_mem(0)->set_as_fault (x);
      } else if (*n == "pgin") {
        this->packet->mutable_mem(0)->set_pgin (x);
      } else {
        UTIL::yellow();
        std::cout << "WARN: encountered unknown type in memory @Zone.hpp:" <<
                      __LINE__ << std::endl;
        UTIL::clear();
      }
    }
 
    /* Simple additions */
    void add_cpu (uint32_t c, uint32_t u) {
      PBMSG::Packet_Cpu *cpu = this->packet->add_cpu();
      cpu->set_core (c);
      cpu->set_usage (u);
    }
    void add_queue (uint32_t c, uint32_t l, uint32_t a) {
      PBMSG::Packet_Cpuqueue *cpuqueue = this->packet->add_cpuqueue();
      cpuqueue->set_core (c);
      cpuqueue->set_length (l);
      cpuqueue->set_amount (a);
    }
    void add_zone (std::string *s) {
      this->packet->add_zonename (*s);
    }
    void add_process (uint32_t p, std::string *e, uint32_t u, uint32_t c) {
      PBMSG::Packet_Process *proc = this->packet->add_process();
      proc->set_pid (p);
      proc->set_execname (*e);
      proc->set_usage (u);
      proc->set_cpu (c);
    }
    void add_callfreq (std::string *n, uint64_t t, uint32_t v) {
      PBMSG::Packet_CallFreq *callfreq = this->packet->add_callfreq();
      callfreq->set_name (*n);
      callfreq->set_time (t);
      callfreq->set_value (v);
    }

    /* Mappable additions */
    void add_network (std::string *d, std::string *s, uint64_t v) {
      PBMSG::Packet_Net *net;
 
      if (this->global_zone) {
        if (this->net_map.find(*d) == this->net_map.end()) {
          net = this->packet->add_net();
          net->set_instance (*d);
          this->net_map.insert (std::make_pair (*d, net_map.size()));
        } else {
          net = this->packet->mutable_net (this->net_map[*d]);
        }
      } else {
        net = this->packet->mutable_net (0);
      }
      
      if (*s == "instance") {
        net->set_instance (*d);
      } else if (*s == "obytes64") {
        net->set_obytes64 (v);
      } else if (*s == "rbytes64") {
        net->set_rbytes64 (v);
      } else if (*s == "opackets") {
        net->set_opackets (v);
      } else if (*s == "ipackets") {
        net->set_ipackets (v);
      } else if (*s == "oerrors") {
        net->set_oerrors (v);
      } else if (*s == "ierrors") {
        net->set_ierrors (v);
      } else {
        UTIL::yellow();
        std::cout << "WARN: encountered unknown type in network @zone.hpp:" << 
                      __LINE__ << std::endl;
        UTIL::clear();
      }
    }
    void add_disk (std::string *d, std::string *s, uint64_t v) {
      PBMSG::Packet_Disk *disk;
      
      if (this->disk_map.find(*d) == this->disk_map.end()) {
        disk = this->packet->add_disk();
        disk->set_instance (*d);
        this->disk_map.insert (std::make_pair (*d, disk_map.size()));
      } else {
        disk = this->packet->mutable_disk (this->disk_map[*d]);
      }

      if (*s == "instance") {
        disk->set_instance (*d);
      } else if (*s == "nread") {
        disk->set_nread (v);
      } else if (*s == "nwritten") {
        disk->set_nwritten (v);
      } else if (*s == "reads") {
        disk->set_reads (v);
      } else if (*s == "writes") {
        disk->set_writes (v);
      } else if (*s == "rtime") {
        disk->set_rtime (v);
      } else if (*s == "wtime") {
        disk->set_wtime (v);
      } else if (*s == "rlentime") {
        disk->set_rlentime (v);
      } else if (*s == "wlentime") {
        disk->set_wlentime (v);
      } else if (*s == "Hard Errors") {
        disk->set_harderror (v);
      } else if (*s == "Soft Errors") {
        disk->set_softerror (v);
      } else if (*s == "Transport Errors") {
        disk->set_tranerror (v);
      } else {
        UTIL::yellow();
        std::cout << "WARN: encountered unknown type in disk @Zone.hpp:" << 
                      __LINE__ << std::endl;
      }
    }

    /* Return the packet */ 
    PBMSG::Packet *ReturnPacket () {
      return this->packet;
    }

    /* Print zone statistics */
    void print_zone () {
      this->packet->PrintDebugString ();
    }
    
  private:
    bool global_zone;
    PBMSG::Packet *packet;
    std::map <std::string, uint32_t> net_map;
    std::map <std::string, uint32_t> disk_map;
};
