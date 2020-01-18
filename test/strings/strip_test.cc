//

// This file contains functions that remove a defined part from the string,
// i.e., strip the string.

#include <abel/strings/strip.h>

#include <cassert>
#include <cstdio>
#include <cstring>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/strings/string_view.h>

namespace {

TEST(Strip, ConsumePrefixOneChar) {
  abel::string_view input("abc");
  EXPECT_TRUE(abel:: consume_prefix(&input, "a"));
  EXPECT_EQ(input, "bc");

  EXPECT_FALSE(abel:: consume_prefix(&input, "x"));
  EXPECT_EQ(input, "bc");

  EXPECT_TRUE(abel:: consume_prefix(&input, "b"));
  EXPECT_EQ(input, "c");

  EXPECT_TRUE(abel:: consume_prefix(&input, "c"));
  EXPECT_EQ(input, "");

  EXPECT_FALSE(abel:: consume_prefix(&input, "a"));
  EXPECT_EQ(input, "");
}

TEST(Strip,  consume_prefix) {
  abel::string_view input("abcdef");
  EXPECT_FALSE(abel:: consume_prefix(&input, "abcdefg"));
  EXPECT_EQ(input, "abcdef");

  EXPECT_FALSE(abel:: consume_prefix(&input, "abce"));
  EXPECT_EQ(input, "abcdef");

  EXPECT_TRUE(abel:: consume_prefix(&input, ""));
  EXPECT_EQ(input, "abcdef");

  EXPECT_FALSE(abel:: consume_prefix(&input, "abcdeg"));
  EXPECT_EQ(input, "abcdef");

  EXPECT_TRUE(abel:: consume_prefix(&input, "abcdef"));
  EXPECT_EQ(input, "");

  input = "abcdef";
  EXPECT_TRUE(abel:: consume_prefix(&input, "abcde"));
  EXPECT_EQ(input, "f");
}

TEST(Strip,  consume_suffix) {
  abel::string_view input("abcdef");
  EXPECT_FALSE(abel:: consume_suffix(&input, "abcdefg"));
  EXPECT_EQ(input, "abcdef");

  EXPECT_TRUE(abel:: consume_suffix(&input, ""));
  EXPECT_EQ(input, "abcdef");

  EXPECT_TRUE(abel:: consume_suffix(&input, "def"));
  EXPECT_EQ(input, "abc");

  input = "abcdef";
  EXPECT_FALSE(abel:: consume_suffix(&input, "abcdeg"));
  EXPECT_EQ(input, "abcdef");

  EXPECT_TRUE(abel:: consume_suffix(&input, "f"));
  EXPECT_EQ(input, "abcde");

  EXPECT_TRUE(abel:: consume_suffix(&input, "abcde"));
  EXPECT_EQ(input, "");
}

TEST(Strip,  strip_prefix) {
  const abel::string_view null_str;

  EXPECT_EQ(abel:: strip_prefix("foobar", "foo"), "bar");
  EXPECT_EQ(abel:: strip_prefix("foobar", ""), "foobar");
  EXPECT_EQ(abel:: strip_prefix("foobar", null_str), "foobar");
  EXPECT_EQ(abel:: strip_prefix("foobar", "foobar"), "");
  EXPECT_EQ(abel:: strip_prefix("foobar", "bar"), "foobar");
  EXPECT_EQ(abel:: strip_prefix("foobar", "foobarr"), "foobar");
  EXPECT_EQ(abel:: strip_prefix("", ""), "");
}

TEST(Strip,  strip_suffix) {
  const abel::string_view null_str;

  EXPECT_EQ(abel:: strip_suffix("foobar", "bar"), "foo");
  EXPECT_EQ(abel:: strip_suffix("foobar", ""), "foobar");
  EXPECT_EQ(abel:: strip_suffix("foobar", null_str), "foobar");
  EXPECT_EQ(abel:: strip_suffix("foobar", "foobar"), "");
  EXPECT_EQ(abel:: strip_suffix("foobar", "foo"), "foobar");
  EXPECT_EQ(abel:: strip_suffix("foobar", "ffoobar"), "foobar");
  EXPECT_EQ(abel:: strip_suffix("", ""), "");
}

TEST(Strip, trim_complete) {
  const char* inputs[] = {
      "No extra space",
      "  Leading whitespace",
      "Trailing whitespace  ",
      "  Leading and trailing  ",
      " Whitespace \t  in\v   middle  ",
      "'Eeeeep!  \n Newlines!\n",
      "nospaces",
  };
  const char* outputs[] = {
      "No extra space",
      "Leading whitespace",
      "Trailing whitespace",
      "Leading and trailing",
      "Whitespace in middle",
      "'Eeeeep! Newlines!",
      "nospaces",
  };
  int NUM_TESTS = 7;

  for (int i = 0; i < NUM_TESTS; i++) {
    std::string s(inputs[i]);
    abel::trim_complete(&s);
    EXPECT_STREQ(outputs[i], s.c_str());
  }

  // Test that abel::trim_complete returns immediately for empty
  // strings (It was adding the \0 character to the C++ std::string, which broke
  // tests involving empty())
  std::string zero_string = "";
  assert(zero_string.empty());
  abel::trim_complete(&zero_string);
  EXPECT_EQ(zero_string.size(), 0);
  EXPECT_TRUE(zero_string.empty());
}

TEST(Strip, trim_right) {
  std::string test = "foo  ";
  abel::trim_right(&test);
  EXPECT_EQ(test, "foo");

  test = "   ";
  abel::trim_right(&test);
  EXPECT_EQ(test, "");

  test = "";
  abel::trim_right(&test);
  EXPECT_EQ(test, "");

  test = " abc\t";
  abel::trim_right(&test);
  EXPECT_EQ(test, " abc");
}

TEST(String, trim_left) {
  abel::string_view orig = "\t  \n\f\r\n\vfoo";
  EXPECT_EQ("foo", abel::trim_left(orig));
  orig = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
  EXPECT_EQ(abel::string_view(), abel::trim_left(orig));
}

TEST(Strip, trim_all) {
  std::string test2 = "\t  \f\r\n\vfoo \t\f\r\v\n";
  abel::trim_all(&test2);
  EXPECT_EQ(test2, "foo");
  std::string test3 = "bar";
  abel::trim_all(&test3);
  EXPECT_EQ(test3, "bar");
  std::string test4 = "\t  \f\r\n\vfoo";
  abel::trim_all(&test4);
  EXPECT_EQ(test4, "foo");
  std::string test5 = "foo \t\f\r\v\n";
  abel::trim_all(&test5);
  EXPECT_EQ(test5, "foo");
  abel::string_view test6("\t  \f\r\n\vfoo \t\f\r\v\n");
  test6 = abel::trim_all(test6);
  EXPECT_EQ(test6, "foo");
  test6 = abel::trim_all(test6);
  EXPECT_EQ(test6, "foo");  // already stripped
}

}  // namespace
