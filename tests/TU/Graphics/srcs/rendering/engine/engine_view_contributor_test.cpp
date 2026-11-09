#include <gtest/gtest.h>

#include <concepts>
#include <memory>
#include <type_traits>

#include "rendering/engine/engine_view.hpp"
#include "rendering/engine/rendering_behaviour.hpp"
#include "rendering/engine/rendering_component.hpp"
#include "rendering/engine/rendering_entity.hpp"
#include "rendering/engine/rendering_object_trait.hpp"
#include "rendering/engine/rendering_system.hpp"

namespace
{
	using Contributor = spk::EngineView::Contributor;
	using CaptureContext = spk::EngineView::CaptureContext;
	using SnapshotMethod = void (Contributor::*)(spk::RenderSnapshot::Builder &, const CaptureContext &);

	static_assert(std::has_virtual_destructor_v<Contributor>);
	static_assert(std::is_same_v<decltype(&Contributor::buildViewRenderSnapshot), SnapshotMethod>);
	static_assert(std::derived_from<spk::RenderingObjectTrait, Contributor>);
	static_assert(std::derived_from<spk::RenderingEntity, Contributor>);
	static_assert(std::derived_from<spk::RenderingEntity2D, Contributor>);
	static_assert(std::derived_from<spk::RenderingEntity3D, Contributor>);
	static_assert(std::derived_from<spk::RenderingComponent, Contributor>);
	static_assert(std::derived_from<spk::RenderingComponent2D, Contributor>);
	static_assert(std::derived_from<spk::RenderingComponent3D, Contributor>);
	static_assert(std::derived_from<spk::RenderingBehaviour, Contributor>);
	static_assert(std::derived_from<spk::RenderingBehaviour2D, Contributor>);
	static_assert(std::derived_from<spk::RenderingBehaviour3D, Contributor>);
	static_assert(std::derived_from<spk::RenderingSystem, Contributor>);

	class DestructorProbe final : public Contributor
	{
	public:
		explicit DestructorProbe(bool &destroyed) :
			_destroyed(destroyed)
		{
		}

		~DestructorProbe() override
		{
			_destroyed = true;
		}

	private:
		bool &_destroyed;
	};

	class CustomContributor final : public spk::RenderingComponent3D
	{
	public:
		using RenderingComponent3D::RenderingComponent3D;

		void buildViewRenderSnapshot(spk::RenderSnapshot::Builder &, const CaptureContext &) override
		{
		}
	};
}

TEST(EngineViewContributorTest, PolymorphicDeletionInvokesConcreteDestructor)
{
	bool destroyed = false;
	std::unique_ptr<Contributor> contributor = std::make_unique<DestructorProbe>(destroyed);
	contributor.reset();
	EXPECT_TRUE(destroyed);
}

TEST(EngineViewContributorTest, DefaultContributorIsConcreteWithoutViewSpecificLogic)
{
	static_assert(!std::is_abstract_v<Contributor>);
	Contributor contributor;
	SUCCEED();
}

TEST(EngineViewContributorTest, ExistingRenderingComponentsSupportViewSpecificOverride)
{
	static_assert(std::derived_from<CustomContributor, Contributor>);
	static_assert(!std::is_abstract_v<CustomContributor>);
	CustomContributor component;
	EXPECT_EQ(component.owner(), nullptr);
}

TEST(EngineViewContributorTest, AllRenderingExtensionFamiliesSatisfyTheContributorContract)
{
	static_assert(std::derived_from<spk::RenderingEntity, Contributor>);
	static_assert(std::derived_from<spk::RenderingEntity2D, Contributor>);
	static_assert(std::derived_from<spk::RenderingEntity3D, Contributor>);
	static_assert(std::derived_from<spk::RenderingComponent, Contributor>);
	static_assert(std::derived_from<spk::RenderingComponent2D, Contributor>);
	static_assert(std::derived_from<spk::RenderingComponent3D, Contributor>);
	static_assert(std::derived_from<spk::RenderingBehaviour, Contributor>);
	static_assert(std::derived_from<spk::RenderingBehaviour2D, Contributor>);
	static_assert(std::derived_from<spk::RenderingBehaviour3D, Contributor>);
	static_assert(std::derived_from<spk::RenderingSystem, Contributor>);
	SUCCEED();
}
