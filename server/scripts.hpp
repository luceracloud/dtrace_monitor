/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  scripts.hpp
 *    Includes kstat & dtrace scripts,
 *    separated by resource namespace.
 *
 *  CREATED:  17 JUL 2013
 *  UPDATED:   8 AUG 2013
 */


namespace MEM {
  /* Global-zone specific scripts */
  const size_t GZ_size = 5;
  std::string GZ_modl[ GZ_size ] = { "unix", "unix", "unix", "unix", "unix" };
  std::string GZ_name[ GZ_size ] = { "system_pages", "system_pages", "system_pages", 
                                      "system_pages", "system_pages" }; 
  std::string GZ_stat[ GZ_size ] = { "physmem", "pp_kernel", "freemem", "nalloc_calls", 
                                      "nfree_calls" };

  /* Other */
  const size_t size = 4;
  std::string modl[ size ] = { "memory_cap", "memory_cap", "memory_cap", "memory_cap" };
  std::string stat[ size ] = { "rss", "physcap", "swap", "swapcap" };
} 

namespace NET {
  /* NGZ scripts */
  const size_t size = 6;
  std::string modl[ size ] = { "link", "link", "link", "link", "link", "link" };
  std::string stat[ size ] = { "obytes64", "rbytes64", "opackets", "ipackets", 
                                  "oerrors", "ierrors" };
}

namespace DISK {
  /* Global-zone specific scripts */
  const size_t GZ_size = 11;
  std::string GZ_modl[ GZ_size ] = { "sd", "sd", "sd", "sd", "sd", "sd", "sd", "sd", 
                                        "sderr", "sderr", "sderr" };
  std::string GZ_stat[ GZ_size ] = { "nread", "nwritten", "reads", "writes", "rtime", 
          "wtime", "rlentime", "wlentime", "Hard Errors", "Soft Errors", "Transport Errors" };

  /* Other */
  const size_t size = 14;
  std::string modl[ size ] = { "zone_zfs", "zone_zfs", "zone_zfs", "zone_zfs", 
                  "zone_zfs", "zone_zfs", "zone_vfs", "zone_vfs", "zone_vfs", 
                  "zone_vfs", "zone_vfs", "zone_vfs", "zone_vfs", "zone_vfs" };
  std::string stat[ size ] = { "nread", "nwritten", "reads", "writes", "rtime", "rlentime", 
        "nread", "nwritten", "reads", "rlentime", "rtime", "wlentime", "writes", "wtime" };
}

namespace DTRACE {
#ifdef ZONE
  const size_t number = 9;
#else
  const size_t number = 10;
#endif

  std::string dtrace[number] = {
    std::string("profile:::profile-4999\n{\n@[0,zonename,cpu] = count();\n}"),

    std::string("profile:::profile-4999\n{\n@[1,zonename,cpu,execname,pid] = count();\n}"),

    std::string("profile:::tick-4999\n{\n@[2,zonename] = count();\n}"),

    std::string("syscall:::entry\n{\nself->begun=timestamp;\n}\nsyscall:::return\n"
        "{\nself->ended=timestamp;\n@[5,zonename]=quantize(self->ended-self->begun);\n}"),

    std::string("profile:::profile-4999\n{\n@[3,zonename,pid] = count();\n}"),

    std::string("profile:::profile-4999\n{\n@[4,zonename,curthread] = count();\n}"),

    std::string("vminfo:::maj_fault\n{\n@[6,zonename] = count();\n}"),

    std::string("vminfo:::as_fault\n{\n@[7,zonename] = count();\n}"),

#ifdef ZONE
    std::string("vminfo:::pgin\n{\n@[8,zonename] = count();\n}")
#else
    std::string("vminfo:::pgin\n{\n@[8,zonename] = count();\n}"),

    std::string("profile:::profile-4999\n/curthread->t_cpu->cpu_disp->disp_nrunnable > "
        "0/\n{\n@[9,\"global\",cpu,curthread->t_cpu->cpu_disp->disp_nrunnable] = count();\n}")
#endif
  };

}
