#include "dupe_file.h"

#include <cppunit/extensions/HelperMacros.h>

class dupe_file_Test : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(dupe_file_Test);
		CPPUNIT_TEST(testCreate);
		CPPUNIT_TEST(testDupe);
		CPPUNIT_TEST(testClear);
		CPPUNIT_TEST(testNonwordBoundry);
	CPPUNIT_TEST_SUITE_END();
	protected:
		dupe_file_checker *dupechecker;
	public:
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
			CPPUNIT_ASSERT(!dupechecker->checkhavefile("boo test.foob (1/1)", "<foo@bar>", 600));//word boundry
			CPPUNIT_ASSERT(!dupechecker->checkhavefile("boo test.foo (1/1)", "<foo@bar>", 400));//too small
			CPPUNIT_ASSERT(!dupechecker->checkhavefile("boo test.foo (1/1)", "<foo@bar>", 1100));//too big
			CPPUNIT_ASSERT(dupechecker->checkhavefile("boo \"test.foo\" (1/1)", "<foo@bar>", 600));//quoted
			CPPUNIT_ASSERT(dupechecker->checkhavefile("test.foo", "<foo@bar>", 600));//no chars before/after
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
			CPPUNIT_ASSERT(dupechecker->checkhavefile("boo \"[bar]test.foo\" (1/1)", "<foo@bar>", 600));
		}
};

CPPUNIT_TEST_SUITE_REGISTRATION( dupe_file_Test );

