#include "rcount.h"

#include <cppunit/extensions/HelperMacros.h>

class rcount_Test : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(rcount_Test);
		CPPUNIT_TEST(testSimple);
		CPPUNIT_TEST(testTransfer);
		CPPUNIT_TEST(testScope);
		CPPUNIT_TEST(testSelfAssignment);
		CPPUNIT_TEST(testTransient);
		CPPUNIT_TEST(testComparison);
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
		void testComparison(void) {
			RCounted::ptr a = new RCounted(&alive);
			RCounted::ptr c;
			RCounted::ptr b = new RCounted(&alive);
			RCounted::ptr a1 = a, b1 = b;
			c = new RCounted(&alive);
#define TEST_COMPARE(OP) \
			CPPUNIT_ASSERT_EQUAL(a OP b, a.gimmethepointer() OP b.gimmethepointer());\
			CPPUNIT_ASSERT_EQUAL(a OP c, a.gimmethepointer() OP c.gimmethepointer());\
			CPPUNIT_ASSERT_EQUAL(b OP c, b.gimmethepointer() OP c.gimmethepointer());
			TEST_COMPARE(<);
			TEST_COMPARE(<=);
			TEST_COMPARE(>);
			TEST_COMPARE(>=);
			TEST_COMPARE(==);
			TEST_COMPARE(!=);
			CPPUNIT_ASSERT(!(b==a));
			CPPUNIT_ASSERT(a==a);
			CPPUNIT_ASSERT(b==b);
			CPPUNIT_ASSERT(a1==a);
			CPPUNIT_ASSERT(b1==b);
			CPPUNIT_ASSERT(a!=b);
			CPPUNIT_ASSERT(b!=a);
			CPPUNIT_ASSERT(!(a!=a));
		}
};

CPPUNIT_TEST_SUITE_REGISTRATION( rcount_Test );


