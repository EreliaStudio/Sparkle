#include <gtest/gtest.h>

#include <GL/glew.h>

#include <limits>
#include <string>

#include "exception.hpp"
#include "graphics/opengl/program.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	const std::string VertexShader = R"(#version 430 core
void main()
{
	const vec2 positions[3] = vec2[3](vec2(-0.5, -0.5), vec2(0.5, -0.5), vec2(0.0, 0.5));
	gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}
)";

	const std::string FragmentShader = R"(#version 430 core
layout(location = 0) out vec4 outputColor;
void main() { outputColor = vec4(1.0, 0.0, 0.0, 1.0); }
)";

	const std::string BindingVertexShader = R"(#version 430 core
layout(std140) uniform TransformBlock { mat4 transform; };
layout(std430) buffer OffsetBlock { vec4 offset; };
void main() { gl_Position = transform * (offset + vec4(0.0, 0.0, 0.0, 1.0)); }
)";

	const std::string BindingFragmentShader = R"(#version 430 core
layout(location = 0) out vec4 outputColor;
uniform sampler2D imageSampler;
void main() { outputColor = texture(imageSampler, vec2(0.5)); }
)";
}

TEST(ProgramTest, ValidProgramCompilesLinksBindsAndCanBeReplaced)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Program program(VertexShader, FragmentShader);
	EXPECT_TRUE(program.isValid());
	const auto firstGeneration = program.generation();
	program.activate(openGL.renderContext());
	GLint firstIdentifier = 0;
	::glGetIntegerv(GL_CURRENT_PROGRAM, &firstIdentifier);
	EXPECT_NE(firstIdentifier, 0);

	program.setSources(VertexShader + "\n// replacement", FragmentShader);
	EXPECT_GT(program.generation(), firstGeneration);
	program.activate(openGL.renderContext());
	GLint replacementIdentifier = 0;
	::glGetIntegerv(GL_CURRENT_PROGRAM, &replacementIdentifier);
	EXPECT_NE(replacementIdentifier, 0);
}

TEST(ProgramTest, EmptyOrInvalidSourcesAreRejectedAtActivationWithDriverLog)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Program empty;
	EXPECT_FALSE(empty.isValid());
	empty.validate();
	EXPECT_THROW(empty.activate(openGL.renderContext()), spk::Exception);
	spk::Program invalid("not valid GLSL", FragmentShader);
	try
	{
		invalid.activate(openGL.renderContext());
		FAIL() << "Expected shader compilation failure";
	}
	catch (const spk::Exception &exception)
	{
		EXPECT_NE(std::string(exception.what()).find("shader compilation failed"), std::string::npos);
	}
}

TEST(ProgramTest, UniformStorageAndSamplerBindingsReachLinkedProgram)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Program program(BindingVertexShader, BindingFragmentShader);
	program.bindUniformBlock("TransformBlock", 2);
	program.bindShaderStorageBlock("OffsetBlock", 3);
	program.bindSampler("imageSampler", 4);
	program.activate(openGL.renderContext());
	GLint identifier = 0;
	::glGetIntegerv(GL_CURRENT_PROGRAM, &identifier);
	const GLuint uniformIndex = ::glGetUniformBlockIndex(static_cast<GLuint>(identifier), "TransformBlock");
	GLint uniformBinding = -1;
	::glGetActiveUniformBlockiv(static_cast<GLuint>(identifier), uniformIndex, GL_UNIFORM_BLOCK_BINDING, &uniformBinding);
	EXPECT_EQ(uniformBinding, 2);
	const GLuint storageIndex = ::glGetProgramResourceIndex(static_cast<GLuint>(identifier), GL_SHADER_STORAGE_BLOCK, "OffsetBlock");
	GLint storageBinding = -1;
	const GLenum bindingProperty = GL_BUFFER_BINDING;
	::glGetProgramResourceiv(static_cast<GLuint>(identifier), GL_SHADER_STORAGE_BLOCK, storageIndex, 1,
		&bindingProperty, 1, nullptr, &storageBinding);
	EXPECT_EQ(storageBinding, 3);
	GLint samplerBinding = -1;
	::glGetUniformiv(static_cast<GLuint>(identifier), ::glGetUniformLocation(static_cast<GLuint>(identifier), "imageSampler"), &samplerBinding);
	EXPECT_EQ(samplerBinding, 4);
}

TEST(ProgramTest, BindingValidationRejectsEmptyMissingAndOutOfRangeBlocks)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Program program(BindingVertexShader, BindingFragmentShader);
	EXPECT_THROW(program.bindUniformBlock("", 0), std::invalid_argument);
	EXPECT_THROW(program.bindShaderStorageBlock("", 0), std::invalid_argument);
	program.bindUniformBlock("missing", 0);
	EXPECT_THROW(program.activate(openGL.renderContext()), spk::Exception);

	GLint maximum = 0;
	::glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maximum);
	spk::Program outOfRange(BindingVertexShader, BindingFragmentShader);
	outOfRange.bindUniformBlock("TransformBlock", static_cast<std::size_t>(maximum));
	EXPECT_THROW(outOfRange.activate(openGL.renderContext()), spk::Exception);
}

TEST(ProgramTest, RawTriangleDrawProducesExpectedFramebufferPixels)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Program program(VertexShader, FragmentShader);
	program.activate(openGL.renderContext());
	GLuint vertexArray = 0;
	::glGenVertexArrays(1, &vertexArray);
	::glBindVertexArray(vertexArray);
	program.renderRaw(spk::Program::Primitive::Triangles, 0, 3);
	const auto image = openGL.capture();
	const std::uint8_t *center = image.pixel({320, 240});
	EXPECT_EQ(center[0], 255);
	EXPECT_EQ(center[1], 0);
	EXPECT_EQ(center[2], 0);
	EXPECT_EQ(center[3], 255);
	::glDeleteVertexArrays(1, &vertexArray);
}

TEST(ProgramTest, SupportedPrimitivesAcceptZeroRawIndexedAndInstancedDraws)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Program program(VertexShader, FragmentShader);
	program.activate(openGL.renderContext());
	const spk::Program::Primitive primitives[] = {
		spk::Program::Primitive::Points,
		spk::Program::Primitive::Lines,
		spk::Program::Primitive::LineLoop,
		spk::Program::Primitive::LineStrip,
		spk::Program::Primitive::Triangles,
		spk::Program::Primitive::TriangleStrip,
		spk::Program::Primitive::TriangleFan};
	for (const auto primitive : primitives)
	{
		EXPECT_NO_THROW(program.renderRaw(primitive, 0, 0));
		EXPECT_NO_THROW(program.render(primitive, spk::IndexBuffer::Type::UnsignedInt, 0, 0));
		EXPECT_NO_THROW(program.renderInstanced(primitive, spk::IndexBuffer::Type::UnsignedInt, 0, 0, 0));
	}
}

TEST(ProgramTest, DrawCountFirstVertexAndIndexOffsetOverflowAreRejected)
{
	spk::Program program(VertexShader, FragmentShader);
	const std::size_t tooLargeCount = static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) + 1;
	const std::size_t tooLargeFirst = static_cast<std::size_t>(std::numeric_limits<GLint>::max()) + 1;
	EXPECT_THROW(program.renderRaw(spk::Program::Primitive::Triangles, 0, tooLargeCount), std::overflow_error);
	EXPECT_THROW(program.renderRaw(spk::Program::Primitive::Triangles, tooLargeFirst, 0), std::overflow_error);
	EXPECT_THROW(program.render(spk::Program::Primitive::Triangles, spk::IndexBuffer::Type::UnsignedInt,
		std::numeric_limits<std::size_t>::max(), 0), std::overflow_error);
	EXPECT_THROW(program.renderInstanced(spk::Program::Primitive::Triangles, spk::IndexBuffer::Type::UnsignedInt,
		0, 0, tooLargeCount), std::overflow_error);
}

TEST(ProgramTest, UnsupportedPrimitiveCastIsRejected)
{
	spk::Program program(VertexShader, FragmentShader);
	EXPECT_THROW(program.renderRaw(static_cast<spk::Program::Primitive>(255), 0, 0), std::logic_error);
}

TEST(ProgramTest, DISABLED_ShaderAndProgramCreationFailureNeedDriverInjectionSeam)
{
	GTEST_SKIP() << "Compilation diagnostics are covered, but forcing glCreateShader/glCreateProgram to return zero needs an injectable GL seam.";
}
