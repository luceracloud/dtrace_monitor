/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  fastbit.hpp
 *   Includes a nice library of functions 
 *   that allow easy interface with ibis
 *   from C++ program.
 *
 *   CREATED:  26 JUL 2013
 *   EDITED:    9 AUG 2013
 */

#define COMBINE(a,b) (((uint64_t)a<<32)|(uint64_t)b)

#include <ibis.h>

namespace FASTBIT {

uint_fast32_t NUMBER_OF_PROC = 0;
char NUMBER_OF_CPUS = 0;
uint_fast8_t FLAGS = 0;
std::string DB_DIRECTORY;

/*
 * Read config file and populate
 * table accordingly
 */
bool load_config (const char *fname, std::string *column_line, 
                    ibis::tablex *tbl, time_t *save_rate) {
   
  std::vector<std::string> columns_vector;
  std::vector<ibis::TYPE_T> columns_type;
  std::string columns;

  FILE *fconf;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  char mode = 0;
  uint_fast8_t repeat = 0;

  /* Load file */
  fconf = fopen(fname, "r");
  if (fconf == NULL) {
    std::cout << "\033[31mUnable to read configuration file\033[m\n"; 
    return 1;
  }

  /* Read and parse config file */
  while ((read = getline(&line, &len, fconf)) != -1) {
    if (line[0]==35 || line[0]==10) {
    } else if (line[0]==36) {
      if (line[1]=='C' && line[2]=='P' && line[3]=='U') FLAGS |= 0b1;
      if (line[1]=='D' && line[2]=='B' && line[4]=='D') FLAGS |= 0b10;
      if (line[1]=='S' && line[2]=='A' && line[6]=='R') FLAGS |= 0b100;
      if (line[1]=='P' && line[2]=='R' && line[4]=='C') FLAGS |= 0b1000;
      mode = 2;
    } else if (mode==2) {
      mode = 1;
      repeat = atoi(line);
      if (FLAGS & 0b1) {
        FLAGS &= ~0b1;
        NUMBER_OF_CPUS = atoi(line);
      } else if (FLAGS & 0b10) {
        FLAGS &= ~0b10;
        DB_DIRECTORY = line;
        DB_DIRECTORY = DB_DIRECTORY.substr(0, strlen(line)-1);
      } else if (FLAGS & 0b100) {
        FLAGS &= ~0b100;
        *save_rate = (time_t)atoi(line);
      } else if (FLAGS & 0b1000) {
        FLAGS &= ~0b1000;
        NUMBER_OF_PROC = atoi(line); 
      }
    } else if (mode==1) {
      mode = 2 + atoi(line);
      if (columns_vector.size()>0) columns_vector.clear();
      if (columns_type.size()>0) columns_type.clear();
    } else if (mode>2) {
      mode--;
      columns_vector.push_back (std::string(line).substr(0, std::string(line).find(':')));
      std::string type = std::string(line).substr(std::string(line).find(':')+1, strlen(line)-1); 
      if (type.substr(0, 4)=="TEXT") {
        columns_type.push_back (ibis::TEXT);
      } else if (type.substr(0, 4)=="UINT") {
        columns_type.push_back (ibis::UINT); 
      } else if (type.substr(0, 5)=="ULONG") {
        columns_type.push_back (ibis::ULONG);
      } else {
        std::cout << "Hit unknown type: " << type << std::endl;
      }

      columns += ",";
      
      if (mode==2) {
        std::ostringstream oss;
        for (int i=0; i<repeat; i++) {
          for (int j=0; j<columns_vector.size(); j++) {
            std::ostringstream column; 
            oss << columns_vector.at (j);
            column << columns_vector.at (j);
            if (repeat>1) {
              if (repeat>10 && i<10) {
                oss << 0;
                column << 0;
              }
              if (repeat>100 && i<100) {
                oss << 0;
                column << 0;
              }
              oss << i;
              column << i;
            }
            tbl->addColumn ((const char *)column.str().c_str(), columns_type.at (j));
            oss << ","; 
          }
        }
        *column_line += oss.str();
      }
    }
  }

  /* Free memory before exiting & close file */
  if (line) free(line);
  fclose(fconf);
  
  column_line->erase (column_line->size() - 1);

  return 0;
}

/*
 * Read GENERAL data and append it to ostring
 */
bool read_gen (PBMSG::Packet *pckt, std::ostringstream *line, bool gz=false) {
  *line << pckt->name() << "," << pckt->time() << "," << pckt->ticks() << ",";
  *line << pckt->processes() << "," << pckt->threads() << ",";
  return 0;
}

/*
 * Read MEM data and append it to ostring
 */
bool read_mem (PBMSG::Packet *pckt, std::ostringstream *line, bool gz=false) {
  if (gz) {
    for (size_t i=0; i<pckt->mem_size(); i++) {
      const PBMSG::Packet_Mem &mem_pckt = pckt->mem(i);
      *line << mem_pckt.rss() << ",";
      *line << mem_pckt.physcap() << ",";
      *line << mem_pckt.swap() << ",";
      *line << mem_pckt.swapcap() << ",";
      *line << mem_pckt.physmem() << ",";
      *line << mem_pckt.pp_kernel() << ",";
      *line << mem_pckt.freemem() << ",";
      *line << mem_pckt.nalloc_calls() << ",";
      *line << mem_pckt.nfree_calls() << ",";
    }
  } else {
    for (int i=0; i<pckt->mem_size(); i++) {
      const PBMSG::Packet_Mem &mem_pckt = pckt->mem(i);
      *line << mem_pckt.rss() << ",";
      *line << mem_pckt.physcap() << ",";
      *line << mem_pckt.swap() << ",";
      *line << mem_pckt.swapcap() << ",";
    }
  }

  return 0;
}

/*
 * Read CPU data and append it to ostring 
 */
bool read_cpu (PBMSG::Packet *pckt, std::ostringstream *line, bool gz=false) {

  uint32_t usage[NUMBER_OF_CPUS];
  memset( usage, 0, NUMBER_OF_CPUS*sizeof(uint32_t) );

  for (int i=0; i<pckt->cpu_size(); i++) {
    const PBMSG::Packet_Cpu &cpu_pckt = pckt->cpu(i);
    usage[cpu_pckt.core()] = cpu_pckt.usage();
  }

  for (int i=0; i<NUMBER_OF_CPUS; i++) {
    *line << usage[i] << ",";
  }

  return 0;
}

/*
 * Read disk info and append to ostring
 */
bool read_disk (PBMSG::Packet *pckt, std::ostringstream *line, bool gz=false) {

  if (gz) {
    for (size_t i=0; i<pckt->disk_size(); i++) {
      const PBMSG::Packet_Disk &disk_pckt = pckt->disk(i);
      *line << disk_pckt.instance() << ",";
      *line << disk_pckt.nread() << ",";
      *line << disk_pckt.nwritten() << ",";
      *line << disk_pckt.reads() << ",";
      *line << disk_pckt.writes() << ",";
      *line << disk_pckt.wtime() << ",";
      *line << disk_pckt.wlentime() << ",";
      *line << disk_pckt.rtime() << ",";
      *line << disk_pckt.rlentime() << ",";
      *line << disk_pckt.harderror() << ",";
      *line << disk_pckt.softerror() << ",";
      *line << disk_pckt.tranerror() << ",";
    }
  } else {
    for (size_t i=0; i<pckt->disk_size(); i++) {
      const PBMSG::Packet_Disk &disk_pckt = pckt->disk(i);
      *line << disk_pckt.instance() << ",";
      *line << disk_pckt.nread() << ",";
      *line << disk_pckt.nwritten() << ",";
      *line << disk_pckt.reads() << ",";
      *line << disk_pckt.writes() << ",";
      *line << disk_pckt.wtime() << ",";
      *line << disk_pckt.wlentime() << ",";
      *line << disk_pckt.rtime() << ",";
      *line << disk_pckt.rlentime() << ",";
    }
  }

 return 0;
}

/*
 * Read network info and append
 */
bool read_net (PBMSG::Packet *pckt, std::ostringstream *line, bool gz=false) {

  for (int i=0; i<pckt->net_size(); i++) {
    const PBMSG::Packet_Net &net_pckt = pckt->net(i);
    *line << net_pckt.instance() << ",";
    *line << net_pckt.obytes64() << ",";
    *line << net_pckt.rbytes64() << ",";
    *line << net_pckt.opackets() << ",";
    *line << net_pckt.ipackets() << ",";
    *line << net_pckt.oerrors() << ",";
    *line << net_pckt.ierrors() << ",";
  }

  return 0;
}

/*
 * Read process information and append to ostring
 */
bool read_proc (PBMSG::Packet *pckt, std::ostringstream *line, bool gz=false) {

  int j=0;
  for (int i=pckt->process_size()-1; i>=0; i--) {
    const PBMSG::Packet_Process &proc_pckt = pckt->process(i);
    *line << proc_pckt.pid() << ",";
    *line << proc_pckt.execname() << ",";
    *line << proc_pckt.usage() << ",";
    *line << proc_pckt.cpu() << ",";
    j++;
    if (j >= NUMBER_OF_PROC) {
      break;
    }
  }
  
  if (pckt->process_size() < NUMBER_OF_PROC) { 
    for (int j=0; j<NUMBER_OF_PROC-pckt->process_size(); j++) {
      *line << std::string("0,,0,0,");
    } // maintains db format
  }

  return 0;
}

/*
 * Allow saving of data into proper directory
 */
bool write_to_db (ibis::tablex *tbl, uint_fast8_t verbosity, time_t t, std::string zone, bool temp=false) {

  struct tm * now = gmtime (&t);
  std::ostringstream directory;  

  directory << DB_DIRECTORY;

  if (temp) directory << "_temp";
  directory << "/" << zone;
  directory << "/" << now->tm_mday << "-" << now->tm_mon+1 << "-" <<
        now->tm_year+1900;
  directory << "/" << now->tm_hour;

  {
  std::ostringstream part_name;
  part_name << directory.str() << "/-part.txt";
  FILE *exists;
  exists = fopen (part_name.str().c_str(), "r");
  if (exists != NULL) {
    std::cout << "Folder already exists, using _collide name" << std::endl;
    directory << "_collide";
  }
  fclose (exists);
  }

  if (temp) directory << "h" << now->tm_min << "m" << now->tm_sec;

  tbl->write ((const char *)directory.str().c_str(), (const char *)"zone data save",
        (const char *)"Collection of dtrace/kstat collected data since either start"
        "or the last hour (UTC)");

  if (verbosity) tbl->describe (std::cout);
  tbl->clearData();
}

}
