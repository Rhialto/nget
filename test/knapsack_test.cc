#include "knapsack.h"

#include <cppunit/extensions/HelperMacros.h>


class knapsack_Test : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(knapsack_Test);
		CPPUNIT_TEST(testSimple);
		CPPUNIT_TEST(test2);
		CPPUNIT_TEST(testValue);
		CPPUNIT_TEST(test3);
		CPPUNIT_TEST(test4);
		CPPUNIT_TEST(test4_2);
		CPPUNIT_TEST(test4_3);
	CPPUNIT_TEST_SUITE_END();
	protected:
		vector<int> values, sizes;
		set<int> results, results2, mresults, mresults2;
	public:
		void setUp(void) {
			values.clear(); sizes.clear();
			results.clear(); results2.clear();
			mresults.clear(); mresults2.clear();
		}
		void testSimple(void) {
			int foo[] = {1,2,4,8,16,32};
			values.insert(values.begin(), foo, foo+sizeof(foo)/sizeof(int));
			sizes.insert(sizes.begin(), foo, foo+sizeof(foo)/sizeof(int));
			int v = knapsack(values, sizes, 25, results);
			int ms = knapsack_minsize(values, sizes, 25, mresults);
			CPPUNIT_ASSERT_EQUAL(25, v);
			CPPUNIT_ASSERT_EQUAL(25, ms);
			CPPUNIT_ASSERT_EQUAL(3, (int)results.size());
			CPPUNIT_ASSERT(results.count(0));
			CPPUNIT_ASSERT(results.count(3));
			CPPUNIT_ASSERT(results.count(4));
			CPPUNIT_ASSERT(results == mresults);
		}
		void test2(void) {
			int foo[] = {7,23,45,78};
			values.insert(values.begin(), foo, foo+sizeof(foo)/sizeof(int));
			sizes.insert(sizes.begin(), foo, foo+sizeof(foo)/sizeof(int));
			int v = knapsack(values, sizes, 70, results);
			CPPUNIT_ASSERT_EQUAL(68, v);
			CPPUNIT_ASSERT_EQUAL(2, (int)results.size());
			CPPUNIT_ASSERT(results.count(1));
			CPPUNIT_ASSERT(results.count(2));
			int ms = knapsack_minsize(values, sizes, 70, mresults);
			CPPUNIT_ASSERT_EQUAL(75, ms);
			CPPUNIT_ASSERT_EQUAL(3, (int)mresults.size());
			CPPUNIT_ASSERT(mresults.count(0));
			CPPUNIT_ASSERT(mresults.count(1));
			CPPUNIT_ASSERT(mresults.count(2));
		}
		void testValue(void) {
			int vfoo[] = {5, 3, 4, 4, 8};
			int sfoo[] = {5, 4, 4, 4, 10};
			values.insert(values.begin(), vfoo, vfoo+sizeof(vfoo)/sizeof(int));
			sizes.insert(sizes.begin(), sfoo, sfoo+sizeof(sfoo)/sizeof(int));
			int v = knapsack(values, sizes, 8, results);
			CPPUNIT_ASSERT_EQUAL(8, v);
			CPPUNIT_ASSERT_EQUAL(2, (int)results.size());
			CPPUNIT_ASSERT(results.count(2));
			CPPUNIT_ASSERT(results.count(3));
			
			v = knapsack(values, sizes, 10, results2);
			CPPUNIT_ASSERT_EQUAL(9, v);
			CPPUNIT_ASSERT_EQUAL(2, (int)results2.size());
			CPPUNIT_ASSERT(results2.count(0));
			CPPUNIT_ASSERT(results2.count(2));
			
			int ms = knapsack_minsize(values, sizes, 8, mresults);
			CPPUNIT_ASSERT_EQUAL(8, ms);
			CPPUNIT_ASSERT_EQUAL(2, (int)mresults.size());
			CPPUNIT_ASSERT(mresults.count(2));
			CPPUNIT_ASSERT(mresults.count(3));
			
			ms = knapsack_minsize(values, sizes, 10, mresults2);
			CPPUNIT_ASSERT_EQUAL(12, ms);
			CPPUNIT_ASSERT_EQUAL(3, (int)mresults2.size());
			CPPUNIT_ASSERT(mresults2.count(1));
			CPPUNIT_ASSERT(mresults2.count(2));
			CPPUNIT_ASSERT(mresults2.count(3));
		}
		void test3(void) {
			int foo[] = {1, 2, 2};
			values.insert(values.begin(), foo, foo+sizeof(foo)/sizeof(int));
			sizes.insert(sizes.begin(), foo, foo+sizeof(foo)/sizeof(int));
			int v = knapsack(values, sizes, 2, results);
			CPPUNIT_ASSERT_EQUAL(2, v);
			CPPUNIT_ASSERT_EQUAL(1, (int)results.size());
			CPPUNIT_ASSERT(results.count(1) || results.count(2));
			
			int ms = knapsack_minsize(values, sizes, 2, mresults);
			CPPUNIT_ASSERT_EQUAL(2, ms);
			CPPUNIT_ASSERT_EQUAL(1, (int)mresults.size());
			CPPUNIT_ASSERT(mresults.count(1) || mresults.count(2));
		}	
		void test4(void) {
			int foo[] = {8,8,8,8,7,7,7,7,7,7};
			values.insert(values.begin(), foo, foo+sizeof(foo)/sizeof(int));
			sizes.insert(sizes.begin(), foo, foo+sizeof(foo)/sizeof(int));
			int v = knapsack(values, sizes, 9, results);
			CPPUNIT_ASSERT_EQUAL(8, v);
			CPPUNIT_ASSERT_EQUAL(1, (int)results.size());
			CPPUNIT_ASSERT(!(results.count(4) || results.count(5) || results.count(6) || results.count(7) || results.count(8) || results.count(9)));
			
			int ms = knapsack_minsize(values, sizes, 9, mresults);
			CPPUNIT_ASSERT_EQUAL(14, ms);
			CPPUNIT_ASSERT_EQUAL(2, (int)mresults.size());
			CPPUNIT_ASSERT(!(mresults.count(0) || mresults.count(1) || mresults.count(2) || mresults.count(3)));
		}
		void test4_2(void) {
			int foo[] = {7,7,7,7,7,7,8,8,8,8};
			values.insert(values.begin(), foo, foo+sizeof(foo)/sizeof(int));
			sizes.insert(sizes.begin(), foo, foo+sizeof(foo)/sizeof(int));
			int v = knapsack(values, sizes, 9, results);
			CPPUNIT_ASSERT_EQUAL(8, v);
			CPPUNIT_ASSERT_EQUAL(1, (int)results.size());
			CPPUNIT_ASSERT(!(results.count(0) || results.count(1) || results.count(2) || results.count(3) || results.count(4) || results.count(5)));
			
			int ms = knapsack_minsize(values, sizes, 9, mresults);
			CPPUNIT_ASSERT_EQUAL(14, ms);
			CPPUNIT_ASSERT_EQUAL(2, (int)mresults.size());
			CPPUNIT_ASSERT(!(mresults.count(6) || mresults.count(7) || mresults.count(8) || mresults.count(9)));
		}
		void test4_3(void) {
			int foo[] = {8,8,7,7,7};
			values.insert(values.begin(), foo, foo+sizeof(foo)/sizeof(int));
			sizes.insert(sizes.begin(), foo, foo+sizeof(foo)/sizeof(int));
			int v = knapsack(values, sizes, 9, results);
			CPPUNIT_ASSERT_EQUAL(8, v);
			CPPUNIT_ASSERT_EQUAL(1, (int)results.size());
			CPPUNIT_ASSERT(results.count(0) || results.count(1));
			
			int ms = knapsack_minsize(values, sizes, 9, mresults);
			CPPUNIT_ASSERT_EQUAL(14, ms);
			CPPUNIT_ASSERT_EQUAL(2, (int)mresults.size());
			CPPUNIT_ASSERT(!(mresults.count(0) || mresults.count(1)));
		}
};

CPPUNIT_TEST_SUITE_REGISTRATION( knapsack_Test );



