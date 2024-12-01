#include "structure/graphics/spk_color_tester.hpp"

#include "structure/container/spk_json_object.hpp"

void ColorTest::SetUp()
{
    defaultColor = spk::Color();
    intColor = spk::Color(255, 128, 64, 32);
    floatColor = spk::Color(1.0f, 0.5f, 0.25f, 0.125f);

    // Mock JSON object for testing
    spk::JSON::Object jsonObject;
    jsonObject = L"0x3F7FBFFF";

    jsonColor = spk::Color(jsonObject);
}

void ColorTest::TearDown()
{

}

TEST_F(ColorTest, DefaultConstructor)
{
    EXPECT_FLOAT_EQ(defaultColor.r, 0.0f);
    EXPECT_FLOAT_EQ(defaultColor.g, 0.0f);
    EXPECT_FLOAT_EQ(defaultColor.b, 0.0f);
    EXPECT_FLOAT_EQ(defaultColor.a, 1.0f);
}

TEST_F(ColorTest, IntegerConstructor)
{
    EXPECT_FLOAT_EQ(intColor.r, 1.0f);
    EXPECT_FLOAT_EQ(intColor.g, 128.0f / 255.0f);
    EXPECT_FLOAT_EQ(intColor.b, 64.0f / 255.0f);
    EXPECT_FLOAT_EQ(intColor.a, 32.0f / 255.0f);
}

TEST_F(ColorTest, FloatConstructor)
{
    EXPECT_FLOAT_EQ(floatColor.r, 1.0f);
    EXPECT_FLOAT_EQ(floatColor.g, 0.5f);
    EXPECT_FLOAT_EQ(floatColor.b, 0.25f);
    EXPECT_FLOAT_EQ(floatColor.a, 0.125f);
}

TEST_F(ColorTest, JSONConstructor)
{
    EXPECT_FLOAT_EQ(jsonColor.r, 0.24705882f);
    EXPECT_FLOAT_EQ(jsonColor.g, 0.49803922f);
    EXPECT_FLOAT_EQ(jsonColor.b, 0.74901962f);
    EXPECT_FLOAT_EQ(jsonColor.a, 1.0f);
}

TEST_F(ColorTest, AdditionOperator)
{
    spk::Color addedColor = intColor + floatColor;

    EXPECT_FLOAT_EQ(addedColor.r, 1.0f);
    EXPECT_FLOAT_EQ(addedColor.g, 1.0f);
    EXPECT_FLOAT_EQ(addedColor.b, 0.50098038f);
}

TEST_F(ColorTest, SubtractionOperator)
{
    spk::Color subtractedColor = intColor - floatColor;

    EXPECT_FLOAT_EQ(subtractedColor.r, 0.0f);
    EXPECT_FLOAT_EQ(subtractedColor.g, 0.001960814f);
    EXPECT_FLOAT_EQ(subtractedColor.b, 0.000980407f);
}

TEST_F(ColorTest, StreamOperator)
{
    std::ostringstream oss;
    oss << intColor;
    EXPECT_EQ(oss.str(), "0xFF804020");
}

TEST_F(ColorTest, SaveColorToJSONString)
{
    spk::JSON::Object jsonObject;
    jsonObject << intColor;

    EXPECT_EQ(jsonObject.as<std::wstring>(), L"0xFF804020");
}

TEST_F(ColorTest, LoadColorFromJSONString)
{
    spk::JSON::Object jsonObject;
    jsonObject.set<std::wstring>(L"0xFF804020");

    spk::Color loadedColor;
    jsonObject >> loadedColor;

    EXPECT_FLOAT_EQ(loadedColor.r, 1.0f);
    EXPECT_FLOAT_EQ(loadedColor.g, 128.0f / 255.0f);
    EXPECT_FLOAT_EQ(loadedColor.b, 64.0f / 255.0f);
    EXPECT_FLOAT_EQ(loadedColor.a, 32.0f / 255.0f);
}

TEST_F(ColorTest, LoadColorFromInvalidJSONString)
{
    spk::JSON::Object jsonObject;
    jsonObject = L"0xZZZZZZZZ";

    spk::Color loadedColor;

    EXPECT_THROW(jsonObject >> loadedColor, std::runtime_error);
}

TEST_F(ColorTest, PredefinedColors)
{
    EXPECT_EQ(spk::Color::red, spk::Color(0xFF0000FF));
    EXPECT_EQ(spk::Color::blue, spk::Color(0x0000FFFF));
    EXPECT_EQ(spk::Color::green, spk::Color(0x00FF00FF));
    EXPECT_EQ(spk::Color::yellow, spk::Color(0xFFFF00FF));
    EXPECT_EQ(spk::Color::purple, spk::Color(0x7F00FFFF));
    EXPECT_EQ(spk::Color::cyan, spk::Color(0x00FFFFFF));
    EXPECT_EQ(spk::Color::orange, spk::Color(0xFF8000FF));
    EXPECT_EQ(spk::Color::magenta, spk::Color(0xFF007FFF));
    EXPECT_EQ(spk::Color::pink, spk::Color(0xFF00FFFF));
    EXPECT_EQ(spk::Color::black, spk::Color(0x000000FF));
    EXPECT_EQ(spk::Color::white, spk::Color(0xFFFFFFFF));
    EXPECT_EQ(spk::Color::grey, spk::Color(0x7F7F7FFF));
}