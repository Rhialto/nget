
#include "nrangetest.h"
#include "dupe_file_test.h"
#include "strtoker_test.h"
#include "rcount_test.h"
#include "misc_test.h"

#include <cppunit/TestSuite.h>
#include <cppunit/TextTestResult.h>
#include <iostream>

int main(void){
	TestSuite suite;
	TextTestResult result;
	suite.addTest(nrangeTest::suite());
	suite.addTest(nrangeEqTest::suite());
	suite.addTest(dupe_file_Test::suite());
	suite.addTest(strtoker_Test::suite());
	suite.addTest(rcount_Test::suite());
	suite.addTest(misc_Test::suite());
	suite.run(&result);
	cout << result;

	return result.wasSuccessful()?0:1;
}
