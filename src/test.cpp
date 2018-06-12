#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>

#include "Sheet.h"

#include "exception/InvalidArgumentException.h"
#include "exception/InvalidInputException.h"

using namespace std;

class __Test
{
public:
    static void test_address()
    {
        /* invalid input */
        assert(Utils::throws<InvalidArgumentException>([]() { Address(0, 0); }));
        assert(Utils::throws<InvalidArgumentException>([]() { Address(0, 5); }));
        assert(Utils::throws<InvalidArgumentException>([]() { Address(-2, 5); }));
        assert(Utils::throws<InvalidArgumentException>([]() { Address(5, 0); }));
        assert(Utils::throws<InvalidArgumentException>([]() { Address(5, -2); }));
        assert(Utils::throws<InvalidArgumentException>([]() { Address(-3, -3); }));

        /* invalid input as string */
        assert(Utils::throws<InvalidArgumentException>([]() { Address("D0"); }));
        assert(Utils::throws<InvalidArgumentException>([]() { Address("D-2"); }));
        assert(Utils::throws<InvalidArgumentException>([]() { Address(""); }));
        assert(Utils::throws<InvalidArgumentException>([]() { Address("1"); }));
        assert(Utils::throws<InvalidArgumentException>([]() { Address("11"); }));
        assert(Utils::throws<InvalidArgumentException>([]() { Address("_11"); }));

        /* valid input */
        assert(Address("A1").col() == 1 && Address("A1").row() == 1);
        assert(Address("D3").col() == 4 && Address("D3").row() == 3);
        assert(Address("A987654").col() == 1 && Address("A987654").row() == 987654);
        assert(Address("Z1").col() == 26);
        assert(Address("AA1").col() == 27);
        assert(Address("AB1").col() == 28);
        assert(Address("AY1").col() == 51);
        assert(Address("AZ1").col() == 52);
        assert(Address("BA1").col() == 53);
        assert(Address("ZY1").col() == 701);
        assert(Address("ZZ1").col() == 702);
        assert(Address("AAA1").col() == 703);
        assert(Address("AAB1").col() == 704);
        assert(Address("AAZ1").col() == 728);
        assert(Address("ABA1").col() == 729);
        assert(Address("ABB1").col() == 730);

        /* case insensitivity */
        assert(Address("ABCDEF123") == Address("aBcDeF123"));

        /* type casting from string */
        assert(Address("ABCD1234") == (Address) "ABCD1234");

        /* generating string representation */
        assert((string) Address(701, 654) == "ZY654");

        /* difference */
        assert(Address("D5") - Address("B3") == Address("C3"));

        /* serialization */
        stringstream ss;
        Address("ABCD1234").serialize(ss);
        assert(ss.str() == "\"ABCD1234\"");

        /* deserialization */
        stringstream ss_;
        ss_ << "\"ABCD1234\"" << flush;
        assert(Address::deserialize(ss_) == Address("ABCD1234"));
    }

    static void test_utils()
    {
        istringstream iss(
            "this is a \\\"large\\\" string with \\\\ backslash\"and here is past its end");
        assert(Utils::readString(iss) == "this is a \\\"large\\\" string with \\\\ backslash");

        assert(
            Utils::escapeString("=\"foo\"+\" and \\\"bar\\\"\"")
                == "=\\\"foo\\\"+\\\" and \\\\\\\"bar\\\\\\\"\\\"");

        assert(
            Utils::unescapeString(
                "this is a \\\"large\\\" string with \\\\ backslash")
                == "this is a \"large\" string with \\ backslash");

        assert(Utils::throws<InvalidInputException>([]() {
            istringstream iss_("foo\\\"bar");
            Utils::readString(iss_);
        }));
    }

    static void test_formula()
    {
        vector<Address> deps;

        /* literal */
        auto l0 = Formula::Parser::parseSource<int>("123", deps);
        assert(
            dynamic_cast<const Formula::Literal<int> *>(l0.get()) != nullptr &&
                deps.size() == 0
        );

        auto l1 = Formula::Parser::parseSource<double>("123.456", deps);
        assert(
            dynamic_cast<const Formula::Literal<double> *>(l1.get()) != nullptr &&
                deps.size() == 0
        );

        auto l2 = Formula::Parser::parseSource<string>("\"some \\\"string\\\"\"", deps);
        assert(
            dynamic_cast<const Formula::Literal<string> *>(l2.get()) != nullptr &&
                deps.size() == 0 &&
                l2->evaluate(Sheet()) == "some \"string\""
        );

        assert(Utils::throws<IncorrectFormulaSyntaxException>([]() {
            vector<Address> deps_;
            Formula::Parser::parseSource<int>("nonsense", deps_);
        }));

        /* link */
        auto link0 = Formula::Parser::parseSource<int>("ABC123", deps);
        assert(
            dynamic_cast<const Formula::Link<int> *>(link0.get()) != nullptr &&
                deps.size() == 1 && deps[0] == Address("ABC123")
        );
        deps.clear();

        /* operations and functions */
        auto o0 = Formula::Parser::parseSource<double>(
            "1+2-4*6/3*(1)*(6-2)*abs(abs(1-2)-abs(3-5))+0.1-abs(0-.2)+0.1*57/57", deps);
        assert(o0->evaluate(Sheet()) == -8 && deps.size() == 0);

        auto o1 = Formula::Parser::parseSource<string>("\"Hello\"+\" \"+\"World!\"", deps);
        assert(o1->evaluate(Sheet()) == "Hello World!" && deps.size() == 0);

        assert(Utils::throws<IncorrectFormulaSyntaxException>([]() {
            vector<Address> deps_;
            Formula::Parser::parseSource<int>("1+2+(3+4+(5+6)", deps_);
        }));

        /* parentheses */
        auto p0 = Formula::Parser::parseSource<int>("((1+(2)))+(3+(4+(5+((6)))))", deps);
        assert(p0->evaluate(Sheet()) == 21);

        /* parsing back to source */
        auto ts0 = Formula::Parser::parseSource<int>("AbS(5)+sIn(cos(6))", deps);
        assert(ts0->toSource() == "ABS(5)+SIN(COS(6))");

        /* abs */
        auto f0 = Formula::Parser::parseSource<int>("abs(7-9)", deps);
        assert(f0->evaluate(Sheet()) == 2);
        auto f1 = Formula::Parser::parseSource<double>("abs(7.5-9)", deps);
        assert(f1->evaluate(Sheet()) == 1.5);

        /* sin */
        Formula::Parser::parseSource<double>("sin(1.234)", deps);

        /* cos */
        Formula::Parser::parseSource<double>("cos(1.234)", deps);

        /* tan */
        Formula::Parser::parseSource<double>("tan(1.234)", deps);
    }

    static void test_sheet()
    {
        Sheet s0;
        const CellBase *c = nullptr;

        s0.attachCellContentChangedEvent([&](const CellBase &cell) {
            c = &cell;
        });

        /* empty cell */
        assert(dynamic_cast<const Cell<string> *>(s0.getCell("A1").get()) != nullptr);
        assert(dynamic_cast<const Cell<string> *>(s0.getCell("A1").get())->getContent() == "");
        assert(s0.getCell("A1").get()->getContentText() == "");
        assert(s0.getCell("A1").get()->getContentSource() == "");
        assert(s0.getCell("A1").get()->getAddr() == "A1");
        assert(s0.getCell("A1").get()->getDependencies().size() == 0);
        assert(s0.m_Cells.size() == 0);
        assert(s0.m_Dependencies.size() == 0);
        assert(c == nullptr);

        /* text cell */
        s0.setCellContent("A1", "foo");
        assert(dynamic_cast<const Cell<string> *>(s0.getCell("A1").get()) != nullptr);
        assert(dynamic_cast<const Cell<string> *>(c) != nullptr);
        assert(s0.getCell("A1")->getAddr() == c->getAddr());
        assert(s0.getCell("A1")->getDependencies() == c->getDependencies());
        assert(s0.getCell("A1")->getContentText() == c->getContentText());
        assert(s0.getCell("A1")->getContentSource() == c->getContentSource());
        assert(s0.getCell("A1")->getContentText() == "foo");
        assert(s0.getCell("A1")->getContentSource() == "foo");
        assert(s0.getCell("A1")->getAddr() == "A1");
        assert(s0.getCell("A1")->getDependencies().size() == 0);
        assert(s0.m_Cells.size() == 1);
        assert(s0.m_Dependencies.size() == 0);

        /* sheet should remove empty string cells */
        s0.setCellContent("A1", "");
        assert(s0.m_Cells.size() == 0);

        /* text cell with formula */
        s0.setCellContent("A2", "=\"foo\"+\"bar\"+\"foo\"");
        assert(c->getAddr() == "A2");
        assert(s0.getCell("A2")->getContentText() == "foobarfoo");
        assert(s0.getCell("A2")->getContentSource() == "=\"foo\"+\"bar\"+\"foo\"");
        assert(s0.getCell("A2")->getDependencies().size() == 0);
        assert(s0.m_Cells.size() == 1);
        assert(s0.m_Dependencies.size() == 0);

        /* int cell with formula */
        s0.setCellType<int>("A3");
        assert(s0.getCell("A3")->getContentText() == "0");
        assert(s0.getCell("A3")->getContentSource() == "0");
        s0.setCellContent("A3", "5");
        assert(s0.getCell("A3")->getContentText() == "5");
        assert(s0.getCell("A3")->getContentSource() == "5");

        /* double cell with formula */
        s0.setCellType<double>("A4");
        assert(s0.getCell("A4")->getContentText() == "0.000000");
        assert(s0.getCell("A4")->getContentSource() == "0.000000");
        s0.setCellContent("A4", "5.75");
        assert(s0.getCell("A4")->getContentText() == "5.750000");
        assert(s0.getCell("A4")->getContentSource() == "5.750000");

        /* cell type cast */
        s0.setCellContent("A5", "123");
        s0.setCellType<int>("A5");
        assert(s0.getCell("A5")->getContentText() == "123");
        s0.setCellType<string>("A5");
        assert(s0.getCell("A5")->getContentText() == "123");
        s0.setCellType<double>("A5");
        assert(s0.getCell("A5")->getContentText() == "123.000000");

        /* link */
        s0.setCellContent("B1", "Hello");
        s0.setCellContent("B2", "World");
        s0.setCellContent("B3", "=B1+\" \"+B2+\"!\"");
        assert(s0.getCell("B3")->getContentText() == "Hello World!");
        assert(c->getContentText() == "Hello World!");

        s0.setCellType<int>("C1");
        s0.setCellContent("C1", "1");
        s0.setCellType<int>("C2");
        s0.setCellContent("C2", "=C1+1");
        s0.setCellType<int>("C3");
        s0.setCellContent("C3", "=C2+1");
        s0.setCellType<int>("C4");
        s0.setCellContent("C4", "=C3+1");
        assert(s0.getCell("C4")->getContentText() == "4");
        assert(c->getContentText() == "4");
        s0.setCellContent("C1", "2");
        assert(s0.getCell("C4")->getContentText() == "5");
        assert(c->getContentText() == "5");

        /* non-existent link target */
        s0.setCellContent("D1", "=D2");
        assert(s0.getCell("D1")->getContentText() == "");

        /* serialization */
        ostringstream oss;
        Sheet s1;

        s1.serialize(oss);
        assert(oss.str() == "[]");
        oss.str("");
        oss.clear();

        s1.setCellType<int>("A1");
        s1.serialize(oss);
        assert(oss.str() == "[{\"type\":\"int\",\"addr\":\"A1\",\"content\":\"0\"}]");
        oss.str("");
        oss.clear();

        s1.setCellType<string>("A1");
        s1.setCellContent("A1", "");
        s1.setCellContent("A2", "=\"foo\"+\" and \\\"bar\\\"\"");
        s1.setCellContent("A3", "654");
        s1.setCellType<double>("A3");
        s1.serialize(oss);
        string a2 = "{\"type\":\"string\",\"addr\":\"A2\",\"content\":\"=\\\"foo\\\"+\\\" and \\\\\\\"bar\\\\\\\"\\\"\"}";
        string a3 = "{\"type\":\"double\",\"addr\":\"A3\",\"content\":\"654.000000\"}";
        assert(oss.str() == string("[") + a2 + "," + a3 + "]" ||
            oss.str() == string("[") + a3 + "," + a2 + "]");

        /* cell should be removed if empty string */
        Sheet s2;
        s2.setCellContent("A1", "asd");
        assert(s2.m_Cells.size() == 1);
        s2.setCellContent("A1", "");
        assert(s2.m_Cells.size() == 0);

        /* deserialization */
        istringstream iss;
        iss.str(
            "[{\"type\":\"string\",\"addr\":\"A1\",\"content\":\"some \\\"escaped\\\" string with \\\\ backslash\"},{\"type\":\"int\",\"addr\":\"A2\",\"content\":\"5\"},{\"type\":\"double\",\"addr\":\"A3\",\"content\":\"=5.75+0.25\"},{\"type\":\"string\",\"addr\":\"A4\",\"content\":\"=\\\"foo\\\"+\\\" and \\\\\\\"bar\\\\\\\"\\\"\"}]");
        Sheet s3 = Sheet::deserialize(iss);
        assert(s3.m_Cells.size() == 4);
        assert(s3.getCell("A1")->getContentText() == "some \"escaped\" string with \\ backslash");
        assert(s3.getCell("A2")->getContentText() == "5");
        assert(s3.getCell("A3")->getContentText() == "6.000000");
        assert(s3.getCell("A4")->getContentText() == "foo and \"bar\"");
    }
};

int main()
{
    cout << "Testing Address... ";
    __Test::test_address();
    cout << "Passed" << endl;

    cout << "Testing Utils... ";
    __Test::test_utils();
    cout << "Passed" << endl;

    cout << "Testing Formula... ";
    __Test::test_formula();
    cout << "Passed" << endl;

    cout << "Testing Sheet... ";
    __Test::test_sheet();
    cout << "Passed" << endl;

    return 0;
}
