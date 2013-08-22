/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  kstat.hpp
 *   Includes a nice library of functions 
 *   that allow easy interface with libkstat 
 *   from a C++ program.
 *
 *   CREATED:   5 AUG 2013
 *   EDITED:    8 AUG 2013
 */

#include <kstat.h>

extern bool VERBOSE;

namespace KSTAT {

/*
 * Useful function to translate from the
 * ambiguous kstat data form to a uint64_t
 * no matter what (or return 0).
 */
uint64_t translate_to_ui64 (kstat_named_t *knp) {

  switch (knp->data_type) {
    case KSTAT_DATA_CHAR:
      UTIL::yellow();
      std::cout << "Encountered char, unable to return (expected)." << std::endl;
      UTIL::clear();
      return (uint64_t)0;
    case KSTAT_DATA_INT32:
      return (uint64_t)knp->value.i32;
    case KSTAT_DATA_UINT32:
      return (uint64_t)knp->value.ui32;
    case KSTAT_DATA_INT64:
      return (uint64_t)knp->value.i64;
    case KSTAT_DATA_UINT64:
      return (uint64_t)knp->value.ui64;
    case KSTAT_DATA_STRING:
      if (VERBOSE) {
        UTIL::yellow();
        std::cout << "Encounted string, unable to return (expected)." << std::endl;
        UTIL::clear();
      }
      return 0;
    default:
      // We should never end up in here
      UTIL::yellow();
      std::cout << "Something rather peculiar happened @kstat.hpp:" << __LINE__ << std::endl;
      std::cout << "Return type: " << knp->data_type << std::endl;
      UTIL::clear();
      break;
    }

  return 0;
}

/*
 * Returns a single statistic value
 * from kstat.
 */
int retreive_kstat (kstat_ctl_t *kc, std::string module, 
            std::string name, std::string statistic, int instance, 
            uint64_t *value, std::string *klasse=(std::string *)"NULL") {
  kstat_t         *ksp;

  if (VERBOSE) {
    std::cout << "KSTAT " << module << " " << name << " " << statistic << " " <<
            instance << std::endl;
  }
 
  /* Allow for unspecific queries */ 
  if (module=="NULL") {
    module.clear();
  }
  if (name=="NULL") {
    name.clear();
  }
  if (statistic=="NULL") {
    statistic.clear();
  }
  
  ksp = kstat_lookup (kc, (char *)module.c_str(), instance, (char *)name.c_str());
  if (ksp == NULL) {
    std::cout << "Unable to retrieve expected kstat " << module << " " << 
        name << " " << statistic << " " << instance << std::endl;
    std::cout << " @kstat.hpp:" << __LINE__ << std::endl;

    return 1; 
  }

  /* We either deal with NAMED_TYPE or IO_TYPE, and each requires a different
   * method for dealing with the data.
   */
  if (ksp->ks_type == KSTAT_TYPE_NAMED) {

    kstat_named_t *knp;   // NAMED stat data structure
    kstat_read (kc, ksp, NULL);
    knp = (kstat_named_t *)kstat_data_lookup (ksp, (char *)statistic.c_str());
    if (knp == NULL) {
      std::cout << "Unable to retreive expected kstat_named " << module << " " <<
          name << " " << statistic << " " << instance << "kstat.hpp:" << __LINE__ << std::endl;
      return 3;
    }

    /* Return data based on type */
    switch (knp->data_type) {
      case KSTAT_DATA_INT32:
        *value = (uint64_t)knp->value.i32;
        break;
      case KSTAT_DATA_UINT32:
        *value = (uint64_t)knp->value.ui32;
        break;
      case KSTAT_DATA_INT64:  
        *value = (uint64_t)knp->value.i64;
        break;
      case KSTAT_DATA_UINT64:
        *value = knp->value.ui64;
        break;
      default:
        // We should never end up in here
        UTIL::red();
        std::cout << "Something rather peculiar happened." << std::endl;
        std::cout << " @kstat.hpp:" << __LINE__ << std::endl;
        UTIL::clear();
        break;
    }
  } else if (ksp->ks_type == KSTAT_TYPE_IO) {

      kstat_io_t kio;   // IO stat data structure
      kstat_read (kc, ksp, &kio);

      if (&kio == NULL) {
        UTIL::yellow();
        std::cout << "Requested kstat (IO) returned NULL for instance" << std::endl;
        UTIL::clear();
      }

     if (statistic == "nread") {
        *value = (uint64_t)kio.nread;
      } else if (statistic == "nwritten") {
        *value = (uint64_t)kio.nwritten;
      } else if (statistic == "reads") {
        *value = (uint64_t)kio.reads;
      } else if (statistic == "writes") {
        *value = (uint64_t)kio.writes;
      } else if (statistic == "rtime") {
        *value = (uint64_t)kio.rtime;
      } else if (statistic == "wtime") {
        *value = (uint64_t)kio.wtime;
      } else if (statistic == "rlentime") {
        *value = (uint64_t)kio.rlentime;
      } else if (statistic == "wlentime") {
        *value = (uint64_t)kio.wlentime;
      } else {
        UTIL::yellow();
        std::cout << "Unrecognzied statistic for IO kstat.hpp:" << __LINE__ << std::endl;
        UTIL::clear();
      }
  } else {
    UTIL::yellow();
    std::cout << "Unexpected KSTAT type, instead returned type: " << ksp->ks_type << std::endl;
    UTIL::clear();
    return 2;
  }
 
  return 0;
}

/*
 * Allows the return of multiple values
 * in a vector. Works with "named" kstat
 * type.
 */
int retreive_multiple_kstat (kstat_ctl_t *kc, std::string module, 
						      std::string statistic, std::vector<uint64_t> *values,
						      std::vector<std::string> *names, std::vector<std::string> *zones,
                  std::string *klasse = (std::string *)"NULL") {
  /* Init */
  names->clear();
  values->clear();
  zones->clear();
  kstat_t         *ksp;

  ksp = kstat_lookup (kc, (char *)module.c_str(), -1, NULL);
  if (ksp == NULL) {
    UTIL::red();
    std::cout << "Initial kstat lookup failed @kstat.hpp:" << __LINE__ << std::endl;
    UTIL::clear();
    return 1;
  }

  /* We return either NAMED_TYPE or IO_TYPE, depending on ks_type */
  if (ksp->ks_type == KSTAT_TYPE_NAMED) {
    kstat_named_t   *knp;   // NAMED stat data structure
    while (ksp != NULL) {
      if (ksp->ks_type != KSTAT_TYPE_NAMED) {
        ksp = kstat_lookup ((kstat_ctl_t *)(ksp->ks_next), (char *)module.c_str(), -1, NULL); 
        continue;
      }
      kstat_read (kc, ksp, NULL);
      knp = (kstat_named_t *)kstat_data_lookup (ksp, (char *)statistic.c_str());
      if (knp == NULL) {
        UTIL::red();
        std::cout << "Failed to resolve kstat statistic lookup @kstat.hpp:" << __LINE__ << std::endl;
        UTIL::clear();
        return 2;
      }

      if (VERBOSE) std::cout << "Retreived kstat from " << ksp->ks_module << " : " << ksp->ks_name << std::endl;

      names->push_back (std::string(ksp->ks_name));
      values->push_back (translate_to_ui64 (knp));
      {
        kstat_named_t *knp = (kstat_named_t *)kstat_data_lookup (ksp, (char *)"zonename");
        zones->push_back (std::string(KSTAT_NAMED_STR_PTR(knp)).substr(0,30));
      }
      ksp = kstat_lookup ((kstat_ctl_t *)(ksp->ks_next), (char *)module.c_str(), -1, NULL);
    }
  } else if (ksp->ks_type == KSTAT_TYPE_IO) {
    while (ksp != NULL) {
      if ((ksp->ks_type != KSTAT_TYPE_IO) || strcmp(ksp->ks_class, "disk")) {
        ksp = kstat_lookup ((kstat_ctl_t *)(ksp->ks_next), (char *)module.c_str(), -1, NULL);
        continue;
      }

      kstat_io_t kio;   // IO stat data structure
      kstat_read (kc, ksp, &kio);

      if (&kio == NULL) {
        UTIL::yellow();
        std::cout << "Requested kstat returned NULL for instance" << std::endl;
        UTIL::clear();
      }
      
      if (statistic == "nread") {
        values->push_back (kio.nread);
        
      } else if (statistic == "nwritten") {
        values->push_back (kio.nwritten);
      } else if (statistic == "reads") {
        values->push_back (kio.reads);
      } else if (statistic == "writes") {
        values->push_back (kio.writes);
      } else if (statistic == "rtime") {
        values->push_back (kio.rtime);
      } else if (statistic == "wtime") {
        values->push_back (kio.wtime);
      } else if (statistic == "rlentime") {
        values->push_back (kio.rlentime);
      } else if (statistic == "wlentime") {
        values->push_back (kio.wlentime);
      } else {
        UTIL::yellow();
        std::cout << "Unrecognzied statistic for IO kstat.hpp:" << __LINE__ << std::endl;
        UTIL::clear();
      }

      names->push_back (ksp->ks_name);

      {
        kstat_named_t *kk = (kstat_named_t *)kstat_data_lookup (ksp, (char *)"zonename");
        if (kk == NULL)  zones->push_back (std::string ("global"));
        else zones->push_back (std::string (KSTAT_NAMED_STR_PTR(kk)).substr(0,30));
      }

      ksp = kstat_lookup ((kstat_ctl_t *)(ksp->ks_next), (char *)module.c_str(), -1, NULL);
    }
  } else {
    /* We don't handle this type of KSTAT type */
    UTIL::red();
    std::cout << "Irregular KSTAT TYPE" << std::endl;
    UTIL::clear();
  }

  return 0;
}

}
