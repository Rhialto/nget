
#include "nrangetest.h"

#include <cppunit/TextTestRunner.h>

int main(void){
	TextTestRunner runner;
	runner.addTest(nrangeTest::suite());
	runner.addTest(nrangeEqTest::suite());
	runner.run();

	return 0;
}
