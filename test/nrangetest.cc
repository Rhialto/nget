#include "nrange.h"

#include <cppunit/extensions/HelperMacros.h>

class nrangeTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(nrangeTest);
		CPPUNIT_TEST(testCreate);
		CPPUNIT_TEST(testInsert);
		CPPUNIT_TEST(testInsertMiddle);
		CPPUNIT_TEST(testInsertNothing);
		CPPUNIT_TEST(testInsertMulti);
		CPPUNIT_TEST(testInsertMulti2);
		CPPUNIT_TEST(testInsertMulti3);
		CPPUNIT_TEST(testInsertMulti4);
		CPPUNIT_TEST(testRemove);
		CPPUNIT_TEST(testRemoveBeg);
		CPPUNIT_TEST(testRemoveEnd);
		CPPUNIT_TEST(testRemoveNothingBeg);
		CPPUNIT_TEST(testRemoveNothingEnd);
		CPPUNIT_TEST(testInvertEmpty);
		CPPUNIT_TEST(testInvertEnds);
		CPPUNIT_TEST(testInvertNoEnds);
	CPPUNIT_TEST_SUITE_END();
	protected:
		c_nrange *range;
	public:
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
		void testInsertMulti2(void) {
			range->insert(10,15);
			CPPUNIT_ASSERT(!range->empty());
			CPPUNIT_ASSERT(range->num_ranges() == 1);
			CPPUNIT_ASSERT(range->get_total() == 6);
			CPPUNIT_ASSERT(range->low() == 10);
			CPPUNIT_ASSERT(range->high() == 15);
			range->insert(3,7);
			CPPUNIT_ASSERT(!range->empty());
			CPPUNIT_ASSERT(range->num_ranges() == 2);
			CPPUNIT_ASSERT(range->get_total() == 11);
			CPPUNIT_ASSERT(range->low() == 3);
			CPPUNIT_ASSERT(range->high() == 15);
			for (int i=0;i<3;i++) CPPUNIT_ASSERT(!range->check(i));
			for (int i=8;i<10;i++) CPPUNIT_ASSERT(!range->check(i));
			for (int i=16;i<18;i++) CPPUNIT_ASSERT(!range->check(i));
			for (int i=3;i<=7;i++) CPPUNIT_ASSERT(range->check(i));
			for (int i=10;i<=15;i++) CPPUNIT_ASSERT(range->check(i));
		}
		void testInsertMulti3(void) {
			range->insert(10,15);
			CPPUNIT_ASSERT(!range->empty());
			CPPUNIT_ASSERT(range->num_ranges() == 1);
			CPPUNIT_ASSERT(range->get_total() == 6);
			CPPUNIT_ASSERT(range->low() == 10);
			CPPUNIT_ASSERT(range->high() == 15);
			range->insert(3,12);
			CPPUNIT_ASSERT(!range->empty());
			CPPUNIT_ASSERT(range->num_ranges() == 1);
			CPPUNIT_ASSERT(range->get_total() == 13);
			CPPUNIT_ASSERT(range->low() == 3);
			CPPUNIT_ASSERT(range->high() == 15);
			for (int i=0;i<3;i++) CPPUNIT_ASSERT(!range->check(i));
			for (int i=16;i<18;i++) CPPUNIT_ASSERT(!range->check(i));
			for (int i=3;i<=15;i++) CPPUNIT_ASSERT(range->check(i));
		}
		void testInsertMulti4(void) {
			range->insert(11,16);
			range->insert(8);
			range->insert(3);
			CPPUNIT_ASSERT(!range->empty());
			CPPUNIT_ASSERT(range->num_ranges() == 3);
			CPPUNIT_ASSERT(range->get_total() == 8);
			CPPUNIT_ASSERT(range->low() == 3);
			CPPUNIT_ASSERT(range->high() == 16);
			range->insert(4,12);
			CPPUNIT_ASSERT(!range->empty());
			CPPUNIT_ASSERT(range->num_ranges() == 1);
			CPPUNIT_ASSERT(range->get_total() == 14);
			CPPUNIT_ASSERT(range->low() == 3);
			CPPUNIT_ASSERT(range->high() == 16);
			for (int i=0;i<3;i++) CPPUNIT_ASSERT(!range->check(i));
			for (int i=17;i<19;i++) CPPUNIT_ASSERT(!range->check(i));
			for (int i=3;i<=16;i++) CPPUNIT_ASSERT(range->check(i));
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
};

class nrangeEqTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(nrangeEqTest);
		CPPUNIT_TEST(testEmpty);
		CPPUNIT_TEST(testOneEmpty);
		CPPUNIT_TEST(testUnEqual);
		CPPUNIT_TEST(testEqual);
	CPPUNIT_TEST_SUITE_END();
	protected:
		c_nrange *range;
		c_nrange *rangeb;
	public:
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
};

CPPUNIT_TEST_SUITE_REGISTRATION( nrangeTest );
CPPUNIT_TEST_SUITE_REGISTRATION( nrangeEqTest );

