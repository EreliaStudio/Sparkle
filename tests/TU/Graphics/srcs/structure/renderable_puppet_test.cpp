#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <utility>

#include "graphics/renderable_puppet.hpp"

namespace
{
	using Blueprint = spk::Assembly::Blueprint;
	using Puppet = spk::Assembly::Puppet;

	class DrawableElement final : public Blueprint::Element
	{
	public:
		int marker;

		DrawableElement(std::string name, int value) :
			Element(std::move(name)),
			marker(value)
		{
		}
	};

	class TrackedGraphics
	{
	public:
		using ElementType = DrawableElement;

		static inline int constructed = 0;
		static inline int destroyed = 0;
		static inline bool throwOnConstruction = false;
		static inline bool throwOnSnapshot = false;

		int marker;
		std::size_t snapshotCount = 0;
		spk::Matrix4x4 lastTransform = spk::Matrix4x4::identity();

		explicit TrackedGraphics(const DrawableElement &element) :
			marker(element.marker)
		{
			if (throwOnConstruction)
			{
				throw std::runtime_error("Mock graphics creation failed");
			}
			++constructed;
		}

		void buildRenderSnapshot(spk::RenderSnapshot::Builder &builder, const spk::Matrix4x4 &transform)
		{
			if (throwOnSnapshot)
			{
				throw std::runtime_error("Mock snapshot construction failed");
			}
			++snapshotCount;
			lastTransform = transform;
			(void)builder.renderPass({"puppet-graphics-test", 0});
		}

		TrackedGraphics(const TrackedGraphics &) = delete;
		TrackedGraphics &operator=(const TrackedGraphics &) = delete;

		~TrackedGraphics()
		{
			++destroyed;
		}

		static void reset()
		{
			constructed = 0;
			destroyed = 0;
			throwOnConstruction = false;
			throwOnSnapshot = false;
		}
	};

	using Renderable = spk::Assembly::RenderablePuppet<TrackedGraphics>;

	[[nodiscard]] Blueprint characterBlueprint()
	{
		Blueprint::Builder builder;
		auto &torso = builder.setRoot<Blueprint::Element>("torso");
		torso.addAttachmentPoint("shoulder");
		auto &arm = torso.attach<DrawableElement>("shoulder", "arm", 10);
		arm.addAttachmentPoint("wrist");
		auto &hand = arm.attach<DrawableElement>("wrist", "hand", 20);
		hand.addAttachmentPoint("grip");
		torso.addAttachmentPoint("empty");
		return builder.build();
	}
}

TEST(RenderablePuppetTest, CreatesOnlyDrawableElementsAndReusesTheirGraphics)
{
	TrackedGraphics::reset();
	{
		Renderable character(characterBlueprint());
		character.synchronize();
		EXPECT_EQ(character.graphicsCount(), 2u);
		EXPECT_EQ(character.graphics(character.element("torso")), nullptr);
		auto *graphics = character.graphics(character.element("arm"));
		auto *hand = character.graphics(character.element("hand"));
		ASSERT_NE(graphics, nullptr);
		ASSERT_NE(hand, nullptr);
		EXPECT_EQ(graphics->marker, 10);
		EXPECT_EQ(hand->marker, 20);

		character.attachment("torso", "shoulder").setRotation(spk::Vector3{0, 0, 45});
		character.synchronize();

		EXPECT_EQ(character.graphics(character.element("arm")), graphics);
		EXPECT_EQ(TrackedGraphics::constructed, 2);
	}
	EXPECT_EQ(TrackedGraphics::constructed, TrackedGraphics::destroyed);
}

TEST(RenderablePuppetTest, TracksDynamicAttachmentsAndCleansDetachedSubtrees)
{
	TrackedGraphics::reset();
	Renderable character(characterBlueprint());
	character.synchronize();
	auto &grip = character.attachment("hand", "grip");
	Puppet &bound = grip.attach(characterBlueprint());

	character.synchronize();
	ASSERT_EQ(character.graphicsCount(), 4u);
	const Puppet::ElementNode *attachedArm = &bound.element("arm");
	ASSERT_NE(character.graphics(*attachedArm), nullptr);

	Puppet detached = grip.detach(bound);
	character.synchronize();
	EXPECT_EQ(character.graphicsCount(), 2u);
	EXPECT_EQ(TrackedGraphics::destroyed, 2);

	Puppet &rebound = grip.attach(std::move(detached));
	EXPECT_EQ(&rebound.element("arm"), attachedArm);
	character.synchronize();
	EXPECT_EQ(character.graphicsCount(), 4u);
	EXPECT_EQ(TrackedGraphics::constructed, 6);
}

TEST(RenderablePuppetTest, ReattachBeforeSynchronizeRebuildsOnlyRelocatedGraphics)
{
	TrackedGraphics::reset();
	Renderable character(characterBlueprint());
	auto &grip = character.attachment("hand", "grip");
	Puppet &bound = grip.attach(characterBlueprint());
	character.synchronize();
	const auto *attachedArm = &bound.element("arm");
	ASSERT_EQ(character.graphicsCount(), 4u);

	Puppet detached = grip.detach(bound);
	Puppet &rebound = grip.attach(std::move(detached));
	ASSERT_EQ(&rebound.element("arm"), attachedArm);
	character.synchronize();

	ASSERT_EQ(character.graphicsCount(), 4u);
	ASSERT_NE(character.graphics(rebound.element("arm")), nullptr);
	ASSERT_NE(character.graphics(rebound.element("hand")), nullptr);
	EXPECT_EQ(character.graphics(rebound.element("arm"))->marker, 10);
	EXPECT_EQ(character.graphics(rebound.element("hand"))->marker, 20);
	EXPECT_EQ(TrackedGraphics::constructed, 6);
	EXPECT_EQ(TrackedGraphics::destroyed, 2);
}

TEST(RenderablePuppetTest, CopyHasIndependentArticulationsAndGraphics)
{
	TrackedGraphics::reset();
	Renderable first(characterBlueprint());
	first.synchronize();
	Renderable copy(first);

	EXPECT_EQ(copy.graphicsCount(), 0u);
	copy.attachment("torso", "shoulder").setRotation(spk::Vector3{0, 0, 45});
	copy.synchronize();

	EXPECT_EQ(copy.graphicsCount(), 2u);
	EXPECT_NE(copy.graphics(copy.element("arm")), first.graphics(first.element("arm")));
	EXPECT_NE(copy.element("arm").globalTransform()[0][0], first.element("arm").globalTransform()[0][0]);

	copy = first;
	EXPECT_EQ(copy.graphicsCount(), 0u);
	copy.synchronize();
	EXPECT_EQ(copy.graphicsCount(), 2u);
}

TEST(RenderablePuppetTest, MovePreservesGraphicsAndNodes)
{
	TrackedGraphics::reset();
	Renderable original(characterBlueprint());
	original.synchronize();
	const auto *node = &original.element("arm");
	const auto *graphics = original.graphics(*node);

	Renderable moved(std::move(original));
	EXPECT_EQ(&moved.element("arm"), node);
	EXPECT_EQ(moved.graphics(*node), graphics);
	moved.synchronize();
	EXPECT_EQ(TrackedGraphics::constructed, 2);
}

TEST(RenderablePuppetTest, FailedConstructionCanBeRetried)
{
	TrackedGraphics::reset();
	Renderable character(characterBlueprint());
	TrackedGraphics::throwOnConstruction = true;
	EXPECT_THROW(character.synchronize(), std::runtime_error);
	EXPECT_EQ(character.graphicsCount(), 0u);

	TrackedGraphics::throwOnConstruction = false;
	EXPECT_NO_THROW(character.synchronize());
	EXPECT_EQ(character.graphicsCount(), 2u);
}

TEST(RenderablePuppetTest, BuildsSnapshotsWithEachElementWorldTransform)
{
	TrackedGraphics::reset();
	Renderable character(characterBlueprint());
	spk::RenderSnapshot::Builder builder;
	character.buildRenderSnapshot(builder);

	EXPECT_EQ(character.graphicsCount(), 2u);
	auto *arm = character.graphics(character.element("arm"));
	ASSERT_NE(arm, nullptr);
	EXPECT_EQ(arm->snapshotCount, 1u);
	EXPECT_EQ(arm->lastTransform[3][0], character.element("arm").globalTransform()[3][0]);

	const auto cameraTransform = spk::Matrix4x4::translation({1, 2, 3});
	character.attachment("torso", "shoulder").setRotation(spk::Vector3{0, 0, 45});
	character.buildRenderSnapshot(builder, cameraTransform);

	ASSERT_EQ(arm, character.graphics(character.element("arm")));
	EXPECT_EQ(arm->snapshotCount, 2u);
	const auto expected = cameraTransform * character.element("arm").globalTransform();
	EXPECT_EQ(arm->lastTransform[0][0], expected[0][0]);
	EXPECT_EQ(arm->lastTransform[3][1], expected[3][1]);
	EXPECT_EQ(TrackedGraphics::constructed, 2);
}

TEST(RenderablePuppetTest, MoveAssignmentPreservesGraphicsAndNodes)
{
	TrackedGraphics::reset();
	Renderable source(characterBlueprint());
	source.synchronize();
	const auto *node = &source.element("arm");
	const auto *graphics = source.graphics(*node);

	Renderable target(characterBlueprint());
	target.synchronize();
	target = std::move(source);

	EXPECT_EQ(&target.element("arm"), node);
	EXPECT_EQ(target.graphics(*node), graphics);
	target.synchronize();
	EXPECT_EQ(TrackedGraphics::constructed, 4);
	EXPECT_EQ(TrackedGraphics::destroyed, 2);
}

TEST(RenderablePuppetTest, FailedSnapshotCanBeRetriedWithoutCorruptingGraphics)
{
	TrackedGraphics::reset();
	Renderable character(characterBlueprint());
	TrackedGraphics::throwOnSnapshot = true;
	spk::RenderSnapshot::Builder failed;
	EXPECT_THROW(character.buildRenderSnapshot(failed), std::runtime_error);
	EXPECT_EQ(character.graphicsCount(), 1u);

	TrackedGraphics::throwOnSnapshot = false;
	spk::RenderSnapshot::Builder retry;
	EXPECT_NO_THROW(character.buildRenderSnapshot(retry));
	EXPECT_EQ(character.graphicsCount(), 2u);
	EXPECT_EQ(TrackedGraphics::constructed, 2);
	EXPECT_EQ(character.graphics(character.element("arm"))->snapshotCount, 1u);
}

TEST(RenderablePuppetTest, FailedTraversalDoesNotPrunePreviouslyCachedGraphics)
{
	TrackedGraphics::reset();
	Renderable character(characterBlueprint());
	character.synchronize();
	auto &grip = character.attachment("hand", "grip");
	Puppet &bound = grip.attach(characterBlueprint());
	character.synchronize();
	ASSERT_EQ(character.graphicsCount(), 4u);

	Puppet detached = grip.detach(bound);
	TrackedGraphics::throwOnSnapshot = true;
	spk::RenderSnapshot::Builder failed;
	EXPECT_THROW(character.buildRenderSnapshot(failed), std::runtime_error);
	EXPECT_EQ(character.graphicsCount(), 4u);

	TrackedGraphics::throwOnSnapshot = false;
	character.synchronize();
	EXPECT_EQ(character.graphicsCount(), 2u);
	EXPECT_NE(&detached.element("arm"), &character.element("arm"));
}

namespace
{
	class MissingSnapshotMethod
	{
	public:
		using ElementType = DrawableElement;
		explicit MissingSnapshotMethod(const DrawableElement &)
		{
		}
	};
}

static_assert(spk::Assembly::PuppetGraphics<TrackedGraphics>);
static_assert(!spk::Assembly::PuppetGraphics<MissingSnapshotMethod>);
