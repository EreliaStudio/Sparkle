#include <gtest/gtest.h>

#include <barrier>
#include <cmath>
#include <functional>
#include <future>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "exception.hpp"
#include "light/channel.hpp"
#include "light/directional.hpp"
#include "light/point.hpp"
#include "light/spot.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/engine/rendering_behaviour.hpp"

namespace
{
	struct SelectionProbe : spk::RenderingBehaviour
	{
		std::function<void(const spk::EngineView::CaptureContext &)> callback;

		void buildViewRenderSnapshot(spk::RenderSnapshot::Builder &, const spk::EngineView::CaptureContext &context) override
		{
			if (callback)
			{
				callback(context);
			}
		}
	};

	struct Scene
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"camera"};
		spk::CameraComponent3D &camera;
		std::vector<std::unique_ptr<spk::Entity3D>> lights;

		Scene() :
			camera(cameraEntity.addComponent<spk::CameraComponent3D>())
		{
			engine.addEntity(&cameraEntity);
		}

		~Scene()
		{
			for (auto &entity : lights)
			{
				engine.removeEntity(entity.get());
			}
			engine.removeEntity(&cameraEntity);
		}

		spk::EngineView &addView(std::string name)
		{
			return engine.addView(std::move(name), &camera, {64, 64});
		}

		template <typename TLight>
		TLight &addLight(float intensity = 1.0f)
		{
			auto entity = std::make_unique<spk::Entity3D>("light");
			entity->transform().place({0.0f, 0.0f, -5.0f});
			TLight &light = entity->addComponent<TLight>();
			light.setIntensity(intensity);
			engine.addEntity(entity.get());
			lights.push_back(std::move(entity));
			return light;
		}
	};

	bool hasOrdinal(const std::vector<spk::Light::Selection::Entry> &entries, std::uint64_t ordinal)
	{
		for (const auto &entry : entries)
		{
			if (entry.registrationOrder == ordinal)
			{
				return true;
			}
		}
		return false;
	}
}

TEST(GraphicsLightChannel, DefaultsAndDirectionalOutsideLocalQuota)
{
	Scene scene;
	auto &view = scene.addView("main");
	EXPECT_EQ(view.lightChannel().limit(), 8u);
	EXPECT_FLOAT_EQ(view.lightChannel().hysteresis(), 0.10f);
	for (int i = 0; i < 9; ++i)
	{
		if (i % 2 == 0)
		{
			scene.addLight<spk::Light::Point>();
		}
		else
		{
			scene.addLight<spk::Light::Spot>();
		}
	}
	const auto &sun = scene.addLight<spk::Light::Directional>();
	scene.engine.refreshRenderSnapshot();
	auto first = view.lightChannel().current();
	ASSERT_NE(first, nullptr);
	ASSERT_EQ(first->local.size(), 8u);
	for (std::uint64_t ordinal = 1; ordinal <= 8; ++ordinal)
	{
		EXPECT_EQ(first->local[ordinal - 1].registrationOrder, ordinal);
	}
	ASSERT_EQ(first->directional.size(), 1u);
	EXPECT_EQ(first->directional.front().registrationOrder, sun.registrationOrder());

	view.lightChannel().setLimit(0);
	scene.engine.refreshRenderSnapshot();
	auto zero = view.lightChannel().current();
	ASSERT_NE(zero, nullptr);
	EXPECT_TRUE(zero->local.empty());
	EXPECT_EQ(zero->directional.size(), 1u);
	EXPECT_EQ(first->local.size(), 8u);
}

TEST(GraphicsLightChannel, IncumbentsSurviveUntilHysteresisThresholdOrReset)
{
	Scene scene;
	auto &view = scene.addView("main");
	view.lightChannel().setLimit(1);
	view.lightChannel().setHysteresis(0.5f);
	const auto &first = scene.addLight<spk::Light::Point>(10.0f);
	scene.engine.refreshRenderSnapshot();
	const auto initial = view.lightChannel().current();
	ASSERT_NE(initial, nullptr);
	ASSERT_EQ(initial->local.size(), 1u);
	EXPECT_EQ(initial->local[0].registrationOrder, first.registrationOrder());
	const auto &second = scene.addLight<spk::Light::Spot>(11.0f);
	scene.engine.refreshRenderSnapshot();
	auto protectedSelection = view.lightChannel().current();
	ASSERT_NE(protectedSelection, nullptr);
	EXPECT_EQ(protectedSelection->local[0].registrationOrder, first.registrationOrder());

	const auto unchangedGeneration = view.lightChannel().generation();
	view.lightChannel().setHysteresis(0.5f);
	EXPECT_EQ(view.lightChannel().generation(), unchangedGeneration);
	view.lightChannel().setHysteresis(0.1f);
	scene.engine.refreshRenderSnapshot();
	auto reset = view.lightChannel().current();
	ASSERT_NE(reset, nullptr);
	EXPECT_EQ(reset->local[0].registrationOrder, second.registrationOrder());
	EXPECT_NE(reset.get(), protectedSelection.get());
}

TEST(GraphicsLightChannel, InvalidationIncludesUnselectedLightsAndPublishesImmutableValues)
{
	Scene scene;
	auto &view = scene.addView("main");
	auto &other = scene.addView("other");
	view.lightChannel().setLimit(1);
	other.lightChannel().setLimit(1);
	auto &first = scene.addLight<spk::Light::Point>(10.0f);
	auto &second = scene.addLight<spk::Light::Point>(1.0f);
	scene.engine.refreshRenderSnapshot();
	const auto historic = view.lightChannel().current();
	ASSERT_NE(historic, nullptr);
	ASSERT_EQ(historic->local.size(), 1u);
	EXPECT_EQ(historic->local[0].registrationOrder, first.registrationOrder());
	const auto beforeMain = view.lightChannel().generation();
	const auto beforeOther = other.lightChannel().generation();
	second.setIntensity(2.0f);
	EXPECT_GT(view.lightChannel().generation(), beforeMain);
	EXPECT_GT(other.lightChannel().generation(), beforeOther);
	EXPECT_EQ(historic->local[0].intensity, 10.0f);

	first.setIntensity(3.0f);
	scene.engine.refreshRenderSnapshot();
	const auto latest = view.lightChannel().current();
	ASSERT_NE(latest, nullptr);
	EXPECT_EQ(latest->local[0].intensity, 3.0f);
	EXPECT_EQ(historic->local[0].intensity, 10.0f);

	const auto beforeInvalid = view.lightChannel().generation();
	const float priorH = view.lightChannel().hysteresis();
	EXPECT_THROW(view.lightChannel().setHysteresis(-1.0f), spk::Exception);
	EXPECT_EQ(view.lightChannel().hysteresis(), priorH);
	EXPECT_EQ(view.lightChannel().generation(), beforeInvalid);
}

TEST(GraphicsLightChannel, ParentTransformsAndCameraProjectionInvalidateAppropriateChannels)
{
	Scene scene;
	spk::Entity3D secondCameraEntity{"other-camera"};
	auto &secondCamera = secondCameraEntity.addComponent<spk::CameraComponent3D>();
	scene.engine.addEntity(&secondCameraEntity);
	auto &one = scene.addView("one");
	auto &two = scene.addView("two");
	auto &three = scene.engine.addView("three", &secondCamera, {64, 64});
	auto &light = scene.addLight<spk::Light::Point>();
	scene.engine.refreshRenderSnapshot();
	const auto generationOne = one.lightChannel().generation();
	const auto generationTwo = two.lightChannel().generation();
	const auto generationThree = three.lightChannel().generation();

	scene.camera.setPerspective(1.2f, 1.0f, 0.1f, 100.0f);
	EXPECT_GT(one.lightChannel().generation(), generationOne);
	EXPECT_GT(two.lightChannel().generation(), generationTwo);
	EXPECT_EQ(three.lightChannel().generation(), generationThree);

	const auto beforeLightOne = one.lightChannel().generation();
	const auto beforeLightThree = three.lightChannel().generation();
	scene.lights.front()->transform().move({1.0f, 0.0f, 0.0f});
	EXPECT_GT(one.lightChannel().generation(), beforeLightOne);
	EXPECT_GT(three.lightChannel().generation(), beforeLightThree);
	EXPECT_NE(light.worldPosition().x, 0.0f);

	const auto previousThree = three.lightChannel().generation();
	scene.cameraEntity.transform().move({0.0f, 0.0f, 1.0f});
	EXPECT_EQ(three.lightChannel().generation(), previousThree);
	scene.engine.removeEntity(&secondCameraEntity);
}

TEST(GraphicsLightChannel, PreviousSnapshotOwnsSelectionAndChannelAfterViewRemoval)
{
	Scene scene;
	auto &view = scene.addView("main");
	scene.addLight<spk::Light::Directional>();
	std::shared_ptr<spk::Light::Channel> capturedChannel;
	std::shared_ptr<const spk::Light::Selection> capturedSelection;
	auto &observer = scene.engine.root().addBehaviour<SelectionProbe>();
	observer.callback = [&](const spk::EngineView::CaptureContext &context) {
		capturedChannel = context.lightChannel;
		capturedSelection = context.lightSelection;
	};
	scene.engine.refreshRenderSnapshot();
	const auto historical = scene.engine.latestRenderSnapshot();
	ASSERT_NE(historical, nullptr);
	const auto retained = view.lightChannel().current();
	ASSERT_NE(retained, nullptr);
	ASSERT_EQ(capturedChannel.get(), &view.lightChannel());
	ASSERT_EQ(capturedSelection.get(), retained.get());
	scene.engine.removeView("main");
	EXPECT_TRUE(capturedChannel->isClosed());
	EXPECT_EQ(retained->directional.size(), 1u);
	EXPECT_EQ(capturedSelection->directional.size(), 1u);
	EXPECT_THROW(capturedChannel->acquire(scene.camera.captureCamera(), 100), spk::Exception);
	EXPECT_NE(scene.engine.latestRenderSnapshot(), nullptr);
	scene.engine.refreshRenderSnapshot();
	EXPECT_NE(scene.engine.latestRenderSnapshot(), nullptr);
}

TEST(GraphicsLightChannel, ConcurrentReadersShareOneCurrentSelection)
{
	Scene scene;
	auto &view = scene.addView("main");
	scene.addLight<spk::Light::Point>();
	const spk::Camera3D camera = scene.camera.captureCamera();
	std::barrier start(3);
	auto consumer = [&] {
		start.arrive_and_wait();
		return view.lightChannel().acquire(camera, 42);
	};
	auto left = std::async(std::launch::async, consumer);
	auto right = std::async(std::launch::async, consumer);
	start.arrive_and_wait();
	const auto a = left.get();
	const auto b = right.get();
	ASSERT_NE(a, nullptr);
	EXPECT_EQ(a.get(), b.get());
	EXPECT_EQ(view.lightChannel().current().get(), a.get());
}

TEST(GraphicsLightChannel, FailedReconstructionDoesNotPoisonChannel)
{
	Scene scene;
	auto &view = scene.addView("main");
	scene.addLight<spk::Light::Point>();
	const auto capturedCamera = scene.camera.captureCamera();
	scene.lights.front()->transform().place({std::numeric_limits<float>::quiet_NaN(), 0.0f, -5.0f});
	EXPECT_THROW(view.lightChannel().acquire(capturedCamera, 5), spk::Exception);
	scene.lights.front()->transform().place({0.0f, 0.0f, -5.0f});
	EXPECT_NO_THROW(static_cast<void>(view.lightChannel().acquire(capturedCamera, 6)));
}
