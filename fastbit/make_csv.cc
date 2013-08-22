/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  make_csv.cc
 *    Takes input directory, reads
 *    FASTBIT ibis::table data
 *    stored in that location and
 *    writes it to a standard .csv
 *    file.
 *
 *    CREATED:  30 JULY 2013
 *    UPDATED:  30 JULY 2013
 */
#include <ibis.h>

/*
 * entry point
 */
int main (int argc, char **argv) {

  bool cmdl_args = false;
  std::ostringstream csv_name;
  std::ostringstream dir;

  /* Parse args from command line if possible */
  if (argc == 3) {
    cmdl_args = true;
    dir << argv[1];
    csv_name << argv[2] << ".csv";
  }

  /* Get database path */
  if (!cmdl_args) {
    std::cout << "\nEnter the path to the storage\n";
    std::cout << "location of the database: ";
    {
      std::string input = "";
      std::cin >> input;
      dir << input;
    }
  }

  /* Init ibis and load table */
  ibis::init();
  ibis::gVerbose = 0;
  ibis::table *tbl = 0;
  tbl = ibis::table::create ((const char *)(dir.str().c_str()));

  /* Get important table statistics */
  const uint64_t columns = tbl->nColumns ();
  const uint64_t rows = tbl->nRows ();
  ibis::table::stringList cNames = tbl->columnNames ();
  std::cout << "\nTable has " << rows << " rows and " <<
              columns << " columns.";

  /* Get csv filename from user */
  if (!cmdl_args) {
    std::cout << "\nLoading database from file\n";
    std::cout << "\nEnter a name for the output csv file\n"
                "and ignore the extension: ";
    {
      std::string input = "";
      std::cin >> input;
      csv_name << input;
      csv_name << ".csv";
    }
  }

  /* Open CSV file for writing */
  std::cout << "\nOpening csv file..." << std::endl;
  FILE *f_csv;
  f_csv = fopen ((const char *)(csv_name.str().c_str()), "w");

  /* Write to file */
  std::cout << "Writing to csv file..." << std::endl;
  std::ostringstream buffer;
  tbl->dumpNames (buffer, (const char *)",");
  fputs (buffer.str().c_str(), f_csv);
  buffer.str("");
  buffer.clear();
  tbl->dump (buffer, (const char*)",");
  fputs (buffer.str().c_str(), f_csv);

  /* Close CSV file */
  fclose (f_csv);

  std::cout << "Process completed. File saved and closed.\n\n";

  return 0;
}
