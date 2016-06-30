
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
  ecl_sum_type* ecl_sum1;
  ecl_sum_type* ecl_sum2;
  ecl_sum_type* ecl_sum_file_short;
  ecl_sum_type * ecl_sum_file_long;
  stringlist_type* keys1;
  stringlist_type* keys2; 
  stringlist_type * keys_short;
  stringlist_type * keys_long;
  double relative_tolerance_max;
  double relative_tolerance_median_max;
  std::vector<double> * referance_vec;
  std::vector<double> * ref_data_vec;
  std::vector<double> * checking_vec;
  std::vector<double> * check_data_vec;
public:
  SummaryReader(): 
    ecl_sum1(nullptr), 
    ecl_sum2(nullptr),
  ecl_sum_file_short(nullptr), 
    ecl_sum_file_long(nullptr),
    referance_vec(nullptr),
    ref_data_vec(nullptr),
    checking_vec(nullptr),
    check_data_vec(nullptr),
     keys1(stringlist_alloc_new()),
    keys2(stringlist_alloc_new()),
    keys_short(stringlist_alloc_new()),
    keys_long(stringlist_alloc_new()) {}
  SummaryReader(double relative_tolerance_max, double relative_tolerance_median_max):
    ecl_sum1(nullptr),
    ecl_sum2(nullptr),
  ecl_sum_file_short(nullptr), 
    ecl_sum_file_long(nullptr),
    referance_vec(nullptr),
    ref_data_vec(nullptr),
    checking_vec(nullptr),
    check_data_vec(nullptr),
    relative_tolerance_max(relative_tolerance_max),
    relative_tolerance_median_max(relative_tolerance_median_max),
    keys1(stringlist_alloc_new()),
    keys2(stringlist_alloc_new()),
    keys_short(stringlist_alloc_new()),
    keys_long(stringlist_alloc_new())
  {} 


  bool open(const char* smspecFile1, const stringlist_type* unsmryFile1, const char* smspecFile2, const stringlist_type* unsmryFile2);
  void close();
  void setDataSets(std::vector<double> &time_vec1,std::vector<double> &time_vec2);
  void setKeys(); // Creates a keylist containing all the keywords in a summary file. When the number of keywords are not
  // equal in the two files of interest, it figures out which that contains most/less keywords
  void setToleranceLevels(double relative_tolerance_max, double relative_tolerance_median_max);
  void setTimeVecs(std::vector<double> &time_vec1,std::vector<double> &time_vec2);
  void setDataVecs(std::vector<double> &data_vec1,std::vector<double> &data_vec2, int index1, int index2); //Read the data from the two files into separate vectors. Not necessary the same amount of values, but the values correspond to the same interval in time. Thus possible to interpolate values. 
  void getDeviations(); 
  void chooseReferance(std::vector<double> &time_vec1,std::vector<double> &time_vec2,std::vector<double> &data_vec1,std::vector<double> &data_vec2);
  void evaluateDeviations(std::vector<double> &absdev_vec, std::vector<double> &reldev_vec);

  static Deviation calculateDeviations( double val1, double val2); static double max(std::vector<double>& vec);
  static double median(std::vector<double>& vec);
  static double linearPolation(double check_value, double check_value_prev, double time_array[3]);
  static void findDeviations(std::vector<double>& absdev_vec,std::vector<double>& reldev_vec,std::vector<double> *referance_vec,std::vector<double>* checking_vec ,std::vector<double> *ref_data_vec,std::vector<double> *check_data_vec);
  static double average(std::vector<double> &vec);
};

