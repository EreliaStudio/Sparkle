#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>

#include "engine/entity2d.hpp"
#include "engine/reference_frame.hpp"
#include "engine/transform2d.hpp"
#include "math/vector4.hpp"

namespace
{
	constexpr float TransformTolerance = 2.0e-4f;

	void expectVectorNear(const spk::Vector2 &actual, const spk::Vector2 &expected)
	{
		EXPECT_NEAR(actual.x, expected.x, TransformTolerance);
		EXPECT_NEAR(actual.y, expected.y, TransformTolerance);
	}

	void expectMatrixNear(const spk::Matrix4x4 &actual, const spk::Matrix4x4 &expected)
	{
		for (std::size_t column = 0; column < 4; ++column)
		{
			for (std::size_t row = 0; row < 4; ++row)
			{
				EXPECT_NEAR(actual[column][row], expected[column][row], TransformTolerance);
			}
		}
	}
}

TEST(Transform2DTest, IdentityDefaultsAndAllLocalMutatorsAreObservable)
{
	spk::Entity2D entity("entity");
	auto &transform = entity.transform();

	EXPECT_EQ(transform.position(), spk::Vector2(0.0f, 0.0f));
	EXPECT_EQ(transform.scale(), spk::Vector2(1.0f, 1.0f));
	EXPECT_FLOAT_EQ(transform.rotation(), 0.0f);
	EXPECT_FLOAT_EQ(transform.depth(), 0.0f);
	expectMatrixNear(transform.localModelMatrix(), spk::Matrix4x4::identity());
	expectMatrixNear(transform.inverseLocalModelMatrix(), spk::Matrix4x4::identity());

	transform.place({2.0f, 3.0f});
	transform.move({-1.0f, 4.0f});
	transform.rescale({-2.0f, 0.5f});
	transform.setRotation(20.0f);
	transform.rotate(10.0f);
	transform.setDepth(-4.0f);
	transform.elevate(1.5f);

	EXPECT_EQ(transform.position(), spk::Vector2(1.0f, 7.0f));
	EXPECT_EQ(transform.scale(), spk::Vector2(-2.0f, 0.5f));
	EXPECT_FLOAT_EQ(transform.rotation(), 30.0f);
	EXPECT_FLOAT_EQ(transform.depth(), -2.5f);
}

TEST(Transform2DTest, ParentCompositionIncludesTranslationScaleRotationAndDepth)
{
	spk::Entity2D parent("parent");
	spk::Entity2D child("child", &parent);
	parent.transform().place({10.0f, 20.0f});
	parent.transform().rescale({2.0f, 3.0f});
	parent.transform().setRotation(90.0f);
	parent.transform().setDepth(4.0f);
	child.transform().place({1.0f, 2.0f});
	child.transform().rescale({4.0f, -2.0f});
	child.transform().setRotation(-15.0f);
	child.transform().setDepth(-1.5f);

	const spk::Vector4 expectedPosition = parent.transform().modelMatrix() * spk::Vector4{1.0f, 2.0f, 0.0f, 1.0f};
	expectVectorNear(child.transform().position(spk::ReferenceFrame::World), {expectedPosition.x, expectedPosition.y});
	expectVectorNear(child.transform().scale(spk::ReferenceFrame::World), {8.0f, -6.0f});
	EXPECT_NEAR(child.transform().rotation(spk::ReferenceFrame::World), 75.0f, TransformTolerance);
	EXPECT_NEAR(child.transform().depth(spk::ReferenceFrame::World), 2.5f, TransformTolerance);
	expectMatrixNear(child.transform().modelMatrix(), parent.transform().modelMatrix() * child.transform().localModelMatrix());
}

TEST(Transform2DTest, ModelAndInverseMatricesRoundTripPoints)
{
	spk::Entity2D parent("parent");
	spk::Entity2D child("child", &parent);
	parent.transform().place({3.0f, -5.0f});
	parent.transform().rescale({2.0f, 0.75f});
	parent.transform().setRotation(17.0f);
	child.transform().place({-4.0f, 8.0f});
	child.transform().rescale({-1.5f, 3.0f});
	child.transform().setRotation(-31.0f);

	const spk::Vector4 point{7.0f, -2.0f, 0.0f, 1.0f};
	const spk::Vector4 world = child.transform().modelMatrix() * point;
	const spk::Vector4 roundTrip = child.transform().inverseModelMatrix() * world;
	EXPECT_NEAR(roundTrip.x, point.x, TransformTolerance);
	EXPECT_NEAR(roundTrip.y, point.y, TransformTolerance);
	EXPECT_NEAR(roundTrip.z, point.z, TransformTolerance);
	EXPECT_NEAR(roundTrip.w, point.w, TransformTolerance);
}

TEST(Transform2DTest, ParentEditsAndReparentingInvalidateDescendantWorldCaches)
{
	spk::Entity2D first("first");
	spk::Entity2D second("second");
	spk::Entity2D child("child", &first);
	spk::Entity2D grandchild("grandchild", &child);
	child.transform().place({2.0f, 0.0f});
	grandchild.transform().place({3.0f, 0.0f});

	EXPECT_EQ(grandchild.transform().position(spk::ReferenceFrame::World), spk::Vector2(5.0f, 0.0f));
	first.transform().place({10.0f, 0.0f});
	EXPECT_EQ(grandchild.transform().position(spk::ReferenceFrame::World), spk::Vector2(15.0f, 0.0f));
	second.transform().place({-10.0f, 0.0f});
	child.setParent(&second);
	EXPECT_EQ(grandchild.transform().position(spk::ReferenceFrame::World), spk::Vector2(-5.0f, 0.0f));
}

TEST(Transform2DTest, EditionCallbackFiresOncePerEffectiveMutation)
{
	spk::Entity2D entity("entity");
	auto &transform = entity.transform();
	std::size_t calls = 0;
	const spk::Transform2D *observed = nullptr;
	auto contract = transform.subscribeToEdition([&](const spk::Transform2D &edited) {
		++calls;
		observed = &edited;
	});

	transform.place({0.0f, 0.0f});
	transform.rescale({1.0f, 1.0f});
	transform.setRotation(0.0f);
	transform.setDepth(0.0f);
	EXPECT_EQ(calls, 0u);

	transform.move({1.0f, 2.0f});
	transform.rescale({-1.0f, 2.0f});
	transform.rotate(-45.0f);
	transform.elevate(-3.0f);
	EXPECT_EQ(calls, 4u);
	EXPECT_EQ(observed, &transform);
}

TEST(Transform2DTest, EveryAccessorRejectsAnInvalidReferenceFrame)
{
	spk::Entity2D entity("entity");
	const auto invalid = static_cast<spk::ReferenceFrame>(255);

	EXPECT_THROW((void)entity.transform().position(invalid), std::runtime_error);
	EXPECT_THROW((void)entity.transform().scale(invalid), std::runtime_error);
	EXPECT_THROW((void)entity.transform().rotation(invalid), std::runtime_error);
	EXPECT_THROW((void)entity.transform().depth(invalid), std::runtime_error);
}

TEST(Transform2DTest, ZeroScaledAxisRejectsInverseButLeavesOtherCachedValuesUsable)
{
	spk::Entity2D entity("entity");
	entity.transform().place({2.0f, 3.0f});
	entity.transform().rescale({0.0f, -2.0f});

	EXPECT_THROW((void)entity.transform().inverseLocalModelMatrix(), std::runtime_error);
	EXPECT_THROW((void)entity.transform().inverseModelMatrix(), std::runtime_error);
	EXPECT_EQ(entity.transform().position(), spk::Vector2(2.0f, 3.0f));
	EXPECT_EQ(entity.transform().scale(), spk::Vector2(0.0f, -2.0f));
	EXPECT_NO_THROW((void)entity.transform().localModelMatrix());
	EXPECT_NO_THROW((void)entity.transform().modelMatrix());
}
