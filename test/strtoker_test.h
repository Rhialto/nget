#include "strtoker.h"

#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>

using namespace CppUnit;

class strtoker_Test : public TestCase {
	protected:
		char buf[40];
	public:
		strtoker_Test(void):TestCase("strtoker_Test"){}
		void setUp(void) {
			strcpy(buf, "a1\tb2\t\tc4");
		}
		void testClass(void) {
			strtoker toker(5,'\t');
			CPPUNIT_ASSERT(toker.tok(buf) == 0);
			CPPUNIT_ASSERT(strcmp(toker[0],"a1") == 0);
			CPPUNIT_ASSERT(strcmp(toker[1],"b2") == 0);
			CPPUNIT_ASSERT(strcmp(toker[2],"") == 0);
			CPPUNIT_ASSERT(strcmp(toker[3],"c4") == 0);
			CPPUNIT_ASSERT(toker[4] == NULL);
		}
		void testFunc(void) {
			char *cur=buf;
			CPPUNIT_ASSERT(strcmp(goodstrtok(&cur,'\t'), "a1") == 0);
			CPPUNIT_ASSERT(strcmp(goodstrtok(&cur,'\t'), "b2") == 0);
			CPPUNIT_ASSERT(strcmp(goodstrtok(&cur,'\t'), "") == 0);
			CPPUNIT_ASSERT(strcmp(goodstrtok(&cur,'\t'), "c4") == 0);
			CPPUNIT_ASSERT(goodstrtok(&cur,'\t') == NULL);
		}
		static Test *suite(void) {
			TestSuite *suite = new TestSuite;
#define ADDTEST(n) suite->addTest(new TestCaller<strtoker_Test>(#n, &strtoker_Test::n))
			ADDTEST(testClass);
			ADDTEST(testFunc);
#undef ADDTEST
			return suite;
		}
};


