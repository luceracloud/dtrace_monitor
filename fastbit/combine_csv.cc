/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  combine_csv
 *   Takes a list of input files (without
 *   .csv extension) as command-line args
 *   and outputs a file combined.csv that
 *   is an aggregate of all input files.
 *
 *  CREATED:  1 AUG 2013
 *  EDITED:   1 AUG 2013
 */

#include <iostream>
#include <sstream>

/* entry point */
int main (int argc, char **argv) {

  /* Open output csv file */
  FILE *f_out;
  f_out = fopen ((const char *)"combined.csv", "w");

  /* Loop through all input files */
  for (int i=0; i<argc-1; i++) {

    std::ostringstream fin_name;
    fin_name << argv[i+1] << ".csv";
    std::cout << fin_name.str() << std::endl;

    FILE *f_in;
    f_in = fopen (fin_name.str().c_str(), "r");
    if (f_in == NULL) {
      std::cout << " > Error opening file " << fin_name.str() << std::endl;
      std::cout << " > Continuing...\n" << std::flush;
      continue;
    }

    uint32_t line_no = 0;
    size_t len = 0;
    ssize_t read;
    char *line = NULL;
    while ((read = getline(&line, &len, f_in)) != -1) {
      if (line[0]==10) continue;
      if (line_no==0 && i!=0) {
        line_no++;
        continue;
      } else {
        fputs (line, f_out);
        line_no++;
      }
    }
    fclose (f_in);
  }

  /* Clean up */
  fclose (f_out);

  return 0;
}
