#include <gtest/gtest.h>

#include <utility>

#include "engine/engine.hpp"
#include "engine/entity2d.hpp"
#include "engine/entity3d.hpp"
#include "engine/registry.hpp"
#include "engine/transform2d.hpp"
#include "engine/transform3d.hpp"

TEST(Entity3DTest, ConstructsAnOwnedTransformAndSupportsConstAndMutableAccess)
{
	spk::Entity3D entity("entity");
	spk::Transform3D *transform = &entity.transform();

	EXPECT_EQ(entity.getParticipant<spk::Transform3D>(), transform);
	EXPECT_EQ(transform->owner(), &entity);
	EXPECT_EQ(&std::as_const(entity).transform(), transform);
	EXPECT_EQ(transform->position(), spk::Vector3(0.0f, 0.0f, 0.0f));
	EXPECT_EQ(transform->scale(), spk::Vector3(1.0f, 1.0f, 1.0f));
	EXPECT_EQ(transform->rotation(), spk::Quaternion::identity());
}

TEST(Entity3DTest, ParentTransformComposesWorldValuesAndReparentingInvalidatesThem)
{
	spk::Entity3D firstParent("first");
	spk::Entity3D secondParent("second");
	spk::Entity3D child("child", &firstParent);

	firstParent.transform().place({10.0f, 20.0f, 30.0f});
	secondParent.transform().place({-4.0f, -5.0f, -6.0f});
	child.transform().place({1.0f, 2.0f, 3.0f});
	EXPECT_EQ(child.transform().position(spk::ReferenceFrame::World), spk::Vector3(11.0f, 22.0f, 33.0f));

	child.setParent(&secondParent);
	EXPECT_EQ(child.transform().position(spk::ReferenceFrame::World), spk::Vector3(-3.0f, -3.0f, -3.0f));

	child.setParent(nullptr);
	EXPECT_EQ(child.transform().position(spk::ReferenceFrame::World), child.transform().position());
}

TEST(Entity3DTest, PlainAnd2DParentsDoNotContributeA3DTransform)
{
	spk::Entity plain("plain");
	spk::Entity2D parent2D("2d");
	spk::Entity3D child("child");
	child.transform().place({1.0f, 2.0f, 3.0f});

	child.setParent(&plain);
	EXPECT_EQ(child.transform().position(spk::ReferenceFrame::World), spk::Vector3(1.0f, 2.0f, 3.0f));

	parent2D.transform().place({100.0f, 200.0f});
	child.setParent(&parent2D);
	EXPECT_EQ(child.transform().position(spk::ReferenceFrame::World), spk::Vector3(1.0f, 2.0f, 3.0f));
}

TEST(Entity3DTest, EngineContextControlsTypedEntityAndTransformRegistryMembership)
{
	spk::Engine engine;
	spk::Entity3D entity("entity");
	spk::Transform3D *transform = &entity.transform();

	engine.addEntity(&entity);
	EXPECT_TRUE((spk::Registry<spk::Engine *, spk::Entity3D>::elements(&engine).contains(&entity)));
	EXPECT_TRUE((spk::Registry<spk::Engine *, spk::Transform3D>::elements(&engine).contains(transform)));

	engine.removeEntity(&entity);
	EXPECT_FALSE((spk::Registry<spk::Engine *, spk::Entity3D>::elements(&engine).contains(&entity)));
	EXPECT_FALSE((spk::Registry<spk::Engine *, spk::Transform3D>::elements(&engine).contains(transform)));
}

TEST(Entity3DTest, TransformLivesForTheWholeEntityLifetime)
{
	spk::Transform3D *transform = nullptr;
	{
		spk::Entity3D entity("entity");
		transform = &entity.transform();
		EXPECT_EQ(transform->owner(), &entity);
	}

	EXPECT_FALSE((spk::Registry<spk::Engine *, spk::Transform3D>::elements(nullptr).contains(transform)));
}
