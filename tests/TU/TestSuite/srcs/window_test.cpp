#include <gtest/gtest.h>

#include <memory>

#include "core/window.hpp"

namespace
{
	struct Parts
	{
		std::shared_ptr<spk::Window::Native> native = std::make_shared<spk::Window::Native>("window-native");
		std::shared_ptr<spk::Window::State> state = std::make_shared<spk::Window::State>("window-state");
		std::shared_ptr<spk::Window::Surface> surface = std::make_shared<spk::Window::Surface>("window-surface");
	};

	[[nodiscard]] spk::Rect2D testGeometry()
	{
		return spk::Rect2D{
			.anchor = {21, 34},
			.size = {640, 360}};
	}
}

TEST(WindowTest, StandardFacadeExposesRootGeometryClosingAndClosedState)
{
	Parts parts;
	parts.surface->setGeometry(testGeometry());
	spk::Window window(parts.native, parts.state, parts.surface);

	EXPECT_EQ(&window.root(), &parts.state->root());
	EXPECT_EQ(window.geometry(), parts.surface->geometry());
	EXPECT_FALSE(window.isClosing());
	EXPECT_FALSE(window.isClosed());

	parts.native->markReady();
	parts.state->markReady();
	EXPECT_FALSE(window.isClosing());

	parts.native->beginRelease();
	EXPECT_TRUE(window.isClosing());
	EXPECT_FALSE(window.isClosed());

	parts.native->markReleased();
	parts.state->markReleased();
	parts.surface->destroy();

	EXPECT_TRUE(window.isClosed());
	EXPECT_FALSE(window.isClosing());
}

TEST(WindowTest, AnyPartStartingReleaseMakesTheFacadeClosing)
{
	for (int releasingPart = 0; releasingPart < 3; ++releasingPart)
	{
		Parts parts;
		spk::Window window(parts.native, parts.state, parts.surface);

		if (releasingPart == 0)
			parts.native->beginRelease();
		else if (releasingPart == 1)
			parts.state->beginRelease();
		else
			parts.surface->destroy();

		EXPECT_TRUE(window.isClosing());
		EXPECT_FALSE(window.isClosed());
	}
}

TEST(WindowTest, ClosedRequiresEveryPartToBeReleased)
{
	Parts parts;
	spk::Window window(parts.native, parts.state, parts.surface);

	parts.native->markReleased();
	EXPECT_FALSE(window.isClosed());
	parts.state->markReleased();
	EXPECT_FALSE(window.isClosed());
	parts.surface->destroy();
	EXPECT_TRUE(window.isClosed());
}

TEST(WindowTest, ConstAndMutableRootAccessReferToTheSameObject)
{
	Parts parts;
	spk::Window window(parts.native, parts.state, parts.surface);
	const spk::Window &constWindow = window;

	EXPECT_EQ(&window.root(), &constWindow.root());
}

TEST(WindowTest, SharedPartsOutliveTheFacade)
{
	Parts parts;
	const auto native = parts.native;
	const auto state = parts.state;
	const auto surface = parts.surface;

	{
		spk::Window window(parts.native, parts.state, parts.surface);
		EXPECT_EQ(&window.root(), &state->root());
	}

	EXPECT_EQ(native->lifeCycle(), spk::Window::LifeCycle::Pending);
	EXPECT_EQ(state->lifeCycle(), spk::Window::LifeCycle::Pending);
	EXPECT_EQ(surface->lifeCycle(), spk::Window::LifeCycle::Pending);
	EXPECT_NO_THROW(surface->destroy());
}
