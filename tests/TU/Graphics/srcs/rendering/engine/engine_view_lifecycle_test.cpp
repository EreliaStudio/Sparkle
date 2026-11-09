#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

#include "engine_view_test_utils.hpp"
#include "exception.hpp"
#include "rendering/engine/engine_view.hpp"

namespace
{
	using Fixture = sparkle_test::engine_view::Fixture;
}

TEST(EngineViewLifecycleTest, NoViewIsImplicitlyCreated)
{
	Fixture fixture;
	EXPECT_THROW(fixture.engine.view("main"), spk::Exception);
	fixture.engine.refreshRenderSnapshot();
	ASSERT_NE(fixture.engine.latestRenderSnapshot(), nullptr);
	EXPECT_NO_THROW(fixture.executeLatest());
}

TEST(EngineViewLifecycleTest, NamedCreationAndLookupReturnSameObject)
{
	Fixture fixture;
	auto &view = fixture.addView("monitor");
	EXPECT_EQ(&fixture.engine.view("monitor"), &view);
	EXPECT_EQ(&std::as_const(fixture.engine).view("monitor"), &view);
	EXPECT_EQ(view.name(), "monitor");
}

TEST(EngineViewLifecycleTest, NameCannotBeModified)
{
	static_assert(!std::is_copy_constructible_v<spk::EngineView>);
	static_assert(!std::is_move_constructible_v<spk::EngineView>);
	static_assert(std::is_same_v<decltype(std::declval<spk::EngineView &>().name()), const std::string &>);
	Fixture fixture;
	const auto &view = fixture.addView("fixed");
	EXPECT_EQ(view.name(), "fixed");
}

TEST(EngineViewLifecycleTest, DuplicateNamePreservesOriginalView)
{
	Fixture fixture;
	auto &view = fixture.addView("monitor");
	EXPECT_THROW(fixture.addView("monitor"), spk::Exception);
	EXPECT_EQ(&fixture.engine.view("monitor"), &view);
}

TEST(EngineViewLifecycleTest, UnknownViewAndRemovalThrow)
{
	Fixture fixture;
	EXPECT_THROW(fixture.engine.view("missing"), spk::Exception);
	EXPECT_THROW(fixture.engine.removeView("missing"), spk::Exception);
}

TEST(EngineViewLifecycleTest, RemoveFreesNameWithoutReusingIdentity)
{
	Fixture fixture;
	const auto firstIdentity = fixture.addView("monitor").identity();
	fixture.engine.removeView("monitor");
	EXPECT_THROW(fixture.engine.view("monitor"), spk::Exception);
	EXPECT_NE(fixture.addView("monitor").identity(), firstIdentity);
}

TEST(EngineViewLifecycleTest, ResolutionIsExplicitAndIndependentOfWindowGeometry)
{
	Fixture fixture;
	auto &view = fixture.addView("monitor");
	EXPECT_EQ(view.resolution(), spk::Vector2UInt(128, 96));
	fixture.engine.handleGeometryChange({.anchor = {0, 0}, .size = {800, 600}});
	EXPECT_EQ(view.resolution(), spk::Vector2UInt(128, 96));
	view.setResolution({640, 360});
	EXPECT_EQ(view.resolution(), spk::Vector2UInt(640, 360));
}

TEST(EngineViewLifecycleTest, InvalidResolutionHasStrongExceptionGuarantee)
{
	Fixture fixture;
	auto &view = fixture.addView("monitor");
	EXPECT_THROW(view.setResolution({0, 50}), spk::Exception);
	EXPECT_THROW(view.setResolution({50, 0}), spk::Exception);
	EXPECT_EQ(view.resolution(), spk::Vector2UInt(128, 96));
	EXPECT_THROW(fixture.engine.addView("bad", &fixture.camera, {0, 0}), spk::Exception);
	EXPECT_THROW(fixture.engine.view("bad"), spk::Exception);
}

TEST(EngineViewLifecycleTest, CameraCanBeDetachedAndReattached)
{
	Fixture fixture;
	auto &view = fixture.addView("monitor");
	EXPECT_EQ(view.camera(), &fixture.camera);
	view.detachCamera();
	EXPECT_EQ(view.camera(), nullptr);
	EXPECT_EQ(&fixture.engine.view("monitor"), &view);
	view.setCamera(&fixture.camera);
	EXPECT_EQ(view.camera(), &fixture.camera);
}

TEST(EngineViewLifecycleTest, TwoViewsMayShareOneCamera)
{
	Fixture fixture;
	auto &first = fixture.addView("first");
	auto &second = fixture.addView("second");
	EXPECT_EQ(first.camera(), second.camera());
	first.detachCamera();
	EXPECT_EQ(second.camera(), &fixture.camera);
}

TEST(EngineViewLifecycleTest, NewViewMayStartWithoutCamera)
{
	Fixture fixture;
	auto &view = fixture.engine.addView("detached", nullptr, {128, 96});
	EXPECT_EQ(view.camera(), nullptr);
	fixture.engine.refreshRenderSnapshot();
	EXPECT_NO_THROW(fixture.executeLatest());
	EXPECT_EQ(&fixture.engine.view("detached"), &view);
}

TEST(EngineViewLifecycleTest, DetachedOutputPolicyDefaultsToInvalid)
{
	Fixture fixture;
	auto &view = fixture.addView("monitor");
	EXPECT_EQ(view.detachedOutputPolicy(), spk::EngineView::DetachedOutputPolicy::Invalid);
	view.setDetachedOutputPolicy(spk::EngineView::DetachedOutputPolicy::KeepLastFrame);
	EXPECT_EQ(view.detachedOutputPolicy(), spk::EngineView::DetachedOutputPolicy::KeepLastFrame);
	view.setDetachedOutputPolicy(spk::EngineView::DetachedOutputPolicy::DefaultTexture);
	EXPECT_EQ(view.detachedOutputPolicy(), spk::EngineView::DetachedOutputPolicy::DefaultTexture);
	EXPECT_FALSE(view.fallbackTexture().valid());
}

TEST(EngineViewLifecycleTest, InvalidDetachedOutputPolicyPreservesPrevious)
{
	Fixture fixture;
	auto &view = fixture.addView("monitor");
	EXPECT_THROW(view.setDetachedOutputPolicy(static_cast<spk::EngineView::DetachedOutputPolicy>(99)), spk::Exception);
	EXPECT_EQ(view.detachedOutputPolicy(), spk::EngineView::DetachedOutputPolicy::Invalid);
}

TEST(EngineViewLifecycleTest, DefaultViewAppearanceAndCaptureParameters)
{
	Fixture fixture;
	auto &view = fixture.addView("monitor");
	EXPECT_EQ(view.backgroundColor(), (spk::RGB{0, 0, 0}));
	EXPECT_FLOAT_EQ(view.exposure(), 1.0f);
	EXPECT_FLOAT_EQ(view.targetFPS(), 60.0f);
}

TEST(EngineViewLifecycleTest, AppearanceParameterSettersValidateWithoutPartialMutation)
{
	Fixture fixture;
	auto &view = fixture.addView("monitor");
	view.setBackgroundColor({0.2f, 0.3f, 0.4f});
	view.setExposure(2.0f);
	view.setTargetFPS(120.0f);
	EXPECT_EQ(view.backgroundColor(), (spk::RGB{0.2f, 0.3f, 0.4f}));
	EXPECT_THROW(view.setBackgroundColor({1.1f, 0.1f, 0.1f}), spk::Exception);
	EXPECT_THROW(view.setExposure(0), spk::Exception);
	EXPECT_THROW(view.setExposure(std::numeric_limits<float>::infinity()), spk::Exception);
	EXPECT_THROW(view.setTargetFPS(-1.0f), spk::Exception);
	EXPECT_THROW(view.setTargetFPS(std::numeric_limits<float>::quiet_NaN()), spk::Exception);
	EXPECT_EQ(view.backgroundColor(), (spk::RGB{0.2f, 0.3f, 0.4f}));
	EXPECT_FLOAT_EQ(view.exposure(), 2.0f);
	EXPECT_FLOAT_EQ(view.targetFPS(), 120.0f);
}

TEST(EngineViewLifecycleTest, CameraCanRetrieveOnlyViewsThatReferenceIt)
{
	Fixture fixture;
	auto &view = fixture.addView("monitor");
	EXPECT_EQ(&fixture.camera.view("monitor"), &view);
	EXPECT_EQ(&std::as_const(fixture.camera).view("monitor"), &view);
	view.detachCamera();
	EXPECT_THROW(fixture.camera.view("monitor"), spk::Exception);
	view.setCamera(&fixture.camera);
	EXPECT_EQ(&fixture.camera.view("monitor"), &view);
}

TEST(EngineViewLifecycleTest, CameraLookupRejectsUnknownOrWrongCamera)
{
	Fixture fixture;
	fixture.addView("monitor");
	spk::Entity3D otherEntity("other");
	auto &otherCamera = otherEntity.addComponent<spk::CameraComponent3D>();
	fixture.engine.addEntity(&otherEntity);
	EXPECT_THROW(fixture.camera.view("missing"), spk::Exception);
	EXPECT_THROW(otherCamera.view("monitor"), spk::Exception);
	fixture.engine.removeEntity(&otherEntity);
}

TEST(EngineViewLifecycleTest, UnattachedCameraCannotResolveView)
{
	spk::CameraComponent3D camera;
	EXPECT_THROW(camera.view("missing"), spk::Exception);
}
