#include "engine/publishable_behaviour.hpp"
#include "engine/publishable_component.hpp"
#include "engine/publishable_entity2d.hpp"
#include "engine/publishable_entity3d.hpp"
#include "exception.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace
{
	class Health : public spk::PublishableComponent
	{
	public:
		int points = 10;

		void setPoints(int next)
		{
			points = next;
			invalidate();
		}

	protected:
		void _encodeData(spk::ByteStream::Writer &writer) const override
		{
			writer << points;
		}

		void _decodeData(const spk::ByteStream::Reader &reader) override
		{
			reader >> points;
		}
	};

	class Movement : public spk::PublishableBehaviour
	{
	public:
		int velocity = 0;

		void change(int next)
		{
			velocity = next;
			invalidate();
		}

	protected:
		void _encodeData(spk::ByteStream::Writer &writer) const override
		{
			writer << velocity;
		}

		void _decodeData(const spk::ByteStream::Reader &reader) override
		{
			reader >> velocity;
		}
	};

	class Character3D : public spk::PublishableEntity3D
	{
	public:
		explicit Character3D(const std::string &name) :
			PublishableEntity3D(Mode::Source, name)
		{
		}

		Character3D(const spk::UUID &uuid, const std::string &name) :
			PublishableEntity3D(Mode::Replicant, uuid, name)
		{
		}

		using PublishableEntity::setPartRegistry;
	};

	class Character2D : public spk::PublishableEntity2D
	{
	public:
		explicit Character2D(const std::string &name) :
			PublishableEntity2D(Mode::Source, name)
		{
		}

		Character2D(const spk::UUID &uuid, const std::string &name) :
			PublishableEntity2D(Mode::Replicant, uuid, name)
		{
		}

		using PublishableEntity::setPartRegistry;
	};

	std::vector<std::uint32_t> ids(const spk::ByteStream &bytes)
	{
		const auto reader = bytes.reader();
		std::uint8_t mode;
		std::uint32_t count;
		reader >> mode >> count;
		std::vector<std::uint32_t> result;
		for (std::uint32_t index = 0; index < count; ++index)
		{
			std::uint32_t id;
			std::uint64_t generation;
			std::uint64_t length;
			reader >> id >> generation >> length;
			result.push_back(id);
			reader.skip(static_cast<std::size_t>(length));
		}
		return result;
	}
}

TEST(PublishableEntityTest, SourceUUIDIsGeneratedAndReplicantUsesAuthoritativeUUID)
{
	Character3D source("source");
	Character3D other("other");
	Character3D replica(source.uuid(), "replica");
	EXPECT_FALSE(source.uuid().isNull());
	EXPECT_NE(source.uuid(), other.uuid());
	EXPECT_EQ(source.uuid(), replica.uuid());
	EXPECT_EQ(source.mode(), spk::Publishable::Mode::Source);
	EXPECT_EQ(replica.mode(), spk::Publishable::Mode::Replicant);
}

TEST(PublishableEntityTest, BuiltInPartsRemainAfterGameRegistryReplacement)
{
	Character3D entity("entity");
	auto &health = entity.addComponent<Health>();
	auto &movement = entity.addBehaviour<Movement>();
	entity.setPartRegistry({{0, health, Character3D::Publication::Update}, {1, movement, {}}});
	EXPECT_EQ(ids(entity.encodeInitialization()), (std::vector<std::uint32_t>{0, 1, 16, 17}));
	entity.setPartRegistry({});
	EXPECT_EQ(ids(entity.encodeInitialization()), (std::vector<std::uint32_t>{0, 1}));
}

TEST(PublishableEntityTest, EngineAttachmentsAndTransformSerializeAndUpdate)
{
	Character3D source("source");
	Character3D replica(source.uuid(), "replica");
	auto &health = source.addComponent<Health>();
	auto &movement = source.addBehaviour<Movement>();
	auto &receivedHealth = replica.addComponent<Health>();
	auto &receivedMovement = replica.addBehaviour<Movement>();
	source.setPartRegistry({{0, health, Character3D::Publication::Update}, {1, movement, Character3D::Publication::Update}});
	replica.setPartRegistry({{0, receivedHealth, Character3D::Publication::Update}, {1, receivedMovement, Character3D::Publication::Update}});
	source.transform().place(spk::Vector3{4, 5, 6});
	source.transform().rescale(spk::Vector3{2, 3, 4});
	source.transform().setRotation(spk::Quaternion{0, 0, 0, 1});
	health.setPoints(90);
	movement.change(6);

	replica.decode(source.encodeInitialization().reader());
	EXPECT_EQ(receivedHealth.points, 90);
	EXPECT_EQ(receivedMovement.velocity, 6);
	EXPECT_EQ(replica.transform().position(), (spk::Vector3{4, 5, 6}));
	EXPECT_EQ(replica.transform().scale(), (spk::Vector3{2, 3, 4}));

	health.deactivate();
	movement.deactivate();
	source.deactivate();
	replica.decode(source.encodeUpdate().reader());
	EXPECT_FALSE(receivedHealth.isActive());
	EXPECT_FALSE(receivedMovement.isActive());
	EXPECT_FALSE(replica.isActive());
}

TEST(PublishableEntityTest, TwoDimensionalTransformIncludesDepth)
{
	Character2D source("source");
	Character2D replica(source.uuid(), "replica");
	source.transform().place(spk::Vector2{3, 4});
	source.transform().rescale(spk::Vector2{5, 6});
	source.transform().setRotation(0.5f);
	source.transform().setDepth(17.0f);

	replica.decode(source.encodeInitialization().reader());
	EXPECT_EQ(replica.transform().position(), (spk::Vector2{3, 4}));
	EXPECT_EQ(replica.transform().scale(), (spk::Vector2{5, 6}));
	EXPECT_FLOAT_EQ(replica.transform().rotation(), 0.5f);
	EXPECT_FLOAT_EQ(replica.transform().depth(), 17.0f);
}

TEST(PublishableEntityTest, GamePartIDsCannotOverflowInternalOffset)
{
	Character3D entity("entity");
	auto &health = entity.addComponent<Health>();
	EXPECT_THROW(entity.setPartRegistry({{std::numeric_limits<std::uint32_t>::max(), health, {}}}), spk::Exception);
	EXPECT_EQ(ids(entity.encodeInitialization()), (std::vector<std::uint32_t>{0, 1}));
}

TEST(PublishableEntityTest, ActivationOnlyAttachmentsNeedNoDataHooks)
{
	Character3D source("source");
	Character3D replica(source.uuid(), "replica");
	auto &flag = source.addComponent<spk::PublishableComponent>();
	auto &replicated = replica.addComponent<spk::PublishableComponent>();
	source.setPartRegistry({{0, flag, Character3D::Publication::Update}});
	replica.setPartRegistry({{0, replicated, Character3D::Publication::Update}});
	flag.deactivate();
	replica.decode(source.encodeInitialization().reader());
	EXPECT_FALSE(replicated.isActive());
	flag.activate();
	replica.decode(source.encodeUpdate().reader());
	EXPECT_TRUE(replicated.isActive());
}
