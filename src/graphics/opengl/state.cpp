#include "graphics/opengl/state.hpp"

#include <GL/glew.h>

#include <algorithm>
#include <array>
#include <utility>

namespace spk::OpenGL
{
	namespace
	{
		template <typename TEnum>
		[[nodiscard]] constexpr GLenum native(TEnum value) noexcept
		{
			return static_cast<GLenum>(value);
		}

		[[nodiscard]] GLint integer(GLenum name) noexcept
		{
			GLint value = 0;
			glGetIntegerv(name, &value);
			return value;
		}

		[[nodiscard]] GLint indexedInteger(GLenum name, GLuint index) noexcept
		{
			GLint value = 0;
			glGetIntegeri_v(name, index, &value);
			return value;
		}

		[[nodiscard]] bool enabled(GLenum name) noexcept
		{
			return glIsEnabled(name) == GL_TRUE;
		}

		void toggle(GLenum name, bool value) noexcept
		{
			if (value)
			{
				glEnable(name);
			}
			else
			{
				glDisable(name);
			}
		}

		[[nodiscard]] State::StencilTest::Face captureStencilFace(bool back)
		{
			using Face = State::StencilTest::Face;
			Face result;
			result.comparison = static_cast<State::Comparison>(integer(back ? GL_STENCIL_BACK_FUNC : GL_STENCIL_FUNC));
			result.reference = integer(back ? GL_STENCIL_BACK_REF : GL_STENCIL_REF);
			result.comparisonMask = static_cast<std::uint32_t>(integer(back ? GL_STENCIL_BACK_VALUE_MASK : GL_STENCIL_VALUE_MASK));
			result.writeMask = static_cast<std::uint32_t>(integer(back ? GL_STENCIL_BACK_WRITEMASK : GL_STENCIL_WRITEMASK));
			result.fail = static_cast<State::StencilOperation>(integer(back ? GL_STENCIL_BACK_FAIL : GL_STENCIL_FAIL));
			result.depthFail = static_cast<State::StencilOperation>(integer(back ? GL_STENCIL_BACK_PASS_DEPTH_FAIL : GL_STENCIL_PASS_DEPTH_FAIL));
			result.pass = static_cast<State::StencilOperation>(integer(back ? GL_STENCIL_BACK_PASS_DEPTH_PASS : GL_STENCIL_PASS_DEPTH_PASS));
			return result;
		}

		void applyStencilFace(GLenum face, const State::StencilTest::Face &state) noexcept
		{
			glStencilFuncSeparate(face, native(state.comparison), state.reference, state.comparisonMask);
			glStencilOpSeparate(face, native(state.fail), native(state.depthFail), native(state.pass));
			glStencilMaskSeparate(face, state.writeMask);
		}

		[[nodiscard]] std::size_t drawBufferCount() noexcept
		{
			return static_cast<std::size_t>(std::max(1, integer(GL_MAX_DRAW_BUFFERS)));
		}
	}

	State::DepthTest State::DepthTest::capture()
	{
		DepthTest result;
		result.enabled = OpenGL::enabled(GL_DEPTH_TEST);
		result.comparison = static_cast<Comparison>(integer(GL_DEPTH_FUNC));
		glGetDoublev(GL_DEPTH_CLEAR_VALUE, &result.clear);
		glGetDoublev(GL_DEPTH_RANGE, result.range.data());
		GLboolean mask = GL_TRUE;
		glGetBooleanv(GL_DEPTH_WRITEMASK, &mask);
		result.write = mask == GL_TRUE;
		return result;
	}

	void State::DepthTest::apply() const noexcept
	{
		toggle(GL_DEPTH_TEST, enabled);
		glDepthFunc(native(comparison));
		glDepthMask(write ? GL_TRUE : GL_FALSE);
		glClearDepth(clear);
		glDepthRange(range[0], range[1]);
	}

	State::StencilTest State::StencilTest::capture()
	{
		StencilTest result;
		result.enabled = OpenGL::enabled(GL_STENCIL_TEST);
		result.clear = integer(GL_STENCIL_CLEAR_VALUE);
		result.front = captureStencilFace(false);
		result.back = captureStencilFace(true);
		return result;
	}

	void State::StencilTest::apply() const noexcept
	{
		toggle(GL_STENCIL_TEST, enabled);
		glClearStencil(clear);
		applyStencilFace(GL_FRONT, front);
		applyStencilFace(GL_BACK, back);
	}

	State::Blend State::Blend::capture()
	{
		Blend result;
		glGetFloatv(GL_BLEND_COLOR, result.constantColor.data());
		result.attachments.reserve(drawBufferCount());
		for (GLuint index = 0; index < drawBufferCount(); ++index)
		{
			Attachment attachment;
			attachment.enabled = glIsEnabledi(GL_BLEND, index) == GL_TRUE;
			attachment.sourceRGB = static_cast<BlendFactor>(indexedInteger(GL_BLEND_SRC_RGB, index));
			attachment.destinationRGB = static_cast<BlendFactor>(indexedInteger(GL_BLEND_DST_RGB, index));
			attachment.sourceAlpha = static_cast<BlendFactor>(indexedInteger(GL_BLEND_SRC_ALPHA, index));
			attachment.destinationAlpha = static_cast<BlendFactor>(indexedInteger(GL_BLEND_DST_ALPHA, index));
			attachment.equationRGB = static_cast<BlendEquation>(indexedInteger(GL_BLEND_EQUATION_RGB, index));
			attachment.equationAlpha = static_cast<BlendEquation>(indexedInteger(GL_BLEND_EQUATION_ALPHA, index));
			result.attachments.push_back(attachment);
		}
		return result;
	}

	State::Blend State::Blend::disabled()
	{
		Blend result;
		result.attachments.resize(drawBufferCount());
		return result;
	}

	void State::Blend::apply() const noexcept
	{
		glBlendColor(constantColor[0], constantColor[1], constantColor[2], constantColor[3]);
		for (GLuint index = 0; index < attachments.size(); ++index)
		{
			const auto &attachment = attachments[index];
			if (attachment.enabled)
			{
				glEnablei(GL_BLEND, index);
			}
			else
			{
				glDisablei(GL_BLEND, index);
			}
			glBlendFuncSeparatei(index, native(attachment.sourceRGB), native(attachment.destinationRGB),
				native(attachment.sourceAlpha), native(attachment.destinationAlpha));
			glBlendEquationSeparatei(index, native(attachment.equationRGB), native(attachment.equationAlpha));
		}
	}

	State::Rasterization State::Rasterization::capture()
	{
		Rasterization result;
		result.cullEnabled = OpenGL::enabled(GL_CULL_FACE);
		result.cull = static_cast<Culling>(integer(GL_CULL_FACE_MODE));
		result.frontWinding = static_cast<Winding>(integer(GL_FRONT_FACE));
		std::array<GLint, 2> mode{};
		glGetIntegerv(GL_POLYGON_MODE, mode.data());
		result.polygonMode = {static_cast<PolygonMode>(mode[0]), static_cast<PolygonMode>(mode[1])};
		result.polygonOffsetEnabled = OpenGL::enabled(GL_POLYGON_OFFSET_FILL);
		glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &result.polygonOffsetFactor);
		glGetFloatv(GL_POLYGON_OFFSET_UNITS, &result.polygonOffsetUnits);
		result.multisampleEnabled = OpenGL::enabled(GL_MULTISAMPLE);
		result.sampleAlphaToCoverageEnabled = OpenGL::enabled(GL_SAMPLE_ALPHA_TO_COVERAGE);
		return result;
	}

	void State::Rasterization::apply() const noexcept
	{
		toggle(GL_CULL_FACE, cullEnabled);
		glCullFace(native(cull));
		glFrontFace(native(frontWinding));
		glPolygonMode(GL_FRONT, native(polygonMode[0]));
		glPolygonMode(GL_BACK, native(polygonMode[1]));
		toggle(GL_POLYGON_OFFSET_FILL, polygonOffsetEnabled);
		glPolygonOffset(polygonOffsetFactor, polygonOffsetUnits);
		toggle(GL_MULTISAMPLE, multisampleEnabled);
		toggle(GL_SAMPLE_ALPHA_TO_COVERAGE, sampleAlphaToCoverageEnabled);
	}

	State::Scissor State::Scissor::capture()
	{
		Scissor result;
		result.enabled = OpenGL::enabled(GL_SCISSOR_TEST);
		glGetIntegerv(GL_SCISSOR_BOX, result.rectangle.data());
		return result;
	}

	void State::Scissor::apply() const noexcept
	{
		toggle(GL_SCISSOR_TEST, enabled);
		glScissor(rectangle[0], rectangle[1], rectangle[2], rectangle[3]);
	}

	State::Viewport State::Viewport::capture()
	{
		Viewport result;
		glGetIntegerv(GL_VIEWPORT, result.rectangle.data());
		return result;
	}

	void State::Viewport::apply() const noexcept
	{
		glViewport(rectangle[0], rectangle[1], rectangle[2], rectangle[3]);
	}

	State::ColorOutput State::ColorOutput::capture()
	{
		ColorOutput result;
		glGetFloatv(GL_COLOR_CLEAR_VALUE, result.clear.data());
		result.framebufferSRGB = OpenGL::enabled(GL_FRAMEBUFFER_SRGB);
		result.masks.reserve(drawBufferCount());
		for (GLuint index = 0; index < drawBufferCount(); ++index)
		{
			std::array<GLboolean, 4> mask{};
			glGetBooleani_v(GL_COLOR_WRITEMASK, index, mask.data());
			result.masks.push_back({mask[0] == GL_TRUE, mask[1] == GL_TRUE,
				mask[2] == GL_TRUE, mask[3] == GL_TRUE});
		}
		return result;
	}

	State::ColorOutput State::ColorOutput::writable(bool framebufferSRGB)
	{
		ColorOutput result;
		result.framebufferSRGB = framebufferSRGB;
		result.masks.resize(drawBufferCount(), {true, true, true, true});
		return result;
	}

	void State::ColorOutput::apply() const noexcept
	{
		toggle(GL_FRAMEBUFFER_SRGB, framebufferSRGB);
		glClearColor(clear[0], clear[1], clear[2], clear[3]);
		for (GLuint index = 0; index < masks.size(); ++index)
		{
			const auto &mask = masks[index];
			glColorMaski(index, mask[0] ? GL_TRUE : GL_FALSE, mask[1] ? GL_TRUE : GL_FALSE,
				mask[2] ? GL_TRUE : GL_FALSE, mask[3] ? GL_TRUE : GL_FALSE);
		}
	}

	void State::Clear::color(std::uint32_t attachment, const std::array<float, 4> &rgba) noexcept
	{
		glClearBufferfv(GL_COLOR, static_cast<GLint>(attachment), rgba.data());
	}

	void State::Clear::depth(float value) noexcept
	{
		glClearBufferfv(GL_DEPTH, 0, &value);
	}

	void State::Clear::stencil(int value) noexcept
	{
		glClearBufferiv(GL_STENCIL, 0, &value);
	}

	State::Bindings State::Bindings::capture()
	{
		Bindings result;
		result.drawFramebuffer = static_cast<std::uint32_t>(integer(GL_DRAW_FRAMEBUFFER_BINDING));
		result.readFramebuffer = static_cast<std::uint32_t>(integer(GL_READ_FRAMEBUFFER_BINDING));
		result.program = static_cast<std::uint32_t>(integer(GL_CURRENT_PROGRAM));
		result.vertexArray = static_cast<std::uint32_t>(integer(GL_VERTEX_ARRAY_BINDING));
		result.activeTexture = static_cast<std::uint32_t>(integer(GL_ACTIVE_TEXTURE) - GL_TEXTURE0);
		const auto count = static_cast<std::size_t>(std::max(1, integer(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS)));
		result.textureUnits.reserve(count);
		for (GLuint index = 0; index < count; ++index)
		{
			glActiveTexture(GL_TEXTURE0 + index);
			result.textureUnits.push_back({
				static_cast<std::uint32_t>(integer(GL_TEXTURE_BINDING_2D)),
				static_cast<std::uint32_t>(indexedInteger(GL_SAMPLER_BINDING, index))});
		}
		glActiveTexture(GL_TEXTURE0 + result.activeTexture);
		return result;
	}

	void State::Bindings::apply() const noexcept
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFramebuffer);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, readFramebuffer);
		glUseProgram(program);
		glBindVertexArray(vertexArray);
		for (GLuint index = 0; index < textureUnits.size(); ++index)
		{
			glActiveTexture(GL_TEXTURE0 + index);
			glBindTexture(GL_TEXTURE_2D, textureUnits[index].texture2D);
			glBindSampler(index, textureUnits[index].sampler);
		}
		glActiveTexture(GL_TEXTURE0 + activeTexture);
	}

	State State::capture()
	{
		State result;
		result.depthTest = DepthTest::capture();
		result.stencilTest = StencilTest::capture();
		result.blend = Blend::capture();
		result.rasterization = Rasterization::capture();
		result.scissor = Scissor::capture();
		result.viewport = Viewport::capture();
		result.colorOutput = ColorOutput::capture();
		result.bindings = Bindings::capture();
		return result;
	}

	void State::apply() const noexcept
	{
		bindings.apply();
		depthTest.apply();
		stencilTest.apply();
		blend.apply();
		rasterization.apply();
		scissor.apply();
		viewport.apply();
		colorOutput.apply();
	}

	State::Scope::Scope() :
		_previous(State::capture())
	{
	}

	State::Scope::~Scope()
	{
		_previous.apply();
	}
}
