#include "misc.h"
#include "file.h"

#include <cppunit/TestCaller.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>

using namespace CppUnit;

class misc_Test : public TestCase {
	public:
		misc_Test(void):TestCase("misc_Test"){}
		void testFileCompareSame(void) {
			CPPUNIT_ASSERT(filecompare("TestRunner.cc", "TestRunner.cc"));
		}
		void testFileCompareDiff(void) {
			CPPUNIT_ASSERT(!filecompare("TestRunner.cc", "misc_test.h"));
		}
		void testFileCompareNonExistA(void) {
			int x=0;
			try {
				filecompare("aoeuidhtns", "misc_test.h");
			} catch (FileNOENTEx &e){
				x=1;
			}
			CPPUNIT_ASSERT(x==1);
		}
		void testFileCompareNonExistB(void) {
			int x=0;
			try {
				filecompare("misc_test.h", "aoeuidhtns");
			} catch (FileNOENTEx &e){
				x=1;
			}
			CPPUNIT_ASSERT(x==1);
		}
		void testFileCompareNonExist(void) {
			int x=0;
			try {
				filecompare("asdfghjkl", "aoeuidhtns");
			} catch (FileNOENTEx &e){
				x=1;
			}
			CPPUNIT_ASSERT(x==1);
		}
		void testFExists(void) {
			CPPUNIT_ASSERT(fexists("TestRunner.cc"));
			CPPUNIT_ASSERT(!fexists("aoeuidhtns"));
		}
		void testDecodeTextMonth(void) {
			CPPUNIT_ASSERT(decode_textmonth("jan")==0);
			CPPUNIT_ASSERT(decode_textmonth("january")==0);
			CPPUNIT_ASSERT(decode_textmonth("feb")==1);
			CPPUNIT_ASSERT(decode_textmonth("february")==1);
			CPPUNIT_ASSERT(decode_textmonth("mar")==2);
			CPPUNIT_ASSERT(decode_textmonth("march")==2);
			CPPUNIT_ASSERT(decode_textmonth("apr")==3);
			CPPUNIT_ASSERT(decode_textmonth("april")==3);
			CPPUNIT_ASSERT(decode_textmonth("may")==4);
			CPPUNIT_ASSERT(decode_textmonth("jun")==5);
			CPPUNIT_ASSERT(decode_textmonth("june")==5);
			CPPUNIT_ASSERT(decode_textmonth("jul")==6);
			CPPUNIT_ASSERT(decode_textmonth("july")==6);
			CPPUNIT_ASSERT(decode_textmonth("aug")==7);
			CPPUNIT_ASSERT(decode_textmonth("august")==7);
			CPPUNIT_ASSERT(decode_textmonth("sep")==8);
			CPPUNIT_ASSERT(decode_textmonth("september")==8);
			CPPUNIT_ASSERT(decode_textmonth("oct")==9);
			CPPUNIT_ASSERT(decode_textmonth("october")==9);
			CPPUNIT_ASSERT(decode_textmonth("nov")==10);
			CPPUNIT_ASSERT(decode_textmonth("november")==10);
			CPPUNIT_ASSERT(decode_textmonth("dec")==11);
			CPPUNIT_ASSERT(decode_textmonth("december")==11);
			CPPUNIT_ASSERT(decode_textmonth("foo")==-1);
		}
		void testDecodeTextDate(void) {
			//extern int debug; debug=3;
			CPPUNIT_ASSERT(decode_textdate("Sun, 17 Feb 2002 22:52:46 GMT",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("Sun, 17 Feb 2002 22:52:46 GMT",1)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("Sun, 17 Feb 2002 15:52:46 -0700",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("Sun, 17 Feb 2002 15:52:46 -0700",1)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("Sun, 17 Feb 2002 15:52 -0700",0)==1013986320);
			CPPUNIT_ASSERT(decode_textdate("Sun, 17 Feb 2002 15:52 -0700",1)==1013986320);
			CPPUNIT_ASSERT(decode_textdate("Sun, 17 Feb 02 22:52:46 GMT",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("Sun, 17 Feb 02 22:52:46 GMT",1)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 2002 22:52:46 GMT",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 2002 22:52:46 GMT",1)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 2002 15:52:46 -0700",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 2002 15:52:46 -0700",1)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 02 22:52:46 GMT",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 02 22:52:46 GMT",1)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 02 15:52:46 -0700",0)==1013986366);
			CPPUNIT_ASSERT(decode_textdate("17 Feb 02 15:52:46 -0700",1)==1013986366);
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
			time_t now = time(NULL);
			CPPUNIT_ASSERT_EQUAL(now, decode_textdate(asctime(localtime(&now)),1));
			CPPUNIT_ASSERT_EQUAL(now, decode_textdate(asctime(gmtime(&now)),0));
		}
		void testDecodeTextAge(void) {
			CPPUNIT_ASSERT_EQUAL(time(NULL), decode_textage("0s"));
			CPPUNIT_ASSERT_EQUAL(time(NULL)-1, decode_textage("1s"));
			CPPUNIT_ASSERT_EQUAL(time(NULL)-60, decode_textage("1m"));
			CPPUNIT_ASSERT_EQUAL(time(NULL)-3600, decode_textage("1h"));
			CPPUNIT_ASSERT_EQUAL(time(NULL)-86400, decode_textage("1d"));
			CPPUNIT_ASSERT_EQUAL(time(NULL)-604800, decode_textage("1w"));
			CPPUNIT_ASSERT_EQUAL(time(NULL)-(604800+86400*2+3600*3+60*4+5), decode_textage("1 week 2 days 3 hours 4 mins 5 second"));
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
			ADDTEST(testDecodeTextMonth);
			ADDTEST(testDecodeTextDate);
			ADDTEST(testDecodeTextAge);
#undef ADDTEST
			return suite;
		}
};




