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




bool SummaryReader::open(const char* smspecFile1, const stringlist_type* unsmryFile1, const char * smspecFile2, const stringlist_type* unsmryFile2){
  //opens the files
  
  
  ecl_sum1 = ecl_sum_fread_alloc(smspecFile1, unsmryFile1, ":");
  ecl_sum2 = ecl_sum_fread_alloc(smspecFile2, unsmryFile2, ":");
  

  if (ecl_sum1 == nullptr || ecl_sum2 == nullptr) {
    OPM_THROW(std::runtime_error, "Not able to open files");
  }
  else {
    return true;
  }
}

void SummaryReader::close() {
  ecl_sum_free_data(ecl_sum1);
  ecl_sum_free_data(ecl_sum2);
  ecl_sum_free_data(ecl_sum_file_short);
  ecl_sum_free_data(ecl_sum_file_long);
  stringlist_free(keys1);
  stringlist_free(keys2);
  stringlist_free(keys_short);
  stringlist_free(keys_long);
}


void SummaryReader::setKeys(){
  ecl_sum_select_matching_general_var_list( ecl_sum1 , "*" , this->keys1);
  stringlist_sort(this->keys1 , nullptr );
  
  ecl_sum_select_matching_general_var_list( ecl_sum2 , "*" , this->keys2);
  stringlist_sort(this->keys2 , nullptr );

  if(stringlist_get_size(keys1) <= stringlist_get_size(keys2)){
    this->keys_short = this->keys1;
    this->keys_long = this->keys2;
  }else{
    this->keys_short = this->keys2;
    this->keys_long = this->keys1;
  }

 

}

void SummaryReader::getDeviations(){ 
  std::vector<double> time_vec1, time_vec2;
  setTimeVecs(time_vec1, time_vec2);  // Sets the time vectors, they are equal for all keywords (WPOR:PROD01 etc)
  setDataSets(time_vec1, time_vec2);

  std::vector<double> data_vec1, data_vec2, absdev_vec, reldev_vec;
  int ivar = 0;
  //Iterates over all keywords from the restricted file, use iterator "ivar". Searches for a  match in the file with more keywords, use the itarator "jvar".  
  
  while(ivar < stringlist_get_size(keys_short)){
    for (int jvar = 0; jvar < stringlist_get_size(keys_long); jvar++){
      
      //When the keywords are equal, proceed in comparing summary files. 
      if (strcmp(stringlist_iget(keys_short, ivar), stringlist_iget(keys_long, jvar)) == 0){
	       
	data_vec1.clear();  // The script uses the mamberfunction std::vector<>.puch_back, need to make sure the vectors don't 
	data_vec2.clear(); // contain information from previous keywords. 
	absdev_vec.clear();
	reldev_vec.clear();
  

	setDataVecs(data_vec1,data_vec2, ivar, jvar); 

 
	chooseReference(time_vec1, time_vec2,data_vec1,data_vec2);
	findDeviations(absdev_vec, reldev_vec);
	evaluateDeviations(absdev_vec, reldev_vec);
	break;
      }
      //will only enter here if no keyword match
      if(jvar == stringlist_get_size(keys_long)-1){
	OPM_THROW(std::invalid_argument, "No match on keyword");
      }
    }
    ivar++;
  }


}







Deviation SummaryReader::calculateDeviations(double val1, double val2){
  //takes in two values as argument, calculates the deviation. returns it as a struct Deviation
  double abs_deviation, rel_deviation;
  Deviation deviation; 
  if(val1 >= 0 && val2 >= 0){
    abs_deviation =  std::max(val1, val2) - std::min(val1, val2);
      
    deviation.absolute_deviation = abs_deviation;
    if(val1 == 0 && val2 == 0){}
    else{
      rel_deviation = abs_deviation/double(std::max(val1, val2));
      deviation.relative_deviation = rel_deviation;
    }

  }
  return deviation;
}



void SummaryReader::setTimeVecs(std::vector<double> &time_vec1,std::vector<double> &time_vec2){
  int first_report1 = ecl_sum_get_first_report_step( ecl_sum1 );
  int last_report1  = ecl_sum_get_last_report_step( ecl_sum1 );
  int first_report2 = ecl_sum_get_first_report_step( ecl_sum2 );
  int last_report2  = ecl_sum_get_last_report_step( ecl_sum2 );
  
  
  //Calculates the time vectors
  for(int report = first_report1; report <= last_report1; report++){

    int time_index;
    time_index = ecl_sum_iget_report_end( ecl_sum1 , report );
    time_vec1.push_back( ecl_sum_iget_sim_days(ecl_sum1 , time_index));
  }
  
  for(int report = first_report2; report <= last_report2; report++){
    int time_index;
    time_index = ecl_sum_iget_report_end( ecl_sum2, report );
    time_vec2.push_back(ecl_sum_iget_sim_days(ecl_sum2 , time_index ));
  }

}


//Read the data from the two files into separate vectors. Not necessary the same amount of values, but the values correspond to the same interval in time. Thus possible to interpolate values. 
void SummaryReader::setDataVecs(std::vector<double> &data_vec1,std::vector<double> &data_vec2, int index1, int index2){
  //bool_vector_type * and int_vector_type * variables makes it possible to get the data corresponding to a certain keyword.
  
  bool_vector_type  * has_var1   = bool_vector_alloc( stringlist_get_size( keys_short ), false );
  int_vector_type   * var_index1 = int_vector_alloc( stringlist_get_size( keys_short ), -1 );
  bool_vector_iset( has_var1 , index1 , true );
  int_vector_iset( var_index1 , index1 , ecl_sum_get_general_var_params_index( ecl_sum_file_short , stringlist_iget( keys_short , index1) ));


  bool_vector_type  * has_var2   = bool_vector_alloc( stringlist_get_size( keys_long ), false );
  int_vector_type   * var_index2 = int_vector_alloc( stringlist_get_size( keys_long ), -1 );

  bool_vector_iset( has_var2 , index2 , true );
  int_vector_iset( var_index2 , index2 , ecl_sum_get_general_var_params_index( ecl_sum_file_long , stringlist_iget( keys_long , index2) ));

  int first_report1 = ecl_sum_get_first_report_step( ecl_sum_file_short );
  int last_report1  = ecl_sum_get_last_report_step( ecl_sum_file_short );
  int first_report2 = ecl_sum_get_first_report_step( ecl_sum_file_long );
  int last_report2  = ecl_sum_get_last_report_step( ecl_sum_file_long );
  
  //Calculates the data, function int_vector_iget() keeps track of which keyword to get data from.
  for(int report = first_report1; report <= last_report1; report++){
    int time_index;
    time_index = ecl_sum_iget_report_end( ecl_sum_file_short , report );
    data_vec1.push_back(ecl_sum_iget(ecl_sum_file_short, time_index, int_vector_iget( var_index1 , index1 )));
  }

  for(int report = first_report2; report <= last_report2; report++){
    int time_index;
    time_index = ecl_sum_iget_report_end( ecl_sum_file_long , report );
    data_vec2.push_back(ecl_sum_iget(ecl_sum_file_long, time_index, int_vector_iget( var_index2 , index2 )));
  }
}

  void SummaryReader::setDataSets(std::vector<double> &time_vec1,std::vector<double> &time_vec2){
    if(time_vec1.size()< time_vec2.size()){
      ecl_sum_file_short = this->ecl_sum1;
      ecl_sum_file_long = this->ecl_sum2;
    }
    else{
      ecl_sum_file_short = this->ecl_sum2;
      ecl_sum_file_long = this->ecl_sum1;
    }
  }
   


//Figures out which time vector that contains the fewer elements. Sets this as reference_vec and its corresponding 
// data as ref_data_vec. The other vector-set as checking_vec( the time vector) and check_data_vec.
void SummaryReader::chooseReference(std::vector<double> &time_vec1,std::vector<double> &time_vec2,std::vector<double> &data_vec1,std::vector<double> &data_vec2){
  if(time_vec1.size() <= time_vec2.size()){
    reference_vec = &time_vec1; // time vector
    ref_data_vec = &data_vec1; //data vector
    checking_vec = &time_vec2;
    check_data_vec = &data_vec2;     
  }
  else{
    reference_vec = &time_vec2;
    ref_data_vec = &data_vec2;
    checking_vec = &time_vec1;
    check_data_vec = &data_vec1;
  }


}

void SummaryReader::findDeviations(std::vector<double>& absdev_vec,std::vector<double>& reldev_vec){
    
  int jvar = 0 ;
  Deviation dev; 

  //here the reference and checking vectors are in use. Iterate over the reference vector
  //and tries to match it with the checking vector
  for (int ivar = 0; ivar < reference_vec->size(); ivar++){

    while (true){
      if((*reference_vec)[ivar] == (*checking_vec)[jvar]){
	//Check without linear interpolation
	dev = SummaryReader::calculateDeviations((*ref_data_vec)[ivar], (*check_data_vec)[jvar]); 
	absdev_vec.push_back(dev.absolute_deviation);
	reldev_vec.push_back(dev.relative_deviation);
	break;
      }
      else if((*reference_vec)[ivar]<(*checking_vec)[jvar]){
	// Check with Linear polized arguments, jvar
	double time_array[3]; // should contain { [time of occurance after reference] , [time of occurance before reference] , [time of reference] }
	time_array[0]= (*checking_vec)[jvar]; 
	time_array[1]= (*checking_vec)[jvar -1];
	time_array[2]= (*reference_vec)[ivar];
	double lp_value = SummaryReader::linearPolation((*check_data_vec)[jvar], (*check_data_vec)[jvar-1],time_array);
	dev = SummaryReader::calculateDeviations((*ref_data_vec)[ivar], lp_value);
	absdev_vec.push_back(dev.absolute_deviation);
	reldev_vec.push_back(dev.relative_deviation);
	//	std::cout << "ref data " << (*ref_data_vec)[ivar] <<  " Check data " << lp_value <<std::endl;

	break; 
      }
      else{
	
	jvar++;
      }
     
    }
  }
}


void SummaryReader::evaluateDeviations(std::vector<double> &absdev_vec, std::vector<double> &reldev_vec){
  if(absdev_vec.empty()|| reldev_vec.empty()){
    OPM_THROW(std::invalid_argument, "No absolute or relative deviations.");
  }


  double avr_absdev = SummaryReader::average(absdev_vec);
  double avr_reldev = SummaryReader::average(reldev_vec);

  double medianValueAbs = SummaryReader::median(absdev_vec);
  double medianValueRel = SummaryReader::median(reldev_vec);

  // Vectors get sorted in function SummaryReader::median(), thus max value at the end of the vectors
  double max_absdev, max_reldev; 
  max_absdev =  absdev_vec.back();
  max_reldev =  reldev_vec.back();
  if(max_reldev > relative_tolerance_max || medianValueRel > relative_tolerance_median_max){
    std::cout << "The maximum relative deviation is " << max_reldev << ".  The tolerance level is " << relative_tolerance_max << std::endl; 
    std::cout << "The median relative deviation is " << medianValueRel << ".  The tolerance level is " << relative_tolerance_median_max << std::endl; 
    OPM_THROW(std::invalid_argument, "The deviation is too large." );
  }

}

void SummaryReader::setToleranceLevels(double relative_tolerance_max, double relative_tolerance_median_max){
  this->relative_tolerance_max = relative_tolerance_max;
  this->relative_tolerance_median_max = relative_tolerance_median_max;
}

double SummaryReader::average(std::vector<double> &vec){
  double sum_vector = 0;
  for (int it = 0; it < vec.size(); it++){
    sum_vector += vec[it];
  }
  return sum_vector/double(vec.size());
}



double SummaryReader::linearPolation(double check_value, double check_value_prev , double time_array[3]){
  //does a Linear Polation
  double time_check = time_array[0]; 
  double time_check_prev = time_array[1];
  double time_reference = time_array[2];
  double sloap, factor, lp_value;
  sloap = (check_value - check_value_prev)/double(time_check - time_check_prev);
  factor = (time_reference - time_check_prev)/double(time_check - time_check_prev);
  lp_value = check_value_prev + factor*sloap*(time_check - time_check_prev); 
  return lp_value;
}

double SummaryReader::median(std::vector<double> &vec) {
  // Sorts and returns the median in a std::vector<double>
  if(vec.empty()){ 
    return 0;
  }
  else {
    std::sort(vec.begin(), vec.end());
    if(vec.size() % 2 == 0)
      return (vec[vec.size()/2 - 1] + vec[vec.size()/2]) / 2;
    else
      return vec[vec.size()/2];
  }
}
