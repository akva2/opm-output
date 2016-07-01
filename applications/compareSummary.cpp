/*
  Copyright 2016 Statoil ASA.
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <opm/output/eclipse/summaryReader.hpp>
#include <ert/ecl/ecl_sum.h>
#include <ert/util/stringlist.h>
#include <ert/util/int_vector.h>
#include <ert/util/bool_vector.h>


void printHelp(){
  std::cout << "Invalid input." << std::endl;
  std::cout << "the arguments are:" << std::endl;
  std::cout << "1) <path to file1>/<base_name>.SMSPEC" << std::endl;
  std::cout << "2) <path to file1>/<base_name>.UNSMRY" << std::endl;
  std::cout << "3) <path to file2>/<base_name>.SMSPEC" << std::endl;
  std::cout << "4) <path to file2>/<base_name>.UNSMRY" << std::endl;
}

//---------------------------------------------------


int main (int argc, char ** argv){
  if(argc != 5){
    printHelp();
    return 0;
  }
  try
    {
      const char * smspecFile1 = argv[1];
      const char * unsmryFile1 = argv[2];
      const char * smspecFile2 = argv[3];
      const char * unsmryFile2 = argv[4];
      double relative_tolerance_max = 1 ;
      double relative_tolerance_median_max = 0.1;
      stringlist_type * file1 = stringlist_alloc_new();
      stringlist_type * file2 = stringlist_alloc_new();
      stringlist_append_copy(file1, unsmryFile1);
      stringlist_append_copy(file2, unsmryFile2);
 

      SummaryReader read(relative_tolerance_max,relative_tolerance_median_max); //may also use sat function for tolerance limits
      read.open(smspecFile1, file1, smspecFile2, file2);
      read.setKeys();
      read.getDeviations();
      read.close();
      stringlist_free(file1);
      stringlist_free(file2);
    }
  catch(const std::exception& e) {
    std::cerr << "Program threw an exception: " << e.what() << std::endl; 
    return EXIT_FAILURE;
  }
  return 0; 
}


