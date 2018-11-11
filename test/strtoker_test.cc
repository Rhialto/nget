#include <cstring>
#include "strtoker.h"

#include <cppunit/extensions/HelperMacros.h>

class strtoker_Test : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(strtoker_Test);
		CPPUNIT_TEST(testClass);
		CPPUNIT_TEST(testFunc);
	CPPUNIT_TEST_SUITE_END();
	protected:
		char buf[40];
	public:
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
};

CPPUNIT_TEST_SUITE_REGISTRATION( strtoker_Test );

