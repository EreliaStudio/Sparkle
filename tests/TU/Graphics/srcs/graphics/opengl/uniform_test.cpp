#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <cstdint>
#include <string>

#include "graphics/opengl/program.hpp"
#include "graphics/opengl/uniform.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	const std::string UniformVertexShader = R"(#version 430 core
uniform bool uBool;
uniform int uInt;
uniform uint uUInt;
uniform float uFloat;
uniform vec2 uVec2;
uniform ivec2 uIVec2;
uniform uvec2 uUVec2;
uniform vec3 uVec3;
uniform ivec3 uIVec3;
uniform uvec3 uUVec3;
uniform vec4 uVec4;
uniform ivec4 uIVec4;
uniform uvec4 uUVec4;
uniform mat2 uMat2;
uniform mat3 uMat3;
uniform mat4 uMat4;

void main()
{
	float value = (uBool ? 1.0 : 0.0) + float(uInt) + float(uUInt) + uFloat;
	value += uVec2.x + float(uIVec2.x) + float(uUVec2.x);
	value += uVec3.x + float(uIVec3.x) + float(uUVec3.x);
	value += uVec4.x + float(uIVec4.x) + float(uUVec4.x);
	value += uMat2[0][0] + uMat3[0][0] + uMat4[0][0];
	gl_Position = vec4(value * 0.000001, 0.0, 0.0, 1.0);
}
)";

	const std::string UniformFragmentShader = R"(#version 430 core
layout(location = 0) out vec4 outputColor;
void main() { outputColor = vec4(1.0); }
)";

	class FloatUniform : public spk::Uniform
	{
	public:
		FloatUniform(std::string name, float value) :
			Uniform(std::move(name), value)
		{
		}

		[[nodiscard]] float &value()
		{
			return cast<float>();
		}
	};

	[[nodiscard]] GLuint activeProgram()
	{
		GLint program = 0;
		::glGetIntegerv(GL_CURRENT_PROGRAM, &program);
		return static_cast<GLuint>(program);
	}
}

TEST(UniformTest, TypedDataCanBeEditedAndDerivedWithoutChangingItsType)
{
	FloatUniform uniform("uFloat", 1.0f);
	EXPECT_EQ(uniform.name(), "uFloat");
	EXPECT_FLOAT_EQ(uniform.value(), 1.0f);
	uniform.value() = 2.0f;
	uniform.setData(3.0f);
	EXPECT_FLOAT_EQ(std::as_const(uniform).cast<float>(), 3.0f);
	EXPECT_THROW((void)uniform.cast<std::int32_t>(), std::logic_error);
	EXPECT_THROW(uniform.setData(std::int32_t{4}), std::logic_error);
	EXPECT_THROW((void)spk::Uniform("", 1.0f), std::invalid_argument);
}

TEST(UniformTest, EverySupportedTypeActivatesOnTheCurrentProgram)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Program program(UniformVertexShader, UniformFragmentShader);
	program.activate(openGL.renderContext());

	spk::Uniform uBool("uBool", true);
	spk::Uniform uInt("uInt", std::int32_t{-2});
	spk::Uniform uUInt("uUInt", std::uint32_t{3});
	spk::Uniform uFloat("uFloat", 4.5f);
	spk::Uniform uVec2("uVec2", spk::Vector2{1.0f, 2.0f});
	spk::Uniform uIVec2("uIVec2", spk::Vector2Int{3, 4});
	spk::Uniform uUVec2("uUVec2", spk::Vector2UInt{5, 6});
	spk::Uniform uVec3("uVec3", spk::Vector3{7.0f, 8.0f, 9.0f});
	spk::Uniform uIVec3("uIVec3", spk::Vector3Int{10, 11, 12});
	spk::Uniform uUVec3("uUVec3", spk::Vector3UInt{13, 14, 15});
	spk::Uniform uVec4("uVec4", spk::Vector4{16.0f, 17.0f, 18.0f, 19.0f});
	spk::Uniform uIVec4("uIVec4", spk::Vector4Int{20, 21, 22, 23});
	spk::Uniform uUVec4("uUVec4", spk::Vector4UInt{24, 25, 26, 27});
	spk::Uniform uMat2("uMat2", spk::Matrix2x2::identity());
	spk::Uniform uMat3("uMat3", spk::Matrix3x3::identity());
	spk::Uniform uMat4("uMat4", spk::Matrix4x4::identity());

	uBool.activate();
	uInt.activate();
	uUInt.activate();
	uFloat.activate();
	uVec2.activate();
	uIVec2.activate();
	uUVec2.activate();
	uVec3.activate();
	uIVec3.activate();
	uUVec3.activate();
	uVec4.activate();
	uIVec4.activate();
	uUVec4.activate();
	uMat2.activate();
	uMat3.activate();
	uMat4.activate();
	EXPECT_EQ(::glGetError(), GL_NO_ERROR);

	const GLuint identifier = activeProgram();
	GLint boolValue = 0;
	::glGetUniformiv(identifier, ::glGetUniformLocation(identifier, "uBool"), &boolValue);
	EXPECT_EQ(boolValue, GL_TRUE);
	GLint intValue = 0;
	::glGetUniformiv(identifier, ::glGetUniformLocation(identifier, "uInt"), &intValue);
	EXPECT_EQ(intValue, -2);
	GLuint uintValue = 0;
	::glGetUniformuiv(identifier, ::glGetUniformLocation(identifier, "uUInt"), &uintValue);
	EXPECT_EQ(uintValue, 3u);
	GLfloat floatValue = 0.0f;
	::glGetUniformfv(identifier, ::glGetUniformLocation(identifier, "uFloat"), &floatValue);
	EXPECT_FLOAT_EQ(floatValue, 4.5f);

	std::array<GLfloat, 4> vectorValue{};
	::glGetUniformfv(identifier, ::glGetUniformLocation(identifier, "uVec4"), vectorValue.data());
	EXPECT_EQ(vectorValue, (std::array<GLfloat, 4>{16.0f, 17.0f, 18.0f, 19.0f}));

	std::array<GLfloat, 16> matrixValue{};
	::glGetUniformfv(identifier, ::glGetUniformLocation(identifier, "uMat4"), matrixValue.data());
	EXPECT_FLOAT_EQ(matrixValue[0], 1.0f);
	EXPECT_FLOAT_EQ(matrixValue[5], 1.0f);
	EXPECT_FLOAT_EQ(matrixValue[10], 1.0f);
	EXPECT_FLOAT_EQ(matrixValue[15], 1.0f);
}

TEST(UniformTest, ActivationRequiresAProgramAndMissingUniformIsIgnored)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Uniform uniform("missing", 1.0f);
	EXPECT_THROW(uniform.activate(), std::logic_error);

	spk::Program program(UniformVertexShader, UniformFragmentShader);
	program.activate(openGL.renderContext());
	EXPECT_NO_THROW(uniform.activate());
	EXPECT_EQ(::glGetError(), GL_NO_ERROR);
}
