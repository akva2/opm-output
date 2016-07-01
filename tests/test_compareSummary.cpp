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
#include <opm/output/eclipse/summaryReader.hpp>
#include "config.h"

#if HAVE_DYNAMIC_BOOST_TEST
#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE CalculationTest
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE(deviation){
  double a = 5;
  double b = 10;
  const double tol = 1.0e-14;
  
  Deviation dev = SummaryReader::calculateDeviations(a,b);
  BOOST_CHECK_EQUAL(dev.absolute_deviation, 5);
  BOOST_CHECK_CLOSE(dev.relative_deviation, 5.0/10.0, tol);
}


BOOST_AUTO_TEST_CASE(median){
  std::vector<double> vec = {8.6, 0.6 ,0 , 3.0, 7.2};
  BOOST_CHECK_EQUAL(3,SummaryReader::median(vec));
  vec.pop_back();
  BOOST_CHECK_EQUAL(1.8, SummaryReader::median(vec));

  }

BOOST_AUTO_TEST_CASE(linearPolation){
  double timeArray[3] = {6,1,2};
  double linearCheckValues[2] = {6,11};
  double constCheckValues[2] = {3,3};
  
  double linearLP = SummaryReader::linearPolation(linearCheckValues[1],linearCheckValues[0], timeArray);
  double constLP = SummaryReader::linearPolation(constCheckValues[1], constCheckValues[0], timeArray);

  BOOST_CHECK_EQUAL(linearLP,7);
  BOOST_CHECK_EQUAL(constLP,3);
}


BOOST_AUTO_TEST_CASE(average) {
    
  std::vector<double> vec = {1,2,3,4,5,6};
  const double tol = 1.0e-14;

  double average = SummaryReader::average(vec);

  BOOST_CHECK_CLOSE(average, 21.0/6, tol);
}
