#include "nrange.h"

#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>

using namespace CppUnit;

class nrangeTest : public TestCase {
	protected:
		c_nrange *range;
	public:
		nrangeTest(void):TestCase("nrangeTest"){}
		void setUp(void) {
			range = new c_nrange();
		}
		void tearDown(void) {
			delete range;
		}
		void testCreate(void) {
			CPPUNIT_ASSERT(range->empty());
			CPPUNIT_ASSERT(range->get_total() == 0);
			CPPUNIT_ASSERT(range->check(0) == 0);
			CPPUNIT_ASSERT(range->check(1) == 0);
			CPPUNIT_ASSERT(range->check(ULONG_MAX) == 0);
		}
		void testInsert(void) {
			range->insert(1);
			CPPUNIT_ASSERT(!range->empty());
			CPPUNIT_ASSERT(range->get_total() == 1);
			CPPUNIT_ASSERT(range->low() == range->high() == 1);
			range->insert(2);
			CPPUNIT_ASSERT(!range->empty());
			CPPUNIT_ASSERT(range->num_ranges() == 1);
			CPPUNIT_ASSERT(range->get_total() == 2);
			CPPUNIT_ASSERT(range->low() == 1);
			CPPUNIT_ASSERT(range->high() == 2);
			CPPUNIT_ASSERT(!range->check(0));
			CPPUNIT_ASSERT(range->check(1));
			CPPUNIT_ASSERT(range->check(2));
			CPPUNIT_ASSERT(!range->check(3));
		}
		void testInsertNothing(void) {
			range->insert(1);
			range->insert(1);
			CPPUNIT_ASSERT(!range->empty());
			CPPUNIT_ASSERT(range->num_ranges() == 1);
			CPPUNIT_ASSERT(range->get_total() == 1);
			CPPUNIT_ASSERT(range->low() == range->high() == 1);
			CPPUNIT_ASSERT(!range->check(0));
			CPPUNIT_ASSERT(range->check(1));
			CPPUNIT_ASSERT(!range->check(2));
		}
		void testInsertMiddle(void) {
			range->insert(1);
			range->insert(3);
			CPPUNIT_ASSERT(range->get_total() == 2);
			CPPUNIT_ASSERT(range->low() == 1);
			CPPUNIT_ASSERT(range->high() == 3);
			range->insert(2);
			CPPUNIT_ASSERT(range->get_total() == 3);
			CPPUNIT_ASSERT(range->num_ranges() == 1);
			CPPUNIT_ASSERT(range->low() == 1);
			CPPUNIT_ASSERT(range->high() == 3);
			CPPUNIT_ASSERT(!range->check(0));
			CPPUNIT_ASSERT(range->check(1));
			CPPUNIT_ASSERT(range->check(2));
			CPPUNIT_ASSERT(range->check(3));
			CPPUNIT_ASSERT(!range->check(4));
		}
		void testInsertMulti(void) {
			range->insert(1,2);
			CPPUNIT_ASSERT(!range->empty());
			CPPUNIT_ASSERT(range->get_total() == 2);
			CPPUNIT_ASSERT(range->low() == 1);
			CPPUNIT_ASSERT(range->high() == 2);
			range->insert(3,4);
			CPPUNIT_ASSERT(!range->empty());
			CPPUNIT_ASSERT(range->num_ranges() == 1);
			CPPUNIT_ASSERT(range->get_total() == 4);
			CPPUNIT_ASSERT(range->low() == 1);
			CPPUNIT_ASSERT(range->high() == 4);
			CPPUNIT_ASSERT(!range->check(0));
			CPPUNIT_ASSERT(range->check(1));
			CPPUNIT_ASSERT(range->check(2));
			CPPUNIT_ASSERT(range->check(3));
			CPPUNIT_ASSERT(range->check(4));
			CPPUNIT_ASSERT(!range->check(5));
		}
		void testRemove(void) {
			range->insert(1,3);
			range->remove(2);
			CPPUNIT_ASSERT(range->num_ranges() == 2);
			CPPUNIT_ASSERT(range->get_total() == 2);
			CPPUNIT_ASSERT(range->low() == 1);
			CPPUNIT_ASSERT(range->high() == 3);
			CPPUNIT_ASSERT(!range->check(0));
			CPPUNIT_ASSERT(range->check(1));
			CPPUNIT_ASSERT(!range->check(2));
			CPPUNIT_ASSERT(range->check(3));
			CPPUNIT_ASSERT(!range->check(4));
		}
		void testRemoveBeg(void) {
			range->insert(1,3);
			range->remove(1);
			CPPUNIT_ASSERT(range->num_ranges() == 1);
			CPPUNIT_ASSERT(range->get_total() == 2);
			CPPUNIT_ASSERT(range->low() == 2);
			CPPUNIT_ASSERT(range->high() == 3);
			CPPUNIT_ASSERT(!range->check(0));
			CPPUNIT_ASSERT(!range->check(1));
			CPPUNIT_ASSERT(range->check(2));
			CPPUNIT_ASSERT(range->check(3));
			CPPUNIT_ASSERT(!range->check(4));
		}
		void testRemoveEnd(void) {
			range->insert(1,3);
			range->remove(3);
			CPPUNIT_ASSERT(range->num_ranges() == 1);
			CPPUNIT_ASSERT(range->get_total() == 2);
			CPPUNIT_ASSERT(range->low() == 1);
			CPPUNIT_ASSERT(range->high() == 2);
			CPPUNIT_ASSERT(!range->check(0));
			CPPUNIT_ASSERT(range->check(1));
			CPPUNIT_ASSERT(range->check(2));
			CPPUNIT_ASSERT(!range->check(3));
			CPPUNIT_ASSERT(!range->check(4));
		}
		void testRemoveNothingBeg(void) {
			range->insert(1,2);
			range->remove(0);
			CPPUNIT_ASSERT(range->num_ranges() == 1);
			CPPUNIT_ASSERT(range->get_total() == 2);
			CPPUNIT_ASSERT(range->low() == 1);
			CPPUNIT_ASSERT(range->high() == 2);
			CPPUNIT_ASSERT(!range->check(0));
			CPPUNIT_ASSERT(range->check(1));
			CPPUNIT_ASSERT(range->check(2));
			CPPUNIT_ASSERT(!range->check(3));
		}
		void testRemoveNothingEnd(void) {
			range->insert(1,2);
			range->remove(3);
			CPPUNIT_ASSERT(range->num_ranges() == 1);
			CPPUNIT_ASSERT(range->get_total() == 2);
			CPPUNIT_ASSERT(range->low() == 1);
			CPPUNIT_ASSERT(range->high() == 2);
			CPPUNIT_ASSERT(!range->check(0));
			CPPUNIT_ASSERT(range->check(1));
			CPPUNIT_ASSERT(range->check(2));
			CPPUNIT_ASSERT(!range->check(3));
		}
		void testInvertEmpty(void) {
			c_nrange i;
			i.invert(*range);
			CPPUNIT_ASSERT(i.num_ranges() == 1);
			CPPUNIT_ASSERT(i.get_total() == ULONG_MAX+1);
			CPPUNIT_ASSERT(i.low() == 0);
			CPPUNIT_ASSERT(i.high() == ULONG_MAX);
			CPPUNIT_ASSERT(i.check(0));
			CPPUNIT_ASSERT(i.check(5));
			CPPUNIT_ASSERT(i.check(ULONG_MAX));
		}
		void testInvertEnds(void) {
			range->insert(0);
			range->insert(5);
			range->insert(ULONG_MAX);
			c_nrange i;
			i.invert(*range);
			CPPUNIT_ASSERT(i.num_ranges() == 2);
			CPPUNIT_ASSERT(i.get_total() == ULONG_MAX-2);
			CPPUNIT_ASSERT(i.low() == 1);
			CPPUNIT_ASSERT(i.high() == ULONG_MAX-1);
			CPPUNIT_ASSERT(!i.check(0));
			CPPUNIT_ASSERT(i.check(1));
			CPPUNIT_ASSERT(i.check(4));
			CPPUNIT_ASSERT(!i.check(5));
			CPPUNIT_ASSERT(i.check(6));
			CPPUNIT_ASSERT(i.check(ULONG_MAX)-1);
			CPPUNIT_ASSERT(!i.check(ULONG_MAX));
		}
		void testInvertNoEnds(void) {
			range->insert(1);
			range->insert(5);
			range->insert(9);
			c_nrange i;
			i.invert(*range);
			CPPUNIT_ASSERT(i.num_ranges() == 4);
			CPPUNIT_ASSERT(i.get_total() == ULONG_MAX-2);
			CPPUNIT_ASSERT(i.low() == 0);
			CPPUNIT_ASSERT(i.high() == ULONG_MAX);
			for (int j=0; j<15; ++j)
				CPPUNIT_ASSERT(i.check(j)!=range->check(j));
			CPPUNIT_ASSERT(i.check(ULONG_MAX));
		}
		static Test *suite(void) {
			TestSuite *suite = new TestSuite;
#define ADDTEST(n) suite->addTest(new TestCaller<nrangeTest>(#n, &nrangeTest::n))
			ADDTEST(testCreate);
			ADDTEST(testInsert);
			ADDTEST(testInsertMiddle);
			ADDTEST(testInsertNothing);
			ADDTEST(testInsertMulti);
			ADDTEST(testRemove);
			ADDTEST(testRemoveBeg);
			ADDTEST(testRemoveEnd);
			ADDTEST(testRemoveNothingBeg);
			ADDTEST(testRemoveNothingEnd);
			ADDTEST(testInvertEmpty);
			ADDTEST(testInvertEnds);
			ADDTEST(testInvertNoEnds);
#undef ADDTEST
			return suite;
		}
};
class nrangeEqTest : public TestCase {
	protected:
		c_nrange *range;
		c_nrange *rangeb;
	public:
		nrangeEqTest(void):TestCase("nrangeEqTest"){}
		void setUp(void) {
			range = new c_nrange();
			rangeb = new c_nrange();
		}
		void tearDown(void) {
			delete range;
			delete rangeb;
		}
		void testEmpty(void) {
			CPPUNIT_ASSERT(*range == *rangeb);
			CPPUNIT_ASSERT(range != rangeb);
		}
		void testOneEmpty(void) {
			range->insert(1);
			CPPUNIT_ASSERT(*range != *rangeb);
		}
		void testUnEqual(void) {
			range->insert(1);
			rangeb->insert(2);
			CPPUNIT_ASSERT(*range != *rangeb);
		}
		void testEqual(void) {
			range->insert(1);
			range->insert(2);
			rangeb->insert(1,2);
			CPPUNIT_ASSERT(*range == *rangeb);
		}
		static Test *suite(void) {
			TestSuite *suite = new TestSuite;
#define ADDTEST(n) suite->addTest(new TestCaller<nrangeEqTest>(#n, &nrangeEqTest::n))
			ADDTEST(testEmpty);
			ADDTEST(testOneEmpty);
			ADDTEST(testUnEqual);
			ADDTEST(testEqual);
#undef ADDTEST
			return suite;
		}
};

