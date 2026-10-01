#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

#include "core/platform/window.hpp"
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

	class FloatUniform : public spk::Uniform<float>
	{
	public:
		using Uniform::Uniform;
	};

	[[nodiscard]] GLuint activeProgram()
	{
		GLint program = 0;
		::glGetIntegerv(GL_CURRENT_PROGRAM, &program);
		return static_cast<GLuint>(program);
	}

	[[nodiscard]] float uniformFloat(GLuint program, const char *name)
	{
		GLfloat value = 0.0f;
		::glGetUniformfv(program, ::glGetUniformLocation(program, name), &value);
		return value;
	}

	class SecondaryOpenGLContext
	{
	private:
		spk::WinAPI::Window::Class _windowClass{"Sparkle_Uniform_Secondary_Context"};
		spk::Window::Native _native{"UniformSecondaryNative"};
		spk::Window::Surface _surface{"UniformSecondarySurface"};
		spk::RenderContext _context{.targetSurface = &_surface};

	public:
		SecondaryOpenGLContext()
		{
			spk::WinAPI::Window::CreationInfo creation;
			creation.title = "Sparkle uniform secondary context";
			creation.width = 64;
			creation.height = 64;
			creation.visible = false;
			_native.window().create(_windowClass, creation);
			_surface.create(_native.window());
			_surface.setGeometry({.anchor = {0, 0}, .size = {64, 64}});
		}

		~SecondaryOpenGLContext()
		{
			_surface.makeCurrent();
			_surface.destroy();
			_native.window().destroy();
		}

		[[nodiscard]] spk::RenderContext &context()
		{
			_surface.makeCurrent();
			return _context;
		}
	};
}

TEST(UniformTest, TypeIsDeclaredAtCompileTimeAndDataIsDirectlyEditable)
{
	static_assert(std::is_same_v<spk::Uniform<std::uint32_t>::Data, std::uint32_t>);
	static_assert(std::is_same_v<spk::Uniform<float_t>::Data, float_t>);
	static_assert(std::is_same_v<spk::Uniform<spk::Vector3>::Data, spk::Vector3>);
	static_assert(std::is_same_v<spk::Uniform<float>::BindingPoint, GLint>);

	FloatUniform uniform("uFloat", 1.0f);
	EXPECT_EQ(uniform.name(), "uFloat");
	EXPECT_FLOAT_EQ(uniform.data(), 1.0f);
	uniform.data() = 2.0f;
	uniform.setData(3.0f);
	EXPECT_FLOAT_EQ(std::as_const(uniform).data(), 3.0f);
	EXPECT_THROW((void)spk::Uniform<float>(""), std::invalid_argument);
}

TEST(UniformTest, EverySupportedTypeActivatesOnTheCurrentProgram)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	auto &context = openGL.renderContext();
	spk::Program program(UniformVertexShader, UniformFragmentShader);
	program.activate(context);

	spk::Uniform<bool> uBool("uBool", true);
	spk::Uniform<std::int32_t> uInt("uInt", -2);
	spk::Uniform<std::uint32_t> uUInt("uUInt", 3u);
	spk::Uniform<float_t> uFloat("uFloat", 4.5f);
	spk::Uniform<spk::Vector2> uVec2("uVec2", {1.0f, 2.0f});
	spk::Uniform<spk::Vector2Int> uIVec2("uIVec2", {3, 4});
	spk::Uniform<spk::Vector2UInt> uUVec2("uUVec2", {5, 6});
	spk::Uniform<spk::Vector3> uVec3("uVec3", {7.0f, 8.0f, 9.0f});
	spk::Uniform<spk::Vector3Int> uIVec3("uIVec3", {10, 11, 12});
	spk::Uniform<spk::Vector3UInt> uUVec3("uUVec3", {13, 14, 15});
	spk::Uniform<spk::Vector4> uVec4("uVec4", {16.0f, 17.0f, 18.0f, 19.0f});
	spk::Uniform<spk::Vector4Int> uIVec4("uIVec4", {20, 21, 22, 23});
	spk::Uniform<spk::Vector4UInt> uUVec4("uUVec4", {24, 25, 26, 27});
	spk::Uniform<spk::Matrix2x2> uMat2("uMat2", spk::Matrix2x2::identity());
	spk::Uniform<spk::Matrix3x3> uMat3("uMat3", spk::Matrix3x3::identity());
	spk::Uniform<spk::Matrix4x4> uMat4("uMat4", spk::Matrix4x4::identity());

	uBool.activate(context);
	uInt.activate(context);
	uUInt.activate(context);
	uFloat.activate(context);
	uVec2.activate(context);
	uIVec2.activate(context);
	uUVec2.activate(context);
	uVec3.activate(context);
	uIVec3.activate(context);
	uUVec3.activate(context);
	uVec4.activate(context);
	uIVec4.activate(context);
	uUVec4.activate(context);
	uMat2.activate(context);
	uMat3.activate(context);
	uMat4.activate(context);
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
	EXPECT_FLOAT_EQ(uniformFloat(identifier, "uFloat"), 4.5f);

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

TEST(UniformTest, RepeatedActivationUpdatesTheResolvedBinding)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	auto &context = openGL.renderContext();
	spk::Program program(UniformVertexShader, UniformFragmentShader);
	spk::Uniform<float> uniform("uFloat", 1.0f);
	program.activate(context);
	const GLuint identifier = activeProgram();

	uniform.activate(context);
	EXPECT_FLOAT_EQ(uniformFloat(identifier, "uFloat"), 1.0f);

	uniform.setData(6.0f);
	uniform.activate(context);
	EXPECT_FLOAT_EQ(uniformFloat(identifier, "uFloat"), 6.0f);
}

TEST(UniformTest, BindingCacheIsIndependentForEachSurface)
{
	auto &shared = sparkle_test::OpenGLTestContext::instance();
	shared.reset();
	auto &sharedContext = shared.renderContext();
	spk::Program program(UniformVertexShader, UniformFragmentShader);
	spk::Uniform<float> uniform("uFloat", 1.0f);

	program.activate(sharedContext);
	const GLuint sharedProgram = activeProgram();
	uniform.activate(sharedContext);
	EXPECT_FLOAT_EQ(uniformFloat(sharedProgram, "uFloat"), 1.0f);

	{
		SecondaryOpenGLContext secondary;
		auto &secondaryContext = secondary.context();
		program.activate(secondaryContext);
		const GLuint secondaryProgram = activeProgram();
		uniform.setData(2.0f);
		uniform.activate(secondaryContext);
		EXPECT_FLOAT_EQ(uniformFloat(secondaryProgram, "uFloat"), 2.0f);
	}

	shared.makeCurrent();
	program.activate(sharedContext);
	uniform.setData(3.0f);
	uniform.activate(sharedContext);
	EXPECT_FLOAT_EQ(uniformFloat(sharedProgram, "uFloat"), 3.0f);
}

TEST(UniformTest, ActiveProgramTrackingBelongsToEachRenderContext)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::RenderContext first{.targetSurface = &openGL.surface()};
	spk::RenderContext second{.targetSurface = &openGL.surface()};
	spk::Program program(UniformVertexShader, UniformFragmentShader);
	spk::Uniform<float> uniform("uFloat", 7.0f);

	program.activate(first);
	EXPECT_NO_THROW(uniform.activate(first));
	EXPECT_THROW(uniform.activate(second), std::logic_error);

	program.activate(second);
	EXPECT_NO_THROW(uniform.activate(second));
	EXPECT_FLOAT_EQ(uniformFloat(activeProgram(), "uFloat"), 7.0f);
}

TEST(UniformTest, TracksProgramsThroughTheRenderContext)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	auto &context = openGL.renderContext();
	spk::Program first(UniformVertexShader, UniformFragmentShader);
	spk::Program second(UniformVertexShader, UniformFragmentShader);
	spk::Uniform<float> uniform("uFloat", 1.0f);

	first.activate(context);
	const GLuint firstIdentifier = activeProgram();
	uniform.activate(context);
	EXPECT_FLOAT_EQ(uniformFloat(firstIdentifier, "uFloat"), 1.0f);

	second.activate(context);
	const GLuint secondIdentifier = activeProgram();
	uniform.setData(2.0f);
	uniform.activate(context);
	EXPECT_FLOAT_EQ(uniformFloat(secondIdentifier, "uFloat"), 2.0f);

	first.activate(context);
	uniform.setData(3.0f);
	uniform.activate(context);
	EXPECT_FLOAT_EQ(uniformFloat(firstIdentifier, "uFloat"), 3.0f);
}

TEST(UniformTest, RefreshesBindingAfterProgramRelink)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	auto &context = openGL.renderContext();
	spk::Program program(UniformVertexShader, UniformFragmentShader);
	spk::Uniform<float> uniform("uFloat", 1.0f);

	program.activate(context);
	uniform.activate(context);
	const auto generation = program.generation();

	program.setSources(UniformVertexShader + "\n", UniformFragmentShader);
	program.activate(context);
	EXPECT_GT(program.generation(), generation);

	uniform.setData(4.0f);
	uniform.activate(context);
	EXPECT_FLOAT_EQ(uniformFloat(activeProgram(), "uFloat"), 4.0f);
}

TEST(UniformTest, ProgramHandleActivationUpdatesTheRenderContext)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	auto &context = openGL.renderContext();
	spk::Program program(UniformVertexShader, UniformFragmentShader);
	const auto handle = program.handle();
	spk::Uniform<float> uniform("uFloat", 5.0f);

	handle.activate(context);
	EXPECT_NO_THROW(uniform.activate(context));
	EXPECT_FLOAT_EQ(uniformFloat(activeProgram(), "uFloat"), 5.0f);
}

TEST(UniformTest, ActivationRequiresTargetSurfaceAndActiveProgram)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Uniform<float> uniform("uFloat", 1.0f);
	spk::RenderContext noSurface{};
	EXPECT_THROW(uniform.activate(noSurface), std::invalid_argument);

	spk::RenderContext inactiveContext{.targetSurface = &openGL.surface()};
	EXPECT_THROW(uniform.activate(inactiveContext), std::logic_error);
}

TEST(UniformTest, MissingUniformIsIgnored)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	auto &context = openGL.renderContext();
	spk::Program program(UniformVertexShader, UniformFragmentShader);
	spk::Uniform<float> uniform("missing", 1.0f);
	program.activate(context);
	EXPECT_NO_THROW(uniform.activate(context));
	EXPECT_EQ(::glGetError(), GL_NO_ERROR);
}
