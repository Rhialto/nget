#include "rcount.h"

#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>

using namespace CppUnit;

class rcount_Test : public TestCase {
	protected:
		int alive;
		class RCounted : public c_refcounted<RCounted> {
			public:
				int *aliveptr;
				static ptr Create(int *a){return new RCounted(a);}
				RCounted(int *a):aliveptr(a){*aliveptr=1;}
				~RCounted(){*aliveptr=0;}
		};
	public:
		rcount_Test(void):TestCase("rcount_Test"){}
		void setUp(void) {
			alive = 0;
		}
		void testSimple(void) {
			RCounted::ptr rcounted = new RCounted(&alive);
			CPPUNIT_ASSERT(alive);
			rcounted = NULL;
			CPPUNIT_ASSERT(!alive);
		}
		void testTransfer(void) {
			RCounted::ptr rcounted = new RCounted(&alive);
			RCounted::ptr rc1 = rcounted;
			CPPUNIT_ASSERT(alive);
			rcounted = NULL;
			CPPUNIT_ASSERT(alive);
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
			rcounted = NULL;
			CPPUNIT_ASSERT(!alive);
		}
		void testTransient(void) {
			RCounted::Create(&alive);
			CPPUNIT_ASSERT(!alive);
		}
		static Test *suite(void) {
			TestSuite *suite = new TestSuite;
#define ADDTEST(n) suite->addTest(new TestCaller<rcount_Test>(#n, &rcount_Test::n))
			ADDTEST(testSimple);
			ADDTEST(testTransfer);
			ADDTEST(testScope);
			ADDTEST(testSelfAssignment);
			ADDTEST(testTransient);
#undef ADDTEST
			return suite;
		}
};



