#include "dupe_file.h"

#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>

using namespace CppUnit;

class dupe_file_Test : public TestCase {
	protected:
		dupe_file_checker *dupechecker;
	public:
		dupe_file_Test(void):TestCase("dupe_file_Test"){}
		void setUp(void) {
			dupechecker = new dupe_file_checker();
		}
		void tearDown(void) {
			delete dupechecker;
		}
		void testCreate(void) {
			CPPUNIT_ASSERT(dupechecker->empty());
		}
		void testDupe(void) {
			dupechecker->add("test.foo", 500);
			CPPUNIT_ASSERT(!dupechecker->empty());
			CPPUNIT_ASSERT(dupechecker->checkhavefile("boo test.foo (1/1)", "<foo@bar>", 600));
			CPPUNIT_ASSERT(!dupechecker->checkhavefile("boo est.foo (1/1)", "<foo@bar>", 600));
			CPPUNIT_ASSERT(!dupechecker->checkhavefile("boo testxfoo (1/1)", "<foo@bar>", 600));//is . escaped?
			CPPUNIT_ASSERT(!dupechecker->checkhavefile("boo atest.foo (1/1)", "<foo@bar>", 600));//word boundry
			CPPUNIT_ASSERT(!dupechecker->checkhavefile("boo test.foo (1/1)", "<foo@bar>", 400));//too small
			CPPUNIT_ASSERT(!dupechecker->checkhavefile("boo test.foo (1/1)", "<foo@bar>", 1100));//too big
		}
		void testClear(void) {
			dupechecker->add("test.foo", 500);
			dupechecker->clear();
			CPPUNIT_ASSERT(dupechecker->empty());
			CPPUNIT_ASSERT(!dupechecker->checkhavefile("boo test.foo (1/1)", "<foo@bar>", 600));
		}
		void testNonwordBoundry(void) {
			dupechecker->add("[bar]test.foo", 500);
			CPPUNIT_ASSERT(!dupechecker->checkhavefile("boo test.foo (1/1)", "<foo@bar>", 600));
			CPPUNIT_ASSERT(!dupechecker->checkhavefile("boo btest.foo (1/1)", "<foo@bar>", 600));//is [bar] escaped?
			CPPUNIT_ASSERT(dupechecker->checkhavefile("boo [bar]test.foo (1/1)", "<foo@bar>", 600));
		}
		static Test *suite(void) {
			TestSuite *suite = new TestSuite;
#define ADDTEST(n) suite->addTest(new TestCaller<dupe_file_Test>(#n, &dupe_file_Test::n))
			ADDTEST(testCreate);
			ADDTEST(testDupe);
			ADDTEST(testClear);
			ADDTEST(testNonwordBoundry);
#undef ADDTEST
			return suite;
		}
};

