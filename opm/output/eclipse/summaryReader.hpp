/*
+   Copyright 2016 Statoil ASA.
+
+   This file is part of the Open Porous Media project (OPM).
+
+   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
+   the Free Software Foundation, either version 3 of the License, or
+   (at your option) any later version.
+
+   OPM is distributed in the hope that it will be useful,
+   but WITHOUT ANY WARRANTY; without even the implied warranty of
+   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+   GNU General Public License for more details.
+
+   You should have received a copy of the GNU General Public License
+   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
+   */


#include <ert/ecl/ecl_sum.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <ert/util/stringlist.h>
#include <ert/util/int_vector.h>
#include <ert/util/bool_vector.h>
#include <opm/common/ErrorMacros.hpp>


struct Deviation {
  double relative_deviation; 
  double absolute_deviation; 
};

class SummaryReader {
private:
  ecl_sum_type* ecl_sum1 = nullptr;
  ecl_sum_type* ecl_sum2 = nullptr;
  ecl_sum_type* ecl_sum_file_short = nullptr;
  ecl_sum_type * ecl_sum_file_long = nullptr;
  stringlist_type* keys1 = stringlist_alloc_new();
  stringlist_type* keys2 = stringlist_alloc_new(); 
  stringlist_type * keys_short = stringlist_alloc_new();
  stringlist_type * keys_long = stringlist_alloc_new();
  double relative_tolerance_max = 0;
  double relative_tolerance_median_max = 0;
  double absolute_tolerance_max = 0;
  std::vector<double> * reference_vec = nullptr;
  std::vector<double> * ref_data_vec = nullptr;
  std::vector<double> * checking_vec = nullptr;
  std::vector<double> * check_data_vec = nullptr;
public:
  SummaryReader(){} 

  SummaryReader(double relative_tolerance_max, double relative_tolerance_median_max):
    relative_tolerance_max(relative_tolerance_max),
    relative_tolerance_median_max(relative_tolerance_median_max)
  {} 


  bool open(const char* smspecFile1, const stringlist_type* unsmryFile1, const char* smspecFile2, const stringlist_type* unsmryFile2);
  void close();
  void setDataSets(std::vector<double> &time_vec1,std::vector<double> &time_vec2);
  void setKeys(); // Creates a keylist containing all the keywords in a summary file. When the number of keywords are not
  // equal in the two files of interest, it figures out which that contains most/less keywords
  void setToleranceLevels(double relative_tolerance_max, double relative_tolerance_median_max);
  void setTimeVecs(std::vector<double> &time_vec1,std::vector<double> &time_vec2);
  void setDataVecs(std::vector<double> &data_vec1,std::vector<double> &data_vec2, int index1, int index2); 
  void getDeviations(); 
  void chooseReference(std::vector<double> &time_vec1,std::vector<double> &time_vec2,std::vector<double> &data_vec1,std::vector<double> &data_vec2);
  void findDeviations(std::vector<double>& absdev_vec,std::vector<double>& reldev_vec);
  void evaluateDeviations(std::vector<double> &absdev_vec, std::vector<double> &reldev_vec);

  static Deviation calculateDeviations( double val1, double val2); 
  static double max(std::vector<double>& vec);
  static double median(std::vector<double>& vec);
  static double linearPolation(double check_value, double check_value_prev, double time_array[3]);
  static double average(std::vector<double> &vec);
};

