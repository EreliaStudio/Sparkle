#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include "core/window_surface_render_target.hpp"
#include "exception.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "rendering/command/clear_render_command.hpp"
#include "rendering/command/render_to_framebuffer_command.hpp"
#include "rendering/command/scissor_render_command.hpp"
#include "rendering/command/viewport_render_command.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	class InspectFramebufferCommand final : public spk::RenderCommand
	{
	private:
		std::vector<std::array<GLint, 5>> *_states;

	public:
		explicit InspectFramebufferCommand(std::vector<std::array<GLint, 5>> &states) :
			_states(&states)
		{
		}

		void execute(spk::RenderContext &) const override
		{
			std::array<GLint, 5> state{};
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, state.data());
			glGetIntegerv(GL_VIEWPORT, state.data() + 1);
			_states->push_back(state);
		}
	};

	struct TargetState
	{
		spk::Vector2UInt size;
		spk::Window::Surface::RenderTarget::Type type;
		std::array<GLint, 4> viewport{};
		std::array<GLint, 4> scissor{};
		GLboolean scissorEnabled = GL_FALSE;
	};

	class InspectTargetCommand final : public spk::RenderCommand
	{
	private:
		std::vector<TargetState> *_states;

	public:
		explicit InspectTargetCommand(std::vector<TargetState> &states) :
			_states(&states)
		{
		}

		void execute(spk::RenderContext &context) const override
		{
			TargetState state{
				.size = context.renderTargetSize(),
				.type = context.activeRenderTarget.type()};
			glGetIntegerv(GL_VIEWPORT, state.viewport.data());
			glGetIntegerv(GL_SCISSOR_BOX, state.scissor.data());
			state.scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
			_states->push_back(state);
		}
	};

	class FailingFramebufferCommand final : public spk::RenderCommand
	{
	public:
		void execute(spk::RenderContext &) const override
		{
			throw std::runtime_error("nested framebuffer render failed");
		}
	};

	[[nodiscard]] std::shared_ptr<const spk::RenderSnapshot> makeClearSnapshot(spk::Color color)
	{
		spk::RenderSnapshot::Builder builder;
		builder.renderPass({"clear", 0}).emplace<spk::ClearRenderCommand>(color, spk::ClearRenderCommand::Mask::Color);
		return std::make_shared<spk::RenderSnapshot>(builder.build());
	}
}

TEST(RenderToFramebufferCommandTest, SurfaceRenderTargetDistinguishesNativeAndOffscreen)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset({80, 60});
	const auto defaultTarget = gl.surface().defaultRenderTarget();
	EXPECT_EQ(defaultTarget.type(), spk::Window::Surface::RenderTarget::Type::Default);
	EXPECT_EQ(defaultTarget.size(), spk::Vector2UInt(80, 60));
	gl.setGeometry({.anchor = {0, 0}, .size = {48, 32}});
	EXPECT_EQ(defaultTarget.size(), spk::Vector2UInt(48, 32));
	EXPECT_EQ(gl.renderContext().renderTargetSize(), spk::Vector2UInt(48, 32));

	spk::Framebuffer framebuffer({8, 6}, {spk::Texture::Format::RGBA}, std::nullopt);
	const spk::Window::Surface::RenderTarget offscreen(gl.surface(), framebuffer.handle());
	EXPECT_EQ(offscreen.type(), spk::Window::Surface::RenderTarget::Type::Framebuffer);
	EXPECT_EQ(offscreen.size(), spk::Vector2UInt(8, 6));
	EXPECT_EQ(&offscreen.surface(), &gl.surface());

	spk::Window::Surface wrongSurface("ForeignTarget");
	const spk::Window::Surface::RenderTarget foreign(wrongSurface, framebuffer.handle());
	EXPECT_THROW(foreign.activate(gl.renderContext()), std::invalid_argument);

	offscreen.activate(gl.renderContext());
	GLint bound = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound);
	EXPECT_GT(bound, 0);
	defaultTarget.activate(gl.renderContext());
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound);
	EXPECT_EQ(bound, 0);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	gl.reset();
}

TEST(RenderToFramebufferCommandTest, RenderPassClearsTargetAndRestoresReadDrawAndViewport)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	spk::Framebuffer target({4, 3}, {spk::Texture::Format::RGBA}, std::nullopt);
	spk::Framebuffer previous({2, 2}, {spk::Texture::Format::RGBA}, std::nullopt);
	previous.activate(gl.renderContext());
	GLint previousName = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousName);
	ASSERT_GT(previousName, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(2, 3, 17, 19);

	std::vector<std::array<GLint, 5>> states;
	spk::RenderSnapshot::Builder nested;
	nested.renderPass({"clear", 0}).emplace<spk::ClearRenderCommand>(spk::Color{0.25f, 0.5f, 0.75f, 1.0f}, spk::ClearRenderCommand::Mask::Color);
	nested.renderPass({"inspect", 1}).emplace<InspectFramebufferCommand>(states);
	spk::RenderSnapshot::Builder outer;
	outer.renderPass({"offscreen", 0}).emplace<spk::RenderToFramebufferCommand>(target.handle(), std::make_shared<spk::RenderSnapshot>(nested.build()));
	outer.build().execute(gl.renderContext());

	ASSERT_EQ(states.size(), 1u);
	EXPECT_GT(states[0][0], 0);
	EXPECT_EQ((std::array<GLint, 4>{states[0][1], states[0][2], states[0][3], states[0][4]}), (std::array<GLint, 4>{0, 0, 4, 3}));
	GLint draw = -1, read = -1;
	std::array<GLint, 4> viewport{};
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
	glGetIntegerv(GL_VIEWPORT, viewport.data());
	EXPECT_EQ(draw, 0);
	EXPECT_EQ(read, previousName);
	EXPECT_EQ(viewport, (std::array<GLint, 4>{2, 3, 17, 19}));

	target.activate(gl.renderContext());
	std::array<std::uint8_t, 4> pixel{};
	glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
	EXPECT_NEAR(pixel[0], 64, 1);
	EXPECT_NEAR(pixel[1], 128, 1);
	EXPECT_NEAR(pixel[2], 191, 1);
	EXPECT_EQ(pixel[3], 255);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(RenderToFramebufferCommandTest, NestedFailureRestoresReadDrawAndViewport)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	spk::Framebuffer target({3, 2}, {spk::Texture::Format::RGBA}, std::nullopt);
	spk::Framebuffer previous({2, 2}, {spk::Texture::Format::RGBA}, std::nullopt);
	previous.activate(gl.renderContext());
	GLint previousName = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousName);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glViewport(5, 6, 22, 23);
	glScissor(7, 8, 9, 10);
	glEnable(GL_SCISSOR_TEST);

	spk::RenderSnapshot::Builder builder;
	builder.renderPass({"failure", 0}).emplace<FailingFramebufferCommand>();
	spk::RenderToFramebufferCommand command(
		target.handle(),
		std::make_shared<spk::RenderSnapshot>(builder.build()));
	EXPECT_THROW(command.execute(gl.renderContext()), spk::Exception);

	GLint draw = -1, read = -1;
	std::array<GLint, 4> viewport{};
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
	glGetIntegerv(GL_VIEWPORT, viewport.data());
	EXPECT_EQ(draw, previousName);
	EXPECT_EQ(read, 0);
	EXPECT_EQ(viewport, (std::array<GLint, 4>{5, 6, 22, 23}));
	std::array<GLint, 4> scissor{};
	glGetIntegerv(GL_SCISSOR_BOX, scissor.data());
	EXPECT_EQ(scissor, (std::array<GLint, 4>{7, 8, 9, 10}));
	EXPECT_EQ(glIsEnabled(GL_SCISSOR_TEST), GL_TRUE);
	EXPECT_EQ(
		gl.renderContext().activeRenderTarget.type(),
		spk::Window::Surface::RenderTarget::Type::Default);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(RenderToFramebufferCommandTest, NestedCommandsRestorePreviouslyActiveOffscreenTarget)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	spk::Framebuffer previous({9, 7}, {spk::Texture::Format::RGBA}, std::nullopt);
	spk::Framebuffer nested({4, 4}, {spk::Texture::Format::RGBA}, std::nullopt);
	auto &context = gl.renderContext();
	const spk::Window::Surface::RenderTarget previousTarget(gl.surface(), previous.handle());
	previousTarget.activate(context);
	context.activeRenderTarget = previousTarget;

	spk::RenderToFramebufferCommand(nested.handle(), makeClearSnapshot({0, 0, 0, 1}))
		.execute(context);
	EXPECT_EQ(context.renderTargetSize(), spk::Vector2UInt(9, 7));
	EXPECT_EQ(context.activeRenderTarget.type(), spk::Window::Surface::RenderTarget::Type::Framebuffer);

	spk::RenderSnapshot::Builder failing;
	failing.renderPass({"failure", 0}).emplace<FailingFramebufferCommand>();
	const auto failure = std::make_shared<spk::RenderSnapshot>(failing.build());
	EXPECT_THROW(
		spk::RenderToFramebufferCommand(nested.handle(), failure).execute(context),
		spk::Exception);
	EXPECT_EQ(context.renderTargetSize(), spk::Vector2UInt(9, 7));
	GLint bound = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound);
	EXPECT_GT(bound, 0);

	context.activeRenderTarget = gl.surface().defaultRenderTarget();
	gl.reset();
}

TEST(RenderToFramebufferCommandTest, NestedTargetsUseTheirOwnDimensionsAndRestoreOuterScissor)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	spk::Framebuffer outer({8, 6}, {spk::Texture::Format::RGBA}, std::nullopt);
	spk::Framebuffer inner({4, 4}, {spk::Texture::Format::RGBA}, std::nullopt);
	std::vector<TargetState> states;

	spk::RenderSnapshot::Builder child;
	auto &childPass = child.renderPass({"child", 0});
	childPass.emplace<spk::ViewportRenderCommand>(spk::Rect2D{.anchor = {0, 0}, .size = {4, 4}});
	childPass.emplace<InspectTargetCommand>(states);
	auto childSnapshot = std::make_shared<spk::RenderSnapshot>(child.build());

	spk::RenderSnapshot::Builder parent;
	auto &parentPass = parent.renderPass({"parent", 0});
	parentPass.emplace<spk::ViewportRenderCommand>(spk::Rect2D{.anchor = {1, 1}, .size = {4, 3}});
	parentPass.emplace<spk::ScissorRenderCommand>(spk::Rect2D{.anchor = {2, 1}, .size = {2, 2}});
	parentPass.emplace<InspectTargetCommand>(states);
	parentPass.emplace<spk::RenderToFramebufferCommand>(inner.handle(), childSnapshot);
	parentPass.emplace<InspectTargetCommand>(states);

	glScissor(11, 12, 13, 14);
	glEnable(GL_SCISSOR_TEST);
	spk::RenderToFramebufferCommand(outer.handle(), std::make_shared<spk::RenderSnapshot>(parent.build()))
		.execute(gl.renderContext());

	ASSERT_EQ(states.size(), 3u);
	EXPECT_EQ(states[0].size, spk::Vector2UInt(8, 6));
	EXPECT_EQ(states[0].type, spk::Window::Surface::RenderTarget::Type::Framebuffer);
	EXPECT_EQ(states[0].viewport, (std::array<GLint, 4>{1, 2, 4, 3}));
	EXPECT_EQ(states[0].scissor, (std::array<GLint, 4>{2, 3, 2, 2}));
	EXPECT_EQ(states[0].scissorEnabled, GL_TRUE);
	EXPECT_EQ(states[1].size, spk::Vector2UInt(4, 4));
	EXPECT_EQ(states[1].type, spk::Window::Surface::RenderTarget::Type::Framebuffer);
	EXPECT_EQ(states[1].viewport, (std::array<GLint, 4>{0, 0, 4, 4}));
	EXPECT_EQ(states[1].scissorEnabled, GL_FALSE);
	EXPECT_EQ(states[2].size, states[0].size);
	EXPECT_EQ(states[2].type, states[0].type);
	EXPECT_EQ(states[2].viewport, states[0].viewport);
	EXPECT_EQ(states[2].scissor, states[0].scissor);
	EXPECT_EQ(states[2].scissorEnabled, GL_TRUE);

	std::array<GLint, 4> scissor{};
	glGetIntegerv(GL_SCISSOR_BOX, scissor.data());
	EXPECT_EQ(scissor, (std::array<GLint, 4>{11, 12, 13, 14}));
	EXPECT_EQ(glIsEnabled(GL_SCISSOR_TEST), GL_TRUE);
	EXPECT_EQ(
		gl.renderContext().activeRenderTarget.type(),
		spk::Window::Surface::RenderTarget::Type::Default);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(RenderToFramebufferCommandTest, CapturedFramebufferVersionSurvivesResize)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	spk::Framebuffer target({4, 3});
	std::vector<std::array<GLint, 5>> states;
	spk::RenderSnapshot::Builder nested;
	nested.renderPass({"inspect", 0}).emplace<InspectFramebufferCommand>(states);
	spk::RenderToFramebufferCommand command(
		target.handle(),
		std::make_shared<spk::RenderSnapshot>(nested.build()));
	target.resize({8, 7});

	command.execute(gl.renderContext());
	ASSERT_EQ(states.size(), 1u);
	EXPECT_EQ(states[0][3], 4);
	EXPECT_EQ(states[0][4], 3);
	EXPECT_EQ(target.size(), spk::Vector2UInt(8, 7));
}

TEST(RenderToFramebufferCommandTest, RejectsMissingInputs)
{
	spk::Framebuffer framebuffer({4, 4});
	auto snapshot = makeClearSnapshot({0, 0, 0, 1});
	EXPECT_THROW(
		(void)spk::RenderToFramebufferCommand(spk::Framebuffer::Handle{}, snapshot),
		std::invalid_argument);
	EXPECT_THROW(
		(void)spk::RenderToFramebufferCommand(framebuffer.handle(), nullptr),
		std::invalid_argument);
}
