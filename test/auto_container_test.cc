#include <cppunit/extensions/HelperMacros.h>

class RCounter{
	public:
		int *aliveptr;
		bool foo(void){return true;}
		RCounter(int *a):aliveptr(a){++*aliveptr;}
		~RCounter(){--*aliveptr;}
};

#include "auto_vector.h"

class auto_vector_Test : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(auto_vector_Test);
		CPPUNIT_TEST(testClear);
		CPPUNIT_TEST(testErase);
		CPPUNIT_TEST(testPop_back);
		CPPUNIT_TEST(testScope);
	CPPUNIT_TEST_SUITE_END();
	protected:
		int alive;
	public:
		void setUp(void) {
			alive = 0;
		}
		/*void testFailCompile(void) {
			auto_vector<RCounter> v;
			v.push_back(new RCounter(&alive));
			v.insert(v.begin(),v.begin(), v.end());//shouldn't compile
			v.front()=NULL;//shouldn't compile
			*v.begin()=NULL;//shouldn't compile
			auto_vector<RCounter> v2(v);//shouldn't compile
			auto_vector<RCounter> v3;
			v3=v;//shouldn't compile
		}*/
		void testClear(void) {
			auto_vector<RCounter> v;
			v.push_back(new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(1, alive);
			v.push_back(new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(2, alive);
			CPPUNIT_ASSERT_EQUAL(2, (int)v.size());
			v.clear();
			CPPUNIT_ASSERT_EQUAL(0, alive);
			CPPUNIT_ASSERT_EQUAL(0, (int)v.size());
		}
		void testErase(void) {
			auto_vector<RCounter> v;
			v.push_back(new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(1, alive);
			v.push_back(new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(2, alive);
			v.erase(v.begin());
			CPPUNIT_ASSERT_EQUAL(1, alive);
			CPPUNIT_ASSERT_EQUAL(1, (int)v.size());
			v.erase(v.begin());
			CPPUNIT_ASSERT_EQUAL(0, (int)v.size());
			CPPUNIT_ASSERT_EQUAL(0, alive);
		}
		void testPop_back(void) {
			auto_vector<RCounter> v;
			v.push_back(new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(1, alive);
			v.push_back(new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(2, alive);
			v.pop_back();
			CPPUNIT_ASSERT_EQUAL(1, alive);
			CPPUNIT_ASSERT_EQUAL(1, (int)v.size());
			v.pop_back();
			CPPUNIT_ASSERT_EQUAL(0, (int)v.size());
			CPPUNIT_ASSERT_EQUAL(0, alive);
		}
		void testScope(void) {
			{
				auto_vector<RCounter> v;
				v.push_back(new RCounter(&alive));
				CPPUNIT_ASSERT_EQUAL(1, alive);
				v.push_back(new RCounter(&alive));
				CPPUNIT_ASSERT_EQUAL(2, alive);
			}
			CPPUNIT_ASSERT_EQUAL(0, alive);
		}
};

CPPUNIT_TEST_SUITE_REGISTRATION( auto_vector_Test );


#include "auto_map.h"

class auto_map_Test : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(auto_map_Test);
		CPPUNIT_TEST(testClear);
		CPPUNIT_TEST(testErase);
		CPPUNIT_TEST(testScope);
	CPPUNIT_TEST_SUITE_END();
	protected:
		int alive;
	public:
		void setUp(void) {
			alive = 0;
		}
		/*void testFailCompile(void) {
			auto_map<int, RCounter> v;
			v.insert_value(1,new RCounter(&alive));
			v.insert(v.begin(),v.end());//shouldn't compile
			v[1];//shouldn't compile
			v[1]=new RCounter(&alive);//shouldn't compile
			(*v.begin()).second=NULL;//shouldn't compile
			auto_map<int, RCounter> v2(v);//shouldn't compile
			auto_map<int, RCounter> v3;
			v3=v;//shouldn't compile
		}*/
		void testClear(void) {
			auto_map<int, RCounter> v;
			v.insert_value(1, new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(1, alive);
			v.insert_value(2, new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(2, alive);
			v.clear();
			CPPUNIT_ASSERT_EQUAL(0, alive);
		}
		void testErase(void) {
			auto_map<int, RCounter> v;
			v.insert_value(1, new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(1, alive);
			v.insert_value(2, new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(2, alive);
			v.erase(v.begin());
			CPPUNIT_ASSERT_EQUAL(1, alive);
			CPPUNIT_ASSERT_EQUAL(1, (int)v.size());
			v.erase(v.begin());
			CPPUNIT_ASSERT_EQUAL(0, (int)v.size());
			CPPUNIT_ASSERT_EQUAL(0, alive);
		}
		void testScope(void) {
			{
				auto_map<int, RCounter> v;
				v.insert_value(1, new RCounter(&alive));
				CPPUNIT_ASSERT_EQUAL(1, alive);
				v.insert_value(2, new RCounter(&alive));
				CPPUNIT_ASSERT_EQUAL(2, alive);
			}
			CPPUNIT_ASSERT_EQUAL(0, alive);
		}
};

CPPUNIT_TEST_SUITE_REGISTRATION( auto_map_Test );


class auto_multimap_Test : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(auto_multimap_Test);
		CPPUNIT_TEST(testClear);
		CPPUNIT_TEST(testErase);
		CPPUNIT_TEST(testScope);
	CPPUNIT_TEST_SUITE_END();
	protected:
		int alive;
	public:
		void setUp(void) {
			alive = 0;
		}
		/*void testFailCompile(void) {
			auto_map<int, RCounter> v;
			v.insert_value(1,new RCounter(&alive));
			v[1];//shouldn't compile
			v[1]=new RCounter(&alive);//shouldn't compile
			(*v.begin()).second=NULL;//shouldn't compile
			auto_multimap<int, RCounter> v2(v);//shouldn't compile
			auto_multimap<int, RCounter> v3;
			v3=v;//shouldn't compile
		}*/
		void testClear(void) {
			auto_multimap<int, RCounter> v;
			v.insert_value(1, new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(1, alive);
			v.insert_value(2, new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(2, alive);
			v.insert_value(2, new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(3, alive);
			v.clear();
			CPPUNIT_ASSERT_EQUAL(0, alive);
		}
		void testErase(void) {
			auto_multimap<int, RCounter> v;
			v.insert_value(1, new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(1, alive);
			v.insert_value(2, new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(2, alive);
			v.insert_value(2, new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(3, alive);
			v.erase(v.begin());
			CPPUNIT_ASSERT_EQUAL(2, alive);
			CPPUNIT_ASSERT_EQUAL(2, (int)v.size());
			v.erase(v.begin());
			CPPUNIT_ASSERT_EQUAL(1, alive);
			CPPUNIT_ASSERT_EQUAL(1, (int)v.size());
			v.erase(v.begin());
			CPPUNIT_ASSERT_EQUAL(0, (int)v.size());
			CPPUNIT_ASSERT_EQUAL(0, alive);
		}
		void testScope(void) {
			{
				auto_multimap<int, RCounter> v;
				v.insert_value(1, new RCounter(&alive));
				CPPUNIT_ASSERT_EQUAL(1, alive);
				v.insert_value(2, new RCounter(&alive));
				CPPUNIT_ASSERT_EQUAL(2, alive);
			}
			CPPUNIT_ASSERT_EQUAL(0, alive);
		}
};

CPPUNIT_TEST_SUITE_REGISTRATION( auto_multimap_Test );

