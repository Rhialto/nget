void set_user_error_status_and_do_fatal_user_error(void) {} // ugly. whee.
#include "misc.h"
#include "path.h"
#include "file.h"

#include <cppunit/extensions/HelperMacros.h>

class misc_Test : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(misc_Test);
		CPPUNIT_TEST(testRegex2Wildmat);
		CPPUNIT_TEST(testFileCompareSame);
		CPPUNIT_TEST(testFileCompareDiff);
		CPPUNIT_TEST(testFileCompareNonExistA);
		CPPUNIT_TEST(testFileCompareNonExistB);
		CPPUNIT_TEST(testFileCompareNonExist);
		CPPUNIT_TEST(testFExists);
		CPPUNIT_TEST(testDecodeTextMonth);
		CPPUNIT_TEST(testDecodeTextDate);
		CPPUNIT_TEST(testDecodeTextAge);
		CPPUNIT_TEST(testParseStr);
		CPPUNIT_TEST(testParseStrUnsigned);
	CPPUNIT_TEST_SUITE_END();
	public:
		void testRegex2Wildmat(void) {
			CPPUNIT_ASSERT_EQUAL(string("*"), regex2wildmat(""));
			CPPUNIT_ASSERT_EQUAL(string("*foo*"), regex2wildmat("foo"));
			CPPUNIT_ASSERT_EQUAL(string("*a*"), regex2wildmat("a"));
			CPPUNIT_ASSERT_EQUAL(string("a"), regex2wildmat("^a$"));
			CPPUNIT_ASSERT_EQUAL(string(""), regex2wildmat("^$"));
			CPPUNIT_ASSERT_EQUAL(string("*a*b*"), regex2wildmat("a.*b"));
			CPPUNIT_ASSERT_EQUAL(string("*a?b*"), regex2wildmat("a.b"));
			CPPUNIT_ASSERT_EQUAL(string("*[aA][bB][cC]*"), regex2wildmat("abc",true));
			CPPUNIT_ASSERT_EQUAL(string("*[abc]*"), regex2wildmat("[abc]"));
			CPPUNIT_ASSERT_EQUAL(string("*[aAbBcC]*"), regex2wildmat("[abc]",true));
			CPPUNIT_ASSERT_EQUAL(string("*[]]*"), regex2wildmat("[]]"));
			CPPUNIT_ASSERT_EQUAL(string("*[\\]]*"), regex2wildmat("[\\]]"));
			CPPUNIT_ASSERT_EQUAL(string("*[\\[]*"), regex2wildmat("[\\[]"));
			CPPUNIT_ASSERT_EQUAL(string("*\\[*"), regex2wildmat("\\["));
			CPPUNIT_ASSERT_EQUAL(string("*(*"), regex2wildmat("\\("));
		}
		void testFileCompareSame(void) {
			CPPUNIT_ASSERT(filecompare("TestRunner.cc", "TestRunner.cc"));
		}
		void testFileCompareDiff(void) {
			CPPUNIT_ASSERT(!filecompare("TestRunner.cc", "misc_test.cc"));
		}
		void testFileCompareNonExistA(void) {
			int x=0;
			try {
				filecompare("aoeuidhtns", "misc_test.cc");
			} catch (FileNOENTEx &e){
				x=1;
			}
			CPPUNIT_ASSERT(x==1);
		}
		void testFileCompareNonExistB(void) {
			int x=0;
			try {
				filecompare("misc_test.cc", "aoeuidhtns");
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
			CPPUNIT_ASSERT(decode_textdate("Sun, 7 Jul 2002 15:6:5 GMT",0)==1026054365);
			CPPUNIT_ASSERT(decode_textdate("Sun, 7 Jul 2002 15:6:5 GMT",1)==1026054365);
			CPPUNIT_ASSERT(decode_textdate("Tue, 07 Aug 2002  0:21:00 GMT",0)==1028679660);
			CPPUNIT_ASSERT(decode_textdate("Tue, 07 Aug 2002  0:21:00 GMT",1)==1028679660);
			CPPUNIT_ASSERT(decode_textdate("Tue, 07 Aug 2002 0:21:00 GMT",0)==1028679660);
			CPPUNIT_ASSERT(decode_textdate("Tue, 07 Aug 2002 0:21:00 GMT",1)==1028679660);
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
		void testParseStr(void) {
			int i=-2;
			CPPUNIT_ASSERT(parsestr("3",i,"foo"));
			CPPUNIT_ASSERT_EQUAL(3, i);
			CPPUNIT_ASSERT(parsestr("-4",i,"foo"));
			CPPUNIT_ASSERT_EQUAL(-4, i);
			CPPUNIT_ASSERT(!parsestr("5 baz",i,"foo"));
			CPPUNIT_ASSERT_EQUAL(-4, i);
			
			CPPUNIT_ASSERT(parsestr("6",i,6,9,"foo"));
			CPPUNIT_ASSERT_EQUAL(6, i);
			CPPUNIT_ASSERT(parsestr("9",i,6,9,"foo"));
			CPPUNIT_ASSERT_EQUAL(9, i);
			CPPUNIT_ASSERT(!parsestr("10",i,6,9,"foo"));
			CPPUNIT_ASSERT_EQUAL(9, i);
			CPPUNIT_ASSERT(!parsestr("5",i,6,9,"foo"));
			CPPUNIT_ASSERT_EQUAL(9, i);
		}	
		void testParseStrUnsigned(void) {
			unsigned long ul=20;
			CPPUNIT_ASSERT(parsestr("3",ul,"foo"));
			CPPUNIT_ASSERT_EQUAL(3UL, ul);
			CPPUNIT_ASSERT(!parsestr("-4",ul,"foo"));
			CPPUNIT_ASSERT_EQUAL(3UL, ul);
			CPPUNIT_ASSERT(!parsestr("5 baz",ul,"foo"));
			CPPUNIT_ASSERT_EQUAL(3UL, ul);

			CPPUNIT_ASSERT(parsestr("6",ul,6UL,9UL,"foo"));
			CPPUNIT_ASSERT_EQUAL(6UL, ul);
			CPPUNIT_ASSERT(parsestr("9",ul,6UL,9UL,"foo"));
			CPPUNIT_ASSERT_EQUAL(9UL, ul);
			CPPUNIT_ASSERT(!parsestr("10",ul,6UL,9UL,"foo"));
			CPPUNIT_ASSERT_EQUAL(9UL, ul);
			CPPUNIT_ASSERT(!parsestr("5",ul,6UL,9UL,"foo"));
			CPPUNIT_ASSERT_EQUAL(9UL, ul);
		}
};


CPPUNIT_TEST_SUITE_REGISTRATION( misc_Test );


