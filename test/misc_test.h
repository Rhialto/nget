#include "misc.h"

#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>

using namespace CppUnit;

class misc_Test : public TestCase {
	public:
		misc_Test(void):TestCase("misc_Test"){}
		void testFileCompareSame(void) {
			CPPUNIT_ASSERT(filecompare("TestRunner.cc", "TestRunner.cc")>0);
		}
		void testFileCompareDiff(void) {
			CPPUNIT_ASSERT(filecompare("TestRunner.cc", "misc_test.h")<0);
		}
		void testFileCompareNonExistA(void) {
			CPPUNIT_ASSERT(filecompare("aoeuidhtns", "misc_test.h")<0);
		}
		void testFileCompareNonExistB(void) {
			CPPUNIT_ASSERT(filecompare("misc_test.h", "aoeuidhtns")<0);
		}
		void testFileCompareNonExist(void) {
			CPPUNIT_ASSERT(filecompare("asdfghjkl", "aoeuidhtns")<0);
		}
		void testFExists(void) {
			CPPUNIT_ASSERT(fexists("TestRunner.cc"));
			CPPUNIT_ASSERT(!fexists("aoeuidhtns"));
		}
		void testDecodeTextDate(void) {
			init_my_timezone();
			//extern int debug; debug=3;
			CPPUNIT_ASSERT(decode_textdate("Sun, 17 Feb 2002 22:52:46 GMT",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("Sun, 17 Feb 2002 22:52:46 GMT",1)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("Sun, 17 Feb 2002 15:52:46 -0700",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("Sun, 17 Feb 2002 15:52:46 -0700",1)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("Sun, 17 Feb 02 22:52:46 GMT",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("Sun, 17 Feb 02 22:52:46 GMT",1)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 2002 22:52:46 GMT",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 2002 22:52:46 GMT",1)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 02 22:52:46 GMT",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 02 22:52:46 GMT",1)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 02 22:52:46",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("7 Feb 2002 22:52:46 GMT",0)==1013122366);
			CPPUNIT_ASSERT(decode_textdate("7 Feb 2002 22:52:46 GMT",1)==1013122366);
			CPPUNIT_ASSERT(decode_textdate("Thu, 7 Feb 2002 22:52:46 GMT",0)==1013122366);
			CPPUNIT_ASSERT(decode_textdate("Thu, 7 Feb 2002 22:52:46 GMT",1)==1013122366);
			CPPUNIT_ASSERT(decode_textdate("17 February 02 22:52:46 GMT",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 February 02 22:52:46 GMT",1)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 02 22:52 GMT",0)==1013986320);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 02 22:52 GMT",1)==1013986320);
			CPPUNIT_ASSERT(decode_textdate("Sun Feb 17 22:52:46 2002",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("Sun Feb 17 22:52:46 02",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("Sun Feb 7 22:52:46 2002",0)==1013122366);
			CPPUNIT_ASSERT(decode_textdate("Sun Feb  7 22:52:46 2002",0)==1013122366);
			CPPUNIT_ASSERT(decode_textdate("2002-02-17 15:52:46-0700",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("2002-02-17 15:52:46-0700",1)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("2002-02-17T15:52:46-0700",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("2002-02-17T15:52:46-0700",1)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("2002-02-17 15:52-0700",0)==1013986320);
			CPPUNIT_ASSERT(decode_textdate("2002-02-17 15:52-0700",1)==1013986320);
		}
		static Test *suite(void) {
			TestSuite *suite = new TestSuite;
#define ADDTEST(n) suite->addTest(new TestCaller<misc_Test>(#n, &misc_Test::n))
			ADDTEST(testFileCompareSame);
			ADDTEST(testFileCompareDiff);
			ADDTEST(testFileCompareNonExistA);
			ADDTEST(testFileCompareNonExistB);
			ADDTEST(testFileCompareNonExist);
			ADDTEST(testFExists);
			ADDTEST(testDecodeTextDate);
#undef ADDTEST
			return suite;
		}
};




