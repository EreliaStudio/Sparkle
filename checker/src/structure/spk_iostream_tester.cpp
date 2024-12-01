#include "structure/spk_iostream_tester.hpp"

TEST_F(IOStreamTest, BasicOutput)
{
    ioStream << L"Hello, World!";
    ASSERT_EQ(ioStream.str(), L"Hello, World!");

    ioStream.flush();
    ASSERT_EQ(testStream1.str(), L"Hello, World!");
}

TEST_F(IOStreamTest, PrefixOutput)
{
    ioStream.setPrefix(L"Prefix");
    ioStream << L"Hello, World!";

    ioStream.flush();
    ASSERT_EQ(testStream1.str(), L"[Prefix] - Hello, World!");
}

TEST_F(IOStreamTest, NewLineHandling)
{
    ioStream.setPrefix(L"Prefix");
    ioStream << L"Hello," << std::endl << L"World!";

    ioStream.flush();
    ASSERT_EQ(testStream1.str(), L"[Prefix] - Hello,\n[Prefix] - World!");
}

TEST_F(IOStreamTest, MultipleLines)
{
    ioStream.setPrefix(L"Prefix");
    ioStream << L"Line 1" << std::endl << L"Line 2" << std::endl << L"Line 3";

    ioStream.flush();
    ASSERT_EQ(testStream1.str(), L"[Prefix] - Line 1\n[Prefix] - Line 2\n[Prefix] - Line 3");
}

TEST_F(IOStreamTest, IntegerOutput)
{
    ioStream << 42;

    ioStream.flush();
    ASSERT_EQ(testStream1.str(), L"42");
}

TEST_F(IOStreamTest, MixedTypeOutput)
{
    ioStream << L"Number: " << 42 << L", Text: " << L"Hello";

    ioStream.flush();
    ASSERT_EQ(testStream1.str(), L"Number: 42, Text: Hello");
}

TEST_F(IOStreamTest, OutputWithManipulators)
{
    ioStream << L"Line 1" << std::endl << L"Line 2";

    ioStream.flush();
    ASSERT_EQ(testStream1.str(), L"Line 1\nLine 2");
}

TEST_F(IOStreamTest, StreamResetAfterNewLine)
{
    ioStream.setPrefix(L"Prefix");
    ioStream << L"Hello, World!" << std::endl;

    ioStream.flush();
    ASSERT_EQ(testStream1.str(), L"[Prefix] - Hello, World!\n");

    ioStream << L"New Line";

    ioStream.flush();
    ASSERT_EQ(testStream1.str(), L"[Prefix] - Hello, World!\n[Prefix] - New Line");
}