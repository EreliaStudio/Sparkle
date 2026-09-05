#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>

#include "engine/entity3d.hpp"
#include "engine/reference_frame.hpp"
#include "engine/transform3d.hpp"
#include "math/vector4.hpp"

namespace
{
	constexpr float TransformTolerance = 3.0e-4f;

	void expectVectorNear(const spk::Vector3 &actual, const spk::Vector3 &expected)
	{
		EXPECT_NEAR(actual.x, expected.x, TransformTolerance);
		EXPECT_NEAR(actual.y, expected.y, TransformTolerance);
		EXPECT_NEAR(actual.z, expected.z, TransformTolerance);
	}

	void expectQuaternionEquivalent(const spk::Quaternion &actual, const spk::Quaternion &expected)
	{
		EXPECT_NEAR(std::abs(actual.dot(expected)), 1.0f, TransformTolerance);
	}
}

TEST(Transform3DTest, IdentityDefaultsAndPositionScaleMutatorsAreObservable)
{
	spk::Entity3D entity("entity");
	auto &transform = entity.transform();

	EXPECT_EQ(transform.position(), spk::Vector3(0.0f, 0.0f, 0.0f));
	EXPECT_EQ(transform.scale(), spk::Vector3(1.0f, 1.0f, 1.0f));
	EXPECT_EQ(transform.rotation(), spk::Quaternion::identity());

	transform.place({1.0f, 2.0f, 3.0f});
	transform.move({-4.0f, 5.0f, -6.0f});
	transform.rescale({-2.0f, 0.5f, 4.0f});
	EXPECT_EQ(transform.position(), spk::Vector3(-3.0f, 7.0f, -3.0f));
	EXPECT_EQ(transform.scale(), spk::Vector3(-2.0f, 0.5f, 4.0f));
}

TEST(Transform3DTest, QuaternionEulerAndAxisAngleOverloadsComposeRotations)
{
	spk::Entity3D entity("entity");
	auto &transform = entity.transform();
	const spk::Quaternion initial = spk::Quaternion::fromEuler({10.0f, 20.0f, 30.0f});
	transform.setRotation({initial.x * 4.0f, initial.y * 4.0f, initial.z * 4.0f, initial.w * 4.0f});
	expectQuaternionEquivalent(transform.rotation(), initial);

	transform.setEulerRotation({15.0f, -25.0f, 35.0f});
	expectQuaternionEquivalent(transform.rotation(), spk::Quaternion::fromEuler({15.0f, -25.0f, 35.0f}));
	expectVectorNear(transform.eulerRotation(), transform.rotation().toEuler());

	const auto before = transform.rotation();
	transform.rotate(spk::Quaternion::fromAxisAngle({0.0f, 1.0f, 0.0f}, 12.0f));
	expectQuaternionEquivalent(transform.rotation(), before * spk::Quaternion::fromAxisAngle({0.0f, 1.0f, 0.0f}, 12.0f));
	transform.rotate(spk::Vector3{2.0f, 3.0f, 4.0f});
	transform.rotate(spk::Vector3{1.0f, 0.0f, 0.0f}, -7.0f);
	EXPECT_NEAR(transform.rotation().dot(transform.rotation()), 1.0f, TransformTolerance);
}

TEST(Transform3DTest, ParentCompositionProducesWorldValuesAndMatrices)
{
	spk::Entity3D parent("parent");
	spk::Entity3D child("child", &parent);
	parent.transform().place({10.0f, 20.0f, 30.0f});
	parent.transform().rescale({2.0f, 3.0f, 4.0f});
	parent.transform().setEulerRotation({0.0f, 0.0f, 90.0f});
	child.transform().place({1.0f, 2.0f, 3.0f});
	child.transform().rescale({-1.0f, 0.5f, 2.0f});
	child.transform().setEulerRotation({5.0f, 10.0f, 15.0f});

	const spk::Vector4 expected = parent.transform().modelMatrix() * spk::Vector4{1.0f, 2.0f, 3.0f, 1.0f};
	expectVectorNear(child.transform().position(spk::ReferenceFrame::World), {expected.x, expected.y, expected.z});
	expectVectorNear(child.transform().scale(spk::ReferenceFrame::World), {-2.0f, 1.5f, 8.0f});
	expectQuaternionEquivalent(
		child.transform().rotation(spk::ReferenceFrame::World),
		(parent.transform().rotation() * child.transform().rotation()).normalized());
}

TEST(Transform3DTest, ModelAndInverseMatricesRoundTripPointsWithNegativeScale)
{
	spk::Entity3D parent("parent");
	spk::Entity3D child("child", &parent);
	parent.transform().place({3.0f, -2.0f, 8.0f});
	parent.transform().setEulerRotation({12.0f, 23.0f, -18.0f});
	child.transform().place({-4.0f, 5.0f, 6.0f});
	child.transform().rescale({-2.0f, 0.75f, 3.0f});
	child.transform().setEulerRotation({-9.0f, 7.0f, 31.0f});

	const spk::Vector4 point{2.0f, -3.0f, 4.0f, 1.0f};
	const spk::Vector4 world = child.transform().modelMatrix() * point;
	const spk::Vector4 roundTrip = child.transform().inverseModelMatrix() * world;
	EXPECT_NEAR(roundTrip.x, point.x, TransformTolerance);
	EXPECT_NEAR(roundTrip.y, point.y, TransformTolerance);
	EXPECT_NEAR(roundTrip.z, point.z, TransformTolerance);
	EXPECT_NEAR(roundTrip.w, point.w, TransformTolerance);
}

TEST(Transform3DTest, ParentEditsAndReparentingInvalidateRecursiveWorldCaches)
{
	spk::Entity3D first("first");
	spk::Entity3D second("second");
	spk::Entity3D child("child", &first);
	spk::Entity3D grandchild("grandchild", &child);
	child.transform().place({2.0f, 0.0f, 0.0f});
	grandchild.transform().place({3.0f, 0.0f, 0.0f});
	EXPECT_EQ(grandchild.transform().position(spk::ReferenceFrame::World), spk::Vector3(5.0f, 0.0f, 0.0f));

	first.transform().place({10.0f, 0.0f, 0.0f});
	EXPECT_EQ(grandchild.transform().position(spk::ReferenceFrame::World), spk::Vector3(15.0f, 0.0f, 0.0f));
	second.transform().place({-10.0f, 0.0f, 0.0f});
	child.setParent(&second);
	EXPECT_EQ(grandchild.transform().position(spk::ReferenceFrame::World), spk::Vector3(-5.0f, 0.0f, 0.0f));
}

TEST(Transform3DTest, EditionCallbackFiresOncePerEffectiveMutation)
{
	spk::Entity3D entity("entity");
	auto &transform = entity.transform();
	std::size_t calls = 0;
	auto contract = transform.subscribeToEdition([&](const spk::Transform3D &edited) {
		EXPECT_EQ(&edited, &transform);
		++calls;
	});

	transform.place({0.0f, 0.0f, 0.0f});
	transform.rescale({1.0f, 1.0f, 1.0f});
	transform.setRotation(spk::Quaternion::identity());
	EXPECT_EQ(calls, 0u);
	transform.move({1.0f, 2.0f, 3.0f});
	transform.rescale({-1.0f, 2.0f, 3.0f});
	transform.rotate({1.0f, 2.0f, 3.0f});
	EXPECT_EQ(calls, 3u);
}

TEST(Transform3DTest, EveryAccessorRejectsAnInvalidReferenceFrame)
{
	spk::Entity3D entity("entity");
	const auto invalid = static_cast<spk::ReferenceFrame>(255);

	EXPECT_THROW((void)entity.transform().position(invalid), std::runtime_error);
	EXPECT_THROW((void)entity.transform().scale(invalid), std::runtime_error);
	EXPECT_THROW((void)entity.transform().rotation(invalid), std::runtime_error);
	EXPECT_THROW((void)entity.transform().eulerRotation(invalid), std::runtime_error);
}

TEST(Transform3DTest, ZeroScaledAxisRejectsInverseButLeavesOtherCachedValuesUsable)
{
	spk::Entity3D entity("entity");
	entity.transform().place({2.0f, 3.0f, 4.0f});
	entity.transform().rescale({1.0f, 0.0f, -2.0f});

	EXPECT_THROW((void)entity.transform().inverseLocalModelMatrix(), std::runtime_error);
	EXPECT_THROW((void)entity.transform().inverseModelMatrix(), std::runtime_error);
	EXPECT_EQ(entity.transform().position(), spk::Vector3(2.0f, 3.0f, 4.0f));
	EXPECT_EQ(entity.transform().scale(), spk::Vector3(1.0f, 0.0f, -2.0f));
	EXPECT_NO_THROW((void)entity.transform().localModelMatrix());
	EXPECT_NO_THROW((void)entity.transform().modelMatrix());
}
