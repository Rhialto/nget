#include "rcount.h"

#include <cppunit/extensions/HelperMacros.h>

class rcount_Test : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(rcount_Test);
		CPPUNIT_TEST(testSimple);
		CPPUNIT_TEST(testTransfer);
		CPPUNIT_TEST(testScope);
		CPPUNIT_TEST(testSelfAssignment);
		CPPUNIT_TEST(testTransient);
	CPPUNIT_TEST_SUITE_END();
	protected:
		int alive;
		class RCounted : public c_refcounted<RCounted> {
			public:
				int *aliveptr;
				bool foo(void){return true;}
				static ptr Create(int *a){return new RCounted(a);}
				RCounted(int *a):aliveptr(a){*aliveptr=1;}
				~RCounted(){*aliveptr=0;}
		};
	public:
		void setUp(void) {
			alive = 0;
		}
		void testSimple(void) {
			RCounted::ptr rcounted = new RCounted(&alive);
			CPPUNIT_ASSERT(alive);
			CPPUNIT_ASSERT(rcounted->foo());
			rcounted = NULL;
			CPPUNIT_ASSERT(!alive);
		}
		void testTransfer(void) {
			RCounted::ptr rcounted = new RCounted(&alive);
			RCounted::ptr rc1 = rcounted;
			CPPUNIT_ASSERT(alive);
			rcounted = NULL;
			CPPUNIT_ASSERT(alive);
			CPPUNIT_ASSERT(rc1->foo());
			rc1 = NULL;
			CPPUNIT_ASSERT(!alive);
		}
		void testScope(void) {
			{
				RCounted::ptr rcounted = new RCounted(&alive);
				CPPUNIT_ASSERT(alive);
			}
			CPPUNIT_ASSERT(!alive);
		}
		void testSelfAssignment(void) {
			RCounted::ptr rcounted = new RCounted(&alive);
			CPPUNIT_ASSERT(alive);
			rcounted = rcounted;
			CPPUNIT_ASSERT(alive);
			CPPUNIT_ASSERT(rcounted->foo());
			rcounted = NULL;
			CPPUNIT_ASSERT(!alive);
		}
		void testTransient(void) {
			CPPUNIT_ASSERT(RCounted::Create(&alive)->foo());
			CPPUNIT_ASSERT(!alive);
		}
};

CPPUNIT_TEST_SUITE_REGISTRATION( rcount_Test );


class auto_vector_Test : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(auto_vector_Test);
		CPPUNIT_TEST(testClear);
		CPPUNIT_TEST(testScope);
	CPPUNIT_TEST_SUITE_END();
	protected:
		int alive;
		class RCounter{
			public:
				int *aliveptr;
				bool foo(void){return true;}
				RCounter(int *a):aliveptr(a){++*aliveptr;}
				~RCounter(){--*aliveptr;}
		};
	public:
		void setUp(void) {
			alive = 0;
		}
		void testClear(void) {
			auto_vector<RCounter> v;
			v.push_back(new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(1, alive);
			v.push_back(new RCounter(&alive));
			CPPUNIT_ASSERT_EQUAL(2, alive);
			v.clear();
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

