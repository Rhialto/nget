
#include "nrangetest.h"
#include "dupe_file_test.h"
#include "strtoker_test.h"
#include "rcount_test.h"
#include "misc_test.h"

#include <cppunit/TextTestRunner.h>

int main(void){
	TextTestRunner runner;
	runner.addTest(nrangeTest::suite());
	runner.addTest(nrangeEqTest::suite());
	runner.addTest(dupe_file_Test::suite());
	runner.addTest(strtoker_Test::suite());
	runner.addTest(rcount_Test::suite());
	runner.addTest(misc_Test::suite());
	runner.run();

	return 0;
}
