#include <gtest/gtest.h>

#include <cstdint>

#include <memory>

#include "engine/entity.hpp"
#include "engine/entity_attachment.hpp"

namespace
{
	[[nodiscard]] spk::Engine *fakeEngine(std::uintptr_t value)
	{
		return reinterpret_cast<spk::Engine *>(value);
	}
}

TEST(EntityAttachmentTest, StandardAttachDetachPropagatesOwnerContextAndPreservesTraits)
{
	spk::Entity owner("owner");
	owner.changeContext(fakeEngine(0x1000));

	spk::EntityAttachment attachment("attachment", &owner);
	EXPECT_EQ(attachment.owner(), &owner);
	EXPECT_EQ(attachment.context(), owner.context());
	EXPECT_EQ(attachment.name(), "attachment");
	EXPECT_TRUE(attachment.isActive());

	attachment.attach(nullptr);
	EXPECT_EQ(attachment.owner(), nullptr);
	EXPECT_EQ(attachment.context(), nullptr);
}

TEST(EntityAttachmentTest, ConstructionWithNullOwnerUsesNullContext)
{
	spk::EntityAttachment attachment("orphan", nullptr);
	const spk::EntityAttachment &constAttachment = attachment;

	EXPECT_EQ(attachment.owner(), nullptr);
	EXPECT_EQ(constAttachment.owner(), nullptr);
	EXPECT_EQ(attachment.context(), nullptr);
	EXPECT_TRUE(attachment.isActive());
}

TEST(EntityAttachmentTest, OwnerOnlyConstructorUsesDefaultName)
{
	spk::Entity owner("owner");
	spk::EntityAttachment attachment(&owner);

	EXPECT_EQ(attachment.owner(), &owner);
	EXPECT_EQ(attachment.name(), "Unnamed entity attachment");
}

TEST(EntityAttachmentTest, OwnerContextChangesPropagateToTheAttachment)
{
	spk::Entity owner("owner");
	spk::EntityAttachment attachment("attachment", &owner);

	owner.changeContext(fakeEngine(0x1000));
	EXPECT_EQ(attachment.context(), fakeEngine(0x1000));

	owner.changeContext(fakeEngine(0x2000));
	EXPECT_EQ(attachment.context(), fakeEngine(0x2000));
}

TEST(EntityAttachmentTest, ReattachmentAcrossEntitiesStopsFollowingThePreviousOwner)
{
	spk::Entity first("first");
	spk::Entity second("second");
	first.changeContext(fakeEngine(0x1000));
	second.changeContext(fakeEngine(0x2000));

	spk::EntityAttachment attachment("attachment", &first);
	attachment.attach(&second);

	EXPECT_EQ(attachment.owner(), &second);
	EXPECT_EQ(attachment.context(), fakeEngine(0x2000));

	first.changeContext(fakeEngine(0x3000));
	EXPECT_EQ(attachment.context(), fakeEngine(0x2000));

	second.changeContext(fakeEngine(0x4000));
	EXPECT_EQ(attachment.context(), fakeEngine(0x4000));
}

TEST(EntityAttachmentTest, AttachingTheSameOwnerIsANoOp)
{
	spk::Entity owner("owner");
	owner.changeContext(fakeEngine(0x1000));
	spk::EntityAttachment attachment("attachment", &owner);
	int contextEditions = 0;

	auto contract = attachment.subscribeToContextEdition(
		[&](spk::Engine *const &, spk::Engine *const &) {
			++contextEditions;
		});

	attachment.attach(&owner);
	EXPECT_EQ(contextEditions, 0);
	EXPECT_EQ(attachment.owner(), &owner);
}

TEST(EntityAttachmentTest, AttachmentMayBeDestroyedBeforeItsOwner)
{
	spk::Entity owner("owner");
	EXPECT_NO_THROW({
		spk::EntityAttachment attachment("attachment", &owner);
	});
}

TEST(EntityAttachmentTest, AttachmentDestructionAfterOwnerDestructionDoesNotTouchOwnerContextContract)
{
	auto attachment = std::make_unique<spk::EntityAttachment>("attachment", nullptr);
	{
		auto owner = std::make_unique<spk::Entity>("owner");
		attachment->attach(owner.get());
	}

	// owner() is now a non-owning dangling value by the current API; do not dereference it.
	EXPECT_NO_THROW(attachment.reset());
}
