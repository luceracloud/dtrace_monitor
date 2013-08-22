/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  Visiual colorizing utilities. If you
 *  don't like the color, simply delete
 *  the below defintions' contents.
 *
 *  CREATED:  7 AUG 2013
 *  EDITED:   7 AUG 2013
 */

namespace UTIL {

  /* Colors */
  void black () {
    std::cout << "\033[30m";
  }
  void red () {
    std::cout << "\033[31m";
  }
  void green () {
    std::cout << "\033[32m";
  }
  void blue () {
    std::cout << "\033[34m";
  }
  void purple () {
    std::cout << "\033[35m";
  }
  void cyan () {
    std::cout << "\033[36m";
  }
  void yellow () {
    std::cout << "\033[33m";
  }
  void white () {
    std::cout << "\033[37m";
  }
  void clear () {
    std::cout << "\033[m";
  }

}
