#include <abel/format/internal/parser.h>
#include <string.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/base/profile.h>

namespace abel {

namespace format_internal {

namespace {

using testing::Pair;

TEST(LengthModTest, Names) {
    struct Expectation {
        int line;
        length_mod::Id id;
        const char *name;
    };
    const Expectation kExpect[] = {
        {__LINE__, length_mod::none, ""},
        {__LINE__, length_mod::h, "h"},
        {__LINE__, length_mod::hh, "hh"},
        {__LINE__, length_mod::l, "l"},
        {__LINE__, length_mod::ll, "ll"},
        {__LINE__, length_mod::L, "L"},
        {__LINE__, length_mod::j, "j"},
        {__LINE__, length_mod::z, "z"},
        {__LINE__, length_mod::t, "t"},
        {__LINE__, length_mod::q, "q"},
    };
    EXPECT_EQ(ABEL_ARRAYSIZE(kExpect), length_mod::kNumValues);
    for (auto e : kExpect) {
        SCOPED_TRACE(e.line);
        length_mod mod = length_mod::FromId(e.id);
        EXPECT_EQ(e.id, mod.id());
        EXPECT_EQ(e.name, mod.name());
    }
}

TEST(ConversionCharTest, Names) {
    struct Expectation {
        conversion_char::Id id;
        char name;
    };
    // clang-format off
    const Expectation kExpect[] = {
#define X(c) {conversion_char::c, #c[0]}
        X(c), X(C), X(s), X(S),                          // text
        X(d), X(i), X(o), X(u), X(x), X(X),              // int
        X(f), X(F), X(e), X(E), X(g), X(G), X(a), X(A),  // float
        X(n), X(p),                                      // misc
#undef X
        {conversion_char::none, '\0'},
    };
    // clang-format on
    EXPECT_EQ(ABEL_ARRAYSIZE(kExpect), conversion_char::kNumValues);
    for (auto e : kExpect) {
        SCOPED_TRACE(e.name);
        conversion_char v = conversion_char::FromId(e.id);
        EXPECT_EQ(e.id, v.id());
        EXPECT_EQ(e.name, v.Char());
    }
}

class ConsumeUnboundConversionTest : public ::testing::Test {
public:
    std::pair<string_view, string_view> Consume (string_view src) {
        int next = 0;
        o = UnboundConversion();  // refresh
        const char *p = ConsumeUnboundConversion(
            src.data(), src.data() + src.size(), &o, &next);
        if (!p)
            return {{}, src};
        return {string_view(src.data(), p - src.data()),
                string_view(p, src.data() + src.size() - p)};
    }

    bool Run (const char *fmt, bool force_positional = false) {
        int next = force_positional ? -1 : 0;
        o = UnboundConversion();  // refresh
        return ConsumeUnboundConversion(fmt, fmt + strlen(fmt), &o, &next) ==
            fmt + strlen(fmt);
    }
    UnboundConversion o;
};

TEST_F(ConsumeUnboundConversionTest, ConsumeSpecification) {
    struct Expectation {
        int line;
        string_view src;
        string_view out;
        string_view src_post;
    };
    const Expectation kExpect[] = {
        {__LINE__, "", "", ""},
        {__LINE__, "b", "", "b"},  // 'b' is invalid
        {__LINE__, "ba", "", "ba"},  // 'b' is invalid
        {__LINE__, "l", "", "l"},  // just length mod isn't okay
        {__LINE__, "d", "d", ""},  // basic
        {__LINE__, "d ", "d", " "},  // leave suffix
        {__LINE__, "dd", "d", "d"},  // don't be greedy
        {__LINE__, "d9", "d", "9"},  // leave non-space suffix
        {__LINE__, "dzz", "d", "zz"},  // length mod as suffix
        {__LINE__, "1$*2$d", "1$*2$d", ""},  // arg indexing and * allowed.
        {__LINE__, "0-14.3hhd", "0-14.3hhd", ""},  // precision, width
        {__LINE__, " 0-+#14.3hhd", " 0-+#14.3hhd", ""},  // flags
    };
    for (const auto &e : kExpect) {
        SCOPED_TRACE(e.line);
        EXPECT_THAT(Consume(e.src), Pair(e.out, e.src_post));
    }
}

TEST_F(ConsumeUnboundConversionTest, BasicConversion) {
    EXPECT_FALSE(Run(""));
    EXPECT_FALSE(Run("z"));

    EXPECT_FALSE(Run("dd"));  // no excess allowed

    EXPECT_TRUE(Run("d"));
    EXPECT_EQ('d', o.conv.Char());
    EXPECT_FALSE(o.width.is_from_arg());
    EXPECT_LT(o.width.value(), 0);
    EXPECT_FALSE(o.precision.is_from_arg());
    EXPECT_LT(o.precision.value(), 0);
    EXPECT_EQ(1, o.arg_position);
    EXPECT_EQ(length_mod::none, o.length_mod.id());
}

TEST_F(ConsumeUnboundConversionTest, ArgPosition) {
    EXPECT_TRUE(Run("d"));
    EXPECT_EQ(1, o.arg_position);
    EXPECT_TRUE(Run("3$d"));
    EXPECT_EQ(3, o.arg_position);
    EXPECT_TRUE(Run("1$d"));
    EXPECT_EQ(1, o.arg_position);
    EXPECT_TRUE(Run("1$d", true));
    EXPECT_EQ(1, o.arg_position);
    EXPECT_TRUE(Run("123$d"));
    EXPECT_EQ(123, o.arg_position);
    EXPECT_TRUE(Run("123$d", true));
    EXPECT_EQ(123, o.arg_position);
    EXPECT_TRUE(Run("10$d"));
    EXPECT_EQ(10, o.arg_position);
    EXPECT_TRUE(Run("10$d", true));
    EXPECT_EQ(10, o.arg_position);

    // Position can't be zero.
    EXPECT_FALSE(Run("0$d"));
    EXPECT_FALSE(Run("0$d", true));
    EXPECT_FALSE(Run("1$*0$d"));
    EXPECT_FALSE(Run("1$.*0$d"));

    // Position can't start with a zero digit at all. That is not a 'decimal'.
    EXPECT_FALSE(Run("01$p"));
    EXPECT_FALSE(Run("01$p", true));
    EXPECT_FALSE(Run("1$*01$p"));
    EXPECT_FALSE(Run("1$.*01$p"));
}

TEST_F(ConsumeUnboundConversionTest, WidthAndPrecision) {
    EXPECT_TRUE(Run("14d"));
    EXPECT_EQ('d', o.conv.Char());
    EXPECT_FALSE(o.width.is_from_arg());
    EXPECT_EQ(14, o.width.value());
    EXPECT_FALSE(o.precision.is_from_arg());
    EXPECT_LT(o.precision.value(), 0);

    EXPECT_TRUE(Run("14.d"));
    EXPECT_FALSE(o.width.is_from_arg());
    EXPECT_FALSE(o.precision.is_from_arg());
    EXPECT_EQ(14, o.width.value());
    EXPECT_EQ(0, o.precision.value());

    EXPECT_TRUE(Run(".d"));
    EXPECT_FALSE(o.width.is_from_arg());
    EXPECT_LT(o.width.value(), 0);
    EXPECT_FALSE(o.precision.is_from_arg());
    EXPECT_EQ(0, o.precision.value());

    EXPECT_TRUE(Run(".5d"));
    EXPECT_FALSE(o.width.is_from_arg());
    EXPECT_LT(o.width.value(), 0);
    EXPECT_FALSE(o.precision.is_from_arg());
    EXPECT_EQ(5, o.precision.value());

    EXPECT_TRUE(Run(".0d"));
    EXPECT_FALSE(o.width.is_from_arg());
    EXPECT_LT(o.width.value(), 0);
    EXPECT_FALSE(o.precision.is_from_arg());
    EXPECT_EQ(0, o.precision.value());

    EXPECT_TRUE(Run("14.5d"));
    EXPECT_FALSE(o.width.is_from_arg());
    EXPECT_FALSE(o.precision.is_from_arg());
    EXPECT_EQ(14, o.width.value());
    EXPECT_EQ(5, o.precision.value());

    EXPECT_TRUE(Run("*.*d"));
    EXPECT_TRUE(o.width.is_from_arg());
    EXPECT_EQ(1, o.width.get_from_arg());
    EXPECT_TRUE(o.precision.is_from_arg());
    EXPECT_EQ(2, o.precision.get_from_arg());
    EXPECT_EQ(3, o.arg_position);

    EXPECT_TRUE(Run("*d"));
    EXPECT_TRUE(o.width.is_from_arg());
    EXPECT_EQ(1, o.width.get_from_arg());
    EXPECT_FALSE(o.precision.is_from_arg());
    EXPECT_LT(o.precision.value(), 0);
    EXPECT_EQ(2, o.arg_position);

    EXPECT_TRUE(Run(".*d"));
    EXPECT_FALSE(o.width.is_from_arg());
    EXPECT_LT(o.width.value(), 0);
    EXPECT_TRUE(o.precision.is_from_arg());
    EXPECT_EQ(1, o.precision.get_from_arg());
    EXPECT_EQ(2, o.arg_position);

    // mixed implicit and explicit: didn't specify arg position.
    EXPECT_FALSE(Run("*23$.*34$d"));

    EXPECT_TRUE(Run("12$*23$.*34$d"));
    EXPECT_EQ(12, o.arg_position);
    EXPECT_TRUE(o.width.is_from_arg());
    EXPECT_EQ(23, o.width.get_from_arg());
    EXPECT_TRUE(o.precision.is_from_arg());
    EXPECT_EQ(34, o.precision.get_from_arg());

    EXPECT_TRUE(Run("2$*5$.*9$d"));
    EXPECT_EQ(2, o.arg_position);
    EXPECT_TRUE(o.width.is_from_arg());
    EXPECT_EQ(5, o.width.get_from_arg());
    EXPECT_TRUE(o.precision.is_from_arg());
    EXPECT_EQ(9, o.precision.get_from_arg());

    EXPECT_FALSE(Run(".*0$d")) << "no arg 0";

    // Large values
    EXPECT_TRUE(Run("999999999.999999999d"));
    EXPECT_FALSE(o.width.is_from_arg());
    EXPECT_EQ(999999999, o.width.value());
    EXPECT_FALSE(o.precision.is_from_arg());
    EXPECT_EQ(999999999, o.precision.value());

    EXPECT_FALSE(Run("1000000000.999999999d"));
    EXPECT_FALSE(Run("999999999.1000000000d"));
    EXPECT_FALSE(Run("9999999999d"));
    EXPECT_FALSE(Run(".9999999999d"));
}

TEST_F(ConsumeUnboundConversionTest, Flags) {
    static const char kAllFlags[] = "-+ #0";
    static const int kNumFlags = ABEL_ARRAYSIZE(kAllFlags) - 1;
    for (int rev = 0; rev < 2; ++rev) {
        for (int i = 0; i < 1 << kNumFlags; ++i) {
            std::string fmt;
            for (int k = 0; k < kNumFlags; ++k)
                if ((i >> k) & 1)
                    fmt += kAllFlags[k];
            // flag order shouldn't matter
            if (rev == 1) {
                std::reverse(fmt.begin(), fmt.end());
            }
            fmt += 'd';
            SCOPED_TRACE(fmt);
            EXPECT_TRUE(Run(fmt.c_str()));
            EXPECT_EQ(fmt.find('-') == std::string::npos, !o.flags.left);
            EXPECT_EQ(fmt.find('+') == std::string::npos, !o.flags.show_pos);
            EXPECT_EQ(fmt.find(' ') == std::string::npos, !o.flags.sign_col);
            EXPECT_EQ(fmt.find('#') == std::string::npos, !o.flags.alt);
            EXPECT_EQ(fmt.find('0') == std::string::npos, !o.flags.zero);
        }
    }
}

TEST_F(ConsumeUnboundConversionTest, BasicFlag) {
    // Flag is on
    for (const char *fmt : {"d", "llx", "G", "1$X"}) {
        SCOPED_TRACE(fmt);
        EXPECT_TRUE(Run(fmt));
        EXPECT_TRUE(o.flags.basic);
    }

    // Flag is off
    for (const char *fmt : {"3d", ".llx", "-G", "1$#X"}) {
        SCOPED_TRACE(fmt);
        EXPECT_TRUE(Run(fmt));
        EXPECT_FALSE(o.flags.basic);
    }
}

struct SummarizeConsumer {
    std::string *out;
    explicit SummarizeConsumer (std::string *out) : out(out) { }

    bool Append (string_view s) {
        *out += "[" + std::string(s) + "]";
        return true;
    }

    bool ConvertOne (const UnboundConversion &conv, string_view s) {
        *out += "{";
        *out += std::string(s);
        *out += ":";
        *out += std::to_string(conv.arg_position) + "$";
        if (conv.width.is_from_arg()) {
            *out += std::to_string(conv.width.get_from_arg()) + "$*";
        }
        if (conv.precision.is_from_arg()) {
            *out += "." + std::to_string(conv.precision.get_from_arg()) + "$*";
        }
        *out += conv.conv.Char();
        *out += "}";
        return true;
    }
};

std::string SummarizeParsedFormat (const ParsedFormatBase &pc) {
    std::string out;
    if (!pc.ProcessFormat(SummarizeConsumer(&out)))
        out += "!";
    return out;
}

class ParsedFormatTest : public testing::Test { };

TEST_F(ParsedFormatTest, ValueSemantics) {
    ParsedFormatBase p1({}, true, {});  // empty format
    EXPECT_EQ("", SummarizeParsedFormat(p1));

    ParsedFormatBase p2 = p1;  // copy construct (empty)
    EXPECT_EQ(SummarizeParsedFormat(p1), SummarizeParsedFormat(p2));

    p1 = ParsedFormatBase("hello%s", true, {Conv::s});  // move assign
    EXPECT_EQ("[hello]{s:1$s}", SummarizeParsedFormat(p1));

    ParsedFormatBase p3 = p1;  // copy construct (nonempty)
    EXPECT_EQ(SummarizeParsedFormat(p1), SummarizeParsedFormat(p3));

    using std::swap;
    swap(p1, p2);
    EXPECT_EQ("", SummarizeParsedFormat(p1));
    EXPECT_EQ("[hello]{s:1$s}", SummarizeParsedFormat(p2));
    swap(p1, p2);  // undo

    p2 = p1;  // copy assign
    EXPECT_EQ(SummarizeParsedFormat(p1), SummarizeParsedFormat(p2));
}

struct ExpectParse {
    const char *in;
    std::initializer_list<Conv> conv_set;
    const char *out;
};

TEST_F(ParsedFormatTest, Parsing) {
    // Parse should be equivalent to that obtained by ConversionParseIterator.
    // No need to retest the parsing edge cases here.
    const ExpectParse kExpect[] = {
        {"", {}, ""},
        {"ab", {}, "[ab]"},
        {"a%d", {Conv::d}, "[a]{d:1$d}"},
        {"a%+d", {Conv::d}, "[a]{+d:1$d}"},
        {"a% d", {Conv::d}, "[a]{ d:1$d}"},
        {"a%b %d", {}, "[a]!"},  // stop after error
    };
    for (const auto &e : kExpect) {
        SCOPED_TRACE(e.in);
        EXPECT_EQ(e.out,
                  SummarizeParsedFormat(ParsedFormatBase(e.in, false, e.conv_set)));
    }
}

TEST_F(ParsedFormatTest, ParsingFlagOrder) {
    const ExpectParse kExpect[] = {
        {"a%+ 0d", {Conv::d}, "[a]{+ 0d:1$d}"},
        {"a%+0 d", {Conv::d}, "[a]{+0 d:1$d}"},
        {"a%0+ d", {Conv::d}, "[a]{0+ d:1$d}"},
        {"a% +0d", {Conv::d}, "[a]{ +0d:1$d}"},
        {"a%0 +d", {Conv::d}, "[a]{0 +d:1$d}"},
        {"a% 0+d", {Conv::d}, "[a]{ 0+d:1$d}"},
        {"a%+   0+d", {Conv::d}, "[a]{+   0+d:1$d}"},
    };
    for (const auto &e : kExpect) {
        SCOPED_TRACE(e.in);
        EXPECT_EQ(e.out,
                  SummarizeParsedFormat(ParsedFormatBase(e.in, false, e.conv_set)));
    }
}

}  // namespace
}  // namespace format_internal

}  // namespace abel
