#include <gtest/gtest.h>

#include <GL/glew.h>
#include <Windows.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "core/platform/window.hpp"
#include "core/window.hpp"
#include "engine/entity3d.hpp"
#include "light/channel.hpp"
#include "light/directional.hpp"
#include "light/selection.hpp"
#include "shadow_map/result.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/engine/rendering_behaviour.hpp"

namespace
{
	[[nodiscard]] std::string uniqueClassName()
	{
		static std::atomic_uint64_t counter = 0;
		return "Sparkle_ResultCollection_" + std::to_string(::GetCurrentProcessId()) + "_" +
			   std::to_string(counter.fetch_add(1));
	}

	struct HiddenFrame
	{
		spk::WinAPI::Window::Class registration{uniqueClassName()};
		spk::Window::Native native{"shadow-results-frame"};

		HiddenFrame()
		{
			spk::WinAPI::Window::CreationInfo info;
			info.title = "Shadow result surface";
			info.width = 32;
			info.height = 32;
			info.visible = false;
			native.window().create(registration, info);
		}

		~HiddenFrame()
		{
			try
			{
				native.window().destroy();
			} catch (...)
			{
			}
		}
	};

	struct Scene
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"camera"};
		spk::CameraComponent3D &camera = cameraEntity.addComponent<spk::CameraComponent3D>();
		std::vector<std::unique_ptr<spk::Entity3D>> lightEntities;

		Scene()
		{
			engine.addEntity(&cameraEntity);
		}

		~Scene()
		{
			for (auto &light : lightEntities)
			{
				engine.removeEntity(light.get());
			}
			engine.removeEntity(&cameraEntity);
		}

		spk::EngineView &addView(std::string name = "main")
		{
			return engine.addView(std::move(name), &camera, {64, 64});
		}

		spk::Light::Directional &addDirectional()
		{
			auto lightEntity = std::make_unique<spk::Entity3D>("directional");
			auto &light = lightEntity->addComponent<spk::Light::Directional>();
			engine.addEntity(lightEntity.get());
			lightEntities.push_back(std::move(lightEntity));
			return light;
		}
	};

	[[nodiscard]] spk::ShadowMap::Result::Entry makeMap(
		const spk::Light::Selection::Entry &light,
		std::uint64_t generation,
		std::uint64_t captureOrdinal)
	{
		spk::ShadowMap::Result::Entry result;
		result.registrationOrder = light.registrationOrder;
		result.admission = light.admission;
		result.generation = generation;
		result.captureOrdinal = captureOrdinal;
		result.light = light;
		spk::Texture texture;
		result.depthTexture = texture.handle();
		result.resolution = light.shadowResolution;
		return result;
	}

	struct CaptureProbe : spk::RenderingBehaviour
	{
		std::function<void(const spk::EngineView::CaptureContext &)> onCapture;

		void buildViewRenderSnapshot(
			spk::RenderSnapshot::Builder &,
			const spk::EngineView::CaptureContext &context) override
		{
			if (onCapture)
			{
				onCapture(context);
			}
		}
	};
}

TEST(GraphicsShadowResultCollection, TwoSurfacesAndRecreatedSurfaceWithSameNameNeverShareSlots)
{
	Scene scene;
	auto &view = scene.addView();
	scene.addDirectional();
	scene.engine.refreshRenderSnapshot();
	const auto selection = view.lightChannel().current();
	ASSERT_NE(selection, nullptr);
	ASSERT_EQ(selection->directional.size(), 1u);
	const auto light = selection->directional.front();

	HiddenFrame firstFrame;
	HiddenFrame secondFrame;
	spk::Window::Surface first("same-window-name");
	spk::Window::Surface second("same-window-name");
	first.create(firstFrame.native.window());
	second.create(secondFrame.native.window());
	ASSERT_NE(first.identity(), second.identity());
	EXPECT_TRUE(first.identity().hasRFCVariant());
	EXPECT_EQ(first.identity().version(), 4u);
	EXPECT_FALSE(first.identity().isNull());
	EXPECT_EQ(view.lightChannel().shadowResultCollection().acquireLatest(first), nullptr);
	EXPECT_EQ(view.lightChannel().shadowResultCollection().acquireLatest(second), nullptr);

	ASSERT_TRUE(view.lightChannel().publishResult(first, makeMap(light, 1, selection->captureOrdinal)));
	auto firstResult = view.lightChannel().shadowResultCollection().acquireLatest(first);
	ASSERT_NE(firstResult, nullptr);
	ASSERT_EQ(firstResult->entries.size(), 1u);
	EXPECT_EQ(view.lightChannel().shadowResultCollection().acquireLatest(second), nullptr);

	ASSERT_TRUE(view.lightChannel().publishResult(second, makeMap(light, 2, selection->captureOrdinal)));
	auto secondResult = view.lightChannel().shadowResultCollection().acquireLatest(second);
	ASSERT_NE(secondResult, nullptr);
	EXPECT_EQ(secondResult->entries.at(light.registrationOrder).generation, 2u);
	EXPECT_EQ(firstResult->entries.at(light.registrationOrder).generation, 1u);

	first.destroy();
	EXPECT_EQ(view.lightChannel().shadowResultCollection().acquireLatest(first), nullptr);
	ASSERT_NE(view.lightChannel().shadowResultCollection().acquireLatest(second), nullptr);
	HiddenFrame replacementFrame;
	spk::Window::Surface replacement("same-window-name");
	replacement.create(replacementFrame.native.window());
	EXPECT_NE(replacement.identity(), first.identity());
	EXPECT_EQ(view.lightChannel().shadowResultCollection().acquireLatest(replacement), nullptr);
	EXPECT_FALSE(view.lightChannel().resolveResult(first, light, selection->captureOrdinal).has_value());
	EXPECT_TRUE(view.lightChannel().resolveResult(second, light, selection->captureOrdinal).has_value());
	EXPECT_EQ(firstResult->entries.at(light.registrationOrder).generation, 1u);
	replacement.destroy();
	second.destroy();
}

TEST(GraphicsShadowResultCollection, CopyMergePreservesOtherLightsAndReadersOfHistoricalValues)
{
	Scene scene;
	auto &view = scene.addView();
	scene.addDirectional();
	scene.addDirectional();
	scene.engine.refreshRenderSnapshot();
	const auto selection = view.lightChannel().current();
	ASSERT_NE(selection, nullptr);
	ASSERT_EQ(selection->directional.size(), 2u);
	const auto first = selection->directional[0];
	const auto second = selection->directional[1];

	HiddenFrame frame;
	spk::Window::Surface surface("merge");
	surface.create(frame.native.window());
	auto &channel = view.lightChannel();
	ASSERT_TRUE(channel.publishResult(surface, makeMap(first, 1, selection->captureOrdinal)));
	auto held = channel.shadowResultCollection().acquireLatest(surface);
	ASSERT_NE(held, nullptr);
	ASSERT_EQ(held->entries.size(), 1u);
	ASSERT_TRUE(channel.publishResult(surface, makeMap(second, 3, selection->captureOrdinal)));
	auto merged = channel.shadowResultCollection().acquireLatest(surface);
	ASSERT_NE(merged, nullptr);
	EXPECT_EQ(merged->entries.size(), 2u);
	EXPECT_EQ(held->entries.size(), 1u);
	EXPECT_EQ(merged->entries.at(first.registrationOrder).generation, 1u);
	EXPECT_EQ(merged->entries.at(second.registrationOrder).generation, 3u);

	EXPECT_FALSE(channel.publishResult(surface, makeMap(first, 1, selection->captureOrdinal)));
	EXPECT_FALSE(channel.publishResult(surface, makeMap(second, 2, selection->captureOrdinal)));
	EXPECT_EQ(channel.shadowResultCollection().acquireLatest(surface).get(), merged.get());
	ASSERT_TRUE(channel.publishResult(surface, makeMap(first, 2, selection->captureOrdinal)));
	auto newer = channel.shadowResultCollection().acquireLatest(surface);
	ASSERT_NE(newer, nullptr);
	EXPECT_EQ(newer->entries.at(second.registrationOrder).generation, 3u);
	EXPECT_EQ(merged->entries.at(first.registrationOrder).generation, 1u);
	EXPECT_EQ(newer->entries.at(first.registrationOrder).generation, 2u);
	surface.destroy();
}

TEST(GraphicsShadowResultCollection, FutureCaptureIsInvisibleToHistoricalSnapshotsAndRevocationBlocksPublication)
{
	Scene scene;
	auto &view = scene.addView();
	auto &component = scene.addDirectional();
	scene.engine.refreshRenderSnapshot();
	const auto selection = view.lightChannel().current();
	ASSERT_NE(selection, nullptr);
	ASSERT_EQ(selection->directional.size(), 1u);
	const auto light = selection->directional.front();

	HiddenFrame frame;
	spk::Window::Surface surface("historical");
	surface.create(frame.native.window());
	auto &channel = view.lightChannel();
	const auto futureOrdinal = selection->captureOrdinal + 1;
	ASSERT_TRUE(channel.publishResult(surface, makeMap(light, 1, futureOrdinal)));
	EXPECT_FALSE(channel.resolveResult(surface, light, selection->captureOrdinal).has_value());
	EXPECT_TRUE(channel.resolveResult(surface, light, futureOrdinal).has_value());

	component.setCastsShadows(false);
	EXPECT_FALSE(channel.resolveResult(surface, light, futureOrdinal).has_value());
	EXPECT_FALSE(channel.publishResult(surface, makeMap(light, 2, futureOrdinal)));
	const auto previous = channel.shadowResultCollection().acquireLatest(surface);
	ASSERT_NE(previous, nullptr);
	EXPECT_EQ(previous->entries.at(light.registrationOrder).generation, 1u);

	scene.engine.refreshRenderSnapshot();
	EXPECT_FALSE(channel.publishResult(surface, makeMap(light, 2, futureOrdinal)));
	EXPECT_EQ(channel.shadowResultCollection().acquireLatest(surface).get(), previous.get());
	surface.destroy();
}

TEST(GraphicsShadowResultCollection, ThrowingSurfaceCallbackDoesNotPreventEntryClosureAndOtherSurface)
{
	Scene scene;
	auto &view = scene.addView();
	scene.addDirectional();
	scene.engine.refreshRenderSnapshot();
	const auto selection = view.lightChannel().current();
	ASSERT_NE(selection, nullptr);
	const auto light = selection->directional.front();

	HiddenFrame firstFrame;
	HiddenFrame secondFrame;
	spk::Window::Surface first("throw-first");
	spk::Window::Surface second("still-ready");
	first.create(firstFrame.native.window());
	second.create(secondFrame.native.window());
	std::vector<int> callbacks;
	auto throwing = first.subscribeToDestruction([&] {
		callbacks.push_back(1);
		throw std::runtime_error("expected close failure");
	});
	ASSERT_TRUE(view.lightChannel().publishResult(first, makeMap(light, 1, selection->captureOrdinal)));
	ASSERT_TRUE(view.lightChannel().publishResult(second, makeMap(light, 1, selection->captureOrdinal)));
	auto next = first.subscribeToDestruction([&] {
		callbacks.push_back(2);
		EXPECT_EQ(view.lightChannel().shadowResultCollection().acquireLatest(first), nullptr);
	});
	EXPECT_THROW(first.destroy(), std::runtime_error);
	EXPECT_EQ(first.lifeCycle(), spk::Window::LifeCycle::Released);
	EXPECT_EQ(callbacks, (std::vector<int>{1, 2}));
	EXPECT_EQ(view.lightChannel().shadowResultCollection().acquireLatest(first), nullptr);
	EXPECT_FALSE(view.lightChannel().publishResult(first, makeMap(light, 2, selection->captureOrdinal)));
	EXPECT_NE(view.lightChannel().shadowResultCollection().acquireLatest(second), nullptr);
	EXPECT_NO_THROW(first.destroy());
	second.destroy();
}

TEST(GraphicsShadowResultCollection, RemovingViewClosesChannelWithoutClearingItsRetainedSlot)
{
	Scene scene;
	auto &view = scene.addView();
	scene.addDirectional();
	std::shared_ptr<spk::Light::Channel> captured;
	auto &probe = scene.engine.root().addBehaviour<CaptureProbe>();
	probe.onCapture = [&](const spk::EngineView::CaptureContext &context) {
		captured = context.lightChannel;
	};
	scene.engine.refreshRenderSnapshot();
	ASSERT_NE(captured, nullptr);
	const auto selected = view.lightChannel().current();
	ASSERT_NE(selected, nullptr);
	ASSERT_EQ(selected->directional.size(), 1u);
	HiddenFrame frame;
	spk::Window::Surface surface("view-retained");
	surface.create(frame.native.window());
	ASSERT_TRUE(captured->publishResult(surface, makeMap(selected->directional.front(), 1, selected->captureOrdinal)));
	auto held = captured->shadowResultCollection().acquireLatest(surface);
	ASSERT_NE(held, nullptr);

	std::weak_ptr<spk::Light::Channel> weak = captured;
	scene.engine.removeView("main");
	EXPECT_TRUE(captured->isClosed());
	EXPECT_FALSE(captured->publishResult(surface, makeMap(selected->directional.front(), 2, selected->captureOrdinal)));
	EXPECT_FALSE(captured->resolveResult(surface, selected->directional.front(), selected->captureOrdinal).has_value());
	EXPECT_EQ(captured->shadowResultCollection().acquireLatest(surface).get(), held.get());
	EXPECT_EQ(held->entries.size(), 1u);

	scene.engine.refreshRenderSnapshot();
	captured.reset();
	EXPECT_TRUE(weak.expired());
	EXPECT_EQ(held->entries.size(), 1u);
	surface.destroy();
}
