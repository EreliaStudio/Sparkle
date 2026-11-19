#include <gtest/gtest.h>

#include "exception.hpp"
#include "network/server.hpp"
#include "network_test_utils.hpp"
#include "voxel/network.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
	namespace Voxel = spk::Voxel;
	namespace Net = spk::Voxel::Network;
	using Position = Voxel::ChunkPosition;
	using Edit = Voxel::CellEdit;
	using Registry = Voxel::Definition::Registry;

	constexpr spk::Vector3UInt Dimensions{4, 2, 2};
	constexpr std::uint64_t Catalogue = 0x9a7548cd;

	[[nodiscard]] Registry registry()
	{
		Voxel::Shape::Builder shape;
		shape.addSlot("face");
		const auto shared = shape.build();
		Registry::Builder builder;
		builder.add(7, Voxel::Definition(shared, {{"face", {"stone"}}}));
		builder.add(12, Voxel::Definition(shared, {{"face", {"glass", 0.5f}}}));
		return builder.build();
	}

	[[nodiscard]] Voxel::Volume volume(const Registry &definitions, std::uint32_t material = 0)
	{
		Voxel::Volume::Builder builder(Dimensions);
		builder.bind(definitions);
		if (material != 0)
		{
			builder.set({1, 0, 0}, material);
		}
		return builder.build();
	}

	[[nodiscard]] Edit edit(std::uint32_t x, std::uint32_t y, std::uint32_t z, std::uint32_t material)
	{
		return {{x, y, z}, Voxel::Volume::PackedCell::encode(material, Voxel::Cell::HorizontalOrientation::XPositive, Voxel::Cell::VerticalOrientation::YPositive)};
	}

	template <typename Predicate, typename... TClients>
	[[nodiscard]] bool pump(Net::Server &server, Predicate predicate, TClients &...clients)
	{
		return NetworkTestUtils::waitUntil(
			[&] {
				server.update();
				(clients.update(), ...);
				return predicate();
			},
			5s);
	}

	[[nodiscard]] bool same(const Voxel::Volume &first, const Voxel::Volume &second)
	{
		if (first.dimensions() != second.dimensions())
		{
			return false;
		}
		for (std::uint32_t z = 0; z < Dimensions.z; ++z)
		{
			for (std::uint32_t y = 0; y < Dimensions.y; ++y)
			{
				for (std::uint32_t x = 0; x < Dimensions.x; ++x)
				{
					if (first.atRaw({x, y, z}) != second.atRaw({x, y, z}))
					{
						return false;
					}
				}
			}
		}
		return true;
	}

	void put(spk::Message::Writer &writer, std::uint64_t value, unsigned count)
	{
		for (unsigned index = 0; index < count; ++index)
		{
			const auto byte = std::byte((value >> (8 * index)) & 0xffu);
			writer.append(&byte, 1);
		}
	}

	[[nodiscard]] spk::Message maliciousSnapshot(std::uint64_t revision, std::uint32_t cells)
	{
		spk::Message::Writer writer(Net::Protocol::Snapshot);
		put(writer, Net::Protocol::Version, 1);
		put(writer, 0, 12);
		put(writer, revision, 8);
		put(writer, cells, 4);
		for (std::uint32_t index = 0; index < cells; ++index)
		{
			put(writer, 0, 4);
		}
		return std::move(writer).build();
	}
}

TEST(VoxelNetworkTest, InitialSyncPreservesPackedCellOrientationsAndCoordinates)
{
	const auto definitions = registry();
	Net::Server server(Dimensions, definitions, Catalogue);
	Net::Client client(Dimensions, definitions, Catalogue);
	const Position key{-3, 12, 9};
	Voxel::Volume::Builder builder(Dimensions);
	builder.bind(definitions);
	builder.set({1, 1, 1}, 12, Voxel::Cell::HorizontalOrientation::ZPositive, Voxel::Cell::VerticalOrientation::YNegative);
	server.setChunk(key, builder.build());
	server.start(0);
	client.connect("127.0.0.1", server.port());

	ASSERT_TRUE(pump(server, [&] {
		return client.isSynchronized() && client.world().chunk(key) != nullptr;
	},
					 client));
	ASSERT_NE(client.world().chunk(key), nullptr);
	EXPECT_EQ(client.world().revision(key), 1u);
	EXPECT_TRUE(same(*server.world().chunk(key), *client.world().chunk(key)));
	EXPECT_EQ(client.world().chunk(key)->at({1, 1, 1}).horizontalOrientation(), Voxel::Cell::HorizontalOrientation::ZPositive);
	EXPECT_EQ(client.world().chunk(key)->at({1, 1, 1}).verticalOrientation(), Voxel::Cell::VerticalOrientation::YNegative);
	EXPECT_EQ(client.takeChangedChunks(), std::vector<Position>{key});
	client.disconnect();
	server.stop();
}

TEST(VoxelNetworkTest, ServerEditsReachTwoClientsAndOnlyInvalidateChangedChunk)
{
	const auto definitions = registry();
	Net::Server server(Dimensions, definitions, Catalogue);
	Net::Client first(Dimensions, definitions, Catalogue);
	Net::Client second(Dimensions, definitions, Catalogue);
	const Position key{0, 0, 0};
	server.setChunk(key, volume(definitions, 7));
	server.start(0);
	first.connect("127.0.0.1", server.port());
	second.connect("127.0.0.1", server.port());
	ASSERT_TRUE(pump(server, [&] {
		return first.isSynchronized() && second.isSynchronized();
	},
					 first,
					 second));
	(void)first.takeChangedChunks();
	(void)second.takeChangedChunks();
	const std::array updates{edit(0, 0, 0, 12), edit(2, 1, 1, 7)};
	ASSERT_TRUE(server.edit(key, updates));
	ASSERT_TRUE(pump(server, [&] {
		return first.world().revision(key) == 2 && second.world().revision(key) == 2;
	},
					 first,
					 second));
	EXPECT_TRUE(same(*server.world().chunk(key), *first.world().chunk(key)));
	EXPECT_TRUE(same(*server.world().chunk(key), *second.world().chunk(key)));
	EXPECT_EQ(first.takeChangedChunks(), std::vector<Position>{key});
	EXPECT_EQ(second.takeChangedChunks(), std::vector<Position>{key});
	EXPECT_FALSE(server.edit(key, updates));
	EXPECT_EQ(server.world().revision(key), 2u);
	first.disconnect();
	second.disconnect();
	server.stop();
}

TEST(VoxelNetworkTest, ClientRequestedEditIsAuthoritativeAndBroadcastToBoth)
{
	const auto definitions = registry();
	Net::Server server(Dimensions, definitions, Catalogue);
	Net::Client first(Dimensions, definitions, Catalogue);
	Net::Client second(Dimensions, definitions, Catalogue);
	const Position key{5, -4, 0};
	server.setChunk(key, volume(definitions));
	server.start(0);
	first.connect("127.0.0.1", server.port());
	second.connect("127.0.0.1", server.port());
	ASSERT_TRUE(pump(server, [&] {
		return first.isSynchronized() && second.isSynchronized();
	},
					 first,
					 second));
	const std::array batch{edit(1, 1, 0, 7), edit(3, 0, 1, 12)};
	first.requestEdit(key, batch);
	ASSERT_TRUE(pump(server, [&] {
		return first.world().revision(key) == 2 && second.world().revision(key) == 2;
	},
					 first,
					 second));
	EXPECT_TRUE(same(*server.world().chunk(key), *first.world().chunk(key)));
	EXPECT_TRUE(same(*server.world().chunk(key), *second.world().chunk(key)));
	first.disconnect();
	second.disconnect();
	server.stop();
}

TEST(VoxelNetworkTest, LateJoinReceivesCurrentSnapshotsForSeveralChunks)
{
	const auto definitions = registry();
	Net::Server server(Dimensions, definitions, Catalogue);
	const Position a{-1, 0, 1};
	const Position b{9, 0, -1};
	server.setChunk(a, volume(definitions));
	server.setChunk(b, volume(definitions, 7));
	ASSERT_TRUE(server.edit(a, std::array{edit(2, 1, 0, 12)}));
	ASSERT_TRUE(server.edit(a, std::array{edit(2, 1, 0, 7)}));
	server.start(0);
	Net::Client late(Dimensions, definitions, Catalogue);
	late.connect("127.0.0.1", server.port());
	ASSERT_TRUE(pump(server, [&] {
		return late.isSynchronized() && late.world().chunks().size() == 2;
	},
					 late));
	EXPECT_EQ(late.world().revision(a), 3u);
	EXPECT_EQ(late.world().revision(b), 1u);
	EXPECT_TRUE(same(*server.world().chunk(a), *late.world().chunk(a)));
	EXPECT_TRUE(same(*server.world().chunk(b), *late.world().chunk(b)));
	late.disconnect();
	server.stop();
}

TEST(VoxelNetworkTest, DisconnectReconnectGetsLatestStateAndInvalidatesPrevious)
{
	const auto definitions = registry();
	Net::Server server(Dimensions, definitions, Catalogue);
	const Position key{0, 0, 0};
	server.setChunk(key, volume(definitions));
	server.start(0);
	Net::Client client(Dimensions, definitions, Catalogue);
	client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(pump(server, [&] {
		return client.isSynchronized();
	},
					 client));
	(void)client.takeChangedChunks();
	client.disconnect();
	ASSERT_TRUE(server.edit(key, std::array{edit(1, 0, 0, 12)}));
	client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(pump(server, [&] {
		return client.isSynchronized() && client.world().revision(key) == 2;
	},
					 client));
	EXPECT_TRUE(same(*server.world().chunk(key), *client.world().chunk(key)));
	EXPECT_EQ(client.takeChangedChunks(), std::vector<Position>{key});
	client.disconnect();
	server.stop();
}

TEST(VoxelNetworkTest, RejectsUnknownChunksInvalidMaterialsAndDuplicateEdits)
{
	const auto definitions = registry();
	Net::Server server(Dimensions, definitions, Catalogue);
	const Position key{1, 1, 1};
	server.setChunk(key, volume(definitions));
	EXPECT_THROW((void)server.edit({8, 8, 8}, std::array{edit(0, 0, 0, 7)}), spk::Exception);
	EXPECT_THROW((void)server.edit(key, std::array{edit(0, 0, 0, 4)}), spk::Exception);
	EXPECT_THROW((void)server.edit(key, std::array{edit(4, 0, 0, 7)}), spk::Exception);
	EXPECT_THROW((void)server.edit(key, std::array{edit(0, 0, 0, 7), edit(0, 0, 0, 12)}), spk::Exception);
	EXPECT_EQ(server.world().revision(key), 1u);
	EXPECT_EQ(server.world().chunk(key)->atRaw({0, 0, 0}), 0u);
}

TEST(VoxelNetworkTest, ValidatorCanRejectClientEditsWithoutChangingWorld)
{
	const auto definitions = registry();
	Net::Server server(Dimensions, definitions, Catalogue);
	Net::Client client(Dimensions, definitions, Catalogue);
	const Position key{0, 0, 0};
	server.setChunk(key, volume(definitions));
	server.setEditValidator([](spk::ConnectionID, Position, std::span<const Edit>) {
		return false;
	});
	server.start(0);
	client.connect("127.0.0.1", server.port());
	ASSERT_TRUE(pump(server, [&] {
		return client.isSynchronized();
	},
					 client));
	client.requestEdit(key, std::array{edit(0, 0, 0, 7)});
	ASSERT_TRUE(pump(server, [&] {
		return client.lastError() == Net::Error::InvalidRequest;
	},
					 client));
	EXPECT_EQ(server.world().revision(key), 1u);
	EXPECT_EQ(client.world().revision(key), 1u);
	client.disconnect();
	server.stop();
}

TEST(VoxelNetworkTest, RejectsMismatchedCataloguesAndChunkDimensions)
{
	const auto definitions = registry();
	Net::Server server(Dimensions, definitions, Catalogue);
	Net::Client wrongCatalogue(Dimensions, definitions, Catalogue + 1);
	Net::Client wrongDimensions({2, 2, 2}, definitions, Catalogue);
	server.start(0);
	wrongCatalogue.connect("127.0.0.1", server.port());
	ASSERT_TRUE(pump(server, [&] {
		return wrongCatalogue.lastError() == Net::Error::CatalogueMismatch;
	},
					 wrongCatalogue));
	EXPECT_FALSE(wrongCatalogue.isSynchronized());
	wrongDimensions.connect("127.0.0.1", server.port());
	ASSERT_TRUE(pump(server, [&] {
		return wrongDimensions.lastError() == Net::Error::CatalogueMismatch;
	},
					 wrongDimensions));
	EXPECT_FALSE(wrongDimensions.isSynchronized());
	wrongCatalogue.disconnect();
	wrongDimensions.disconnect();
	server.stop();
}

TEST(VoxelNetworkTest, MalformedOrTruncatedSnapshotCannotMutateClientWorld)
{
	const auto definitions = registry();
	spk::Server raw;
	Net::Client client(Dimensions, definitions, Catalogue);
	raw.start(0);
	client.connect("127.0.0.1", raw.port());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.isConnected();
	},
											5s));
	auto connections = raw.connections();
	ASSERT_EQ(connections.size(), 1u);
	spk::Message::Writer truncated(Net::Protocol::Snapshot);
	put(truncated, Net::Protocol::Version, 1);
	put(truncated, 0, 12);
	put(truncated, 1, 8);
	put(truncated, 16, 4);
	put(truncated, 0, 4);
	(void)raw.sendTo(*connections.begin(), std::move(truncated).build());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.lastError() == Net::Error::InvalidRequest;
	},
											5s));
	EXPECT_TRUE(client.world().chunks().empty());
	EXPECT_FALSE(client.isSynchronized());
	client.disconnect();
	raw.stop();
}

TEST(VoxelNetworkTest, UnsupportedProtocolVersionIsRejected)
{
	const auto definitions = registry();
	spk::Server raw;
	Net::Client client(Dimensions, definitions, Catalogue);
	raw.start(0);
	client.connect("127.0.0.1", raw.port());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.isConnected();
	},
											5s));
	auto connections = raw.connections();
	ASSERT_EQ(connections.size(), 1u);
	spk::Message::Writer bad(Net::Protocol::Ready);
	put(bad, Net::Protocol::Version + 1, 1);
	(void)raw.sendTo(*connections.begin(), std::move(bad).build());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.lastError() == Net::Error::ProtocolMismatch;
	},
											5s));
	EXPECT_FALSE(client.isSynchronized());
	client.disconnect();
	raw.stop();
}

TEST(VoxelNetworkTest, GapRequestsResynchronizationAndAcceptsNewerSnapshot)
{
	const auto definitions = registry();
	spk::Server raw;
	Net::Client client(Dimensions, definitions, Catalogue);
	const Position key{0, 0, 0};
	raw.start(0);
	client.connect("127.0.0.1", raw.port());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.isConnected();
	},
											5s));
	const auto connections = raw.connections();
	ASSERT_EQ(connections.size(), 1u);
	(void)raw.sendTo(*connections.begin(), maliciousSnapshot(1, 16));
	spk::Message::Writer ready(Net::Protocol::Ready);
	put(ready, Net::Protocol::Version, 1);
	put(ready, 1, 8);
	(void)raw.sendTo(*connections.begin(), std::move(ready).build());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.isSynchronized() && client.world().revision(key) == 1;
	},
											5s));
	std::vector<spk::ReceivedMessage> old;
	(void)raw.messages().drain(old);

	spk::Message::Writer ahead(Net::Protocol::Patch);
	put(ahead, Net::Protocol::Version, 1);
	put(ahead, 0, 12);
	put(ahead, 3, 8);
	put(ahead, 4, 8);
	put(ahead, 1, 4);
	put(ahead, 0, 12);
	put(ahead, edit(0, 0, 0, 7).value, 4);
	(void)raw.sendTo(*connections.begin(), std::move(ahead).build());
	std::vector<spk::ReceivedMessage> requests;
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		std::vector<spk::ReceivedMessage> batch;
		(void)raw.messages().drain(batch);
		for (auto &request : batch)
		{
			requests.push_back(std::move(request));
		}
		for (const auto &request : requests)
		{
			if (request.message.type() == Net::Protocol::ResyncRequest)
			{
				return true;
			}
		}
		return false;
	},
											5s));
	EXPECT_FALSE(client.isSynchronized());
	(void)raw.sendTo(*connections.begin(), maliciousSnapshot(4, 16));
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.isSynchronized() && client.world().revision(key) == 4;
	},
											5s));
	client.disconnect();
	raw.stop();
}

TEST(VoxelNetworkTest, RejectsOversizedEditBatchesWithoutRevisionChanges)
{
	const auto definitions = registry();
	Net::Server server(Dimensions, definitions, Catalogue);
	const Position key{0, 0, 0};
	server.setChunk(key, volume(definitions));
	std::vector<Edit> excessive(Net::Protocol::MaxEdits + 1, edit(0, 0, 0, 7));
	EXPECT_THROW((void)server.edit(key, excessive), spk::Exception);
	EXPECT_EQ(server.world().revision(key), 1u);
}


TEST(VoxelNetworkTest, InitialReadyRejectsAnIncompleteChunkCollection)
{
	const auto definitions = registry();
	spk::Server raw;
	Net::Client client(Dimensions, definitions, Catalogue);
	raw.start(0);
	client.connect("127.0.0.1", raw.port());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.isConnected();
	}, 5s));
	const auto connections = raw.connections();
	ASSERT_EQ(connections.size(), 1u);

	// A malicious or failed initial transfer must never mark a partial world ready.
	(void)raw.sendTo(*connections.begin(), maliciousSnapshot(1, 16));
	spk::Message::Writer ready(Net::Protocol::Ready);
	put(ready, Net::Protocol::Version, 1);
	put(ready, 2, 8);
	(void)raw.sendTo(*connections.begin(), std::move(ready).build());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.lastError() == Net::Error::InvalidRequest;
	}, 5s));
	EXPECT_FALSE(client.isSynchronized());
	EXPECT_EQ(client.world().chunks().size(), 1u);
	client.disconnect();
	raw.stop();
}

TEST(VoxelNetworkTest, RepeatedEditsOnTheSameVoxelConverge)
{
	const auto definitions = registry();
	Net::Server server(Dimensions, definitions, Catalogue);
	Net::Client first(Dimensions, definitions, Catalogue);
	Net::Client second(Dimensions, definitions, Catalogue);
	const Position key{0, 0, 0};
	server.setChunk(key, volume(definitions));
	server.start(0);
	first.connect("127.0.0.1", server.port());
	second.connect("127.0.0.1", server.port());
	ASSERT_TRUE(pump(server, [&] { return first.isSynchronized() && second.isSynchronized(); }, first, second));
	(void)first.takeChangedChunks();
	(void)second.takeChangedChunks();
	for (const auto material : {7u, 12u, 0u, 7u, 12u})
	{
		ASSERT_TRUE(server.edit(key, std::array{edit(0, 0, 0, material)}));
	}
	ASSERT_TRUE(pump(server, [&] {
		return first.world().revision(key) == 6 && second.world().revision(key) == 6;
	}, first, second));
	EXPECT_TRUE(same(*server.world().chunk(key), *first.world().chunk(key)));
	EXPECT_TRUE(same(*server.world().chunk(key), *second.world().chunk(key)));
	EXPECT_EQ(first.takeChangedChunks(), std::vector<Position>{key});
	EXPECT_EQ(second.takeChangedChunks(), std::vector<Position>{key});
	first.disconnect();
	second.disconnect();
	server.stop();
}

TEST(VoxelNetworkTest, StalePatchCannotRollbackNewerSnapshot)
{
	const auto definitions = registry();
	spk::Server raw;
	Net::Client client(Dimensions, definitions, Catalogue);
	const Position key{0, 0, 0};
	raw.start(0);
	client.connect("127.0.0.1", raw.port());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.isConnected();
	}, 5s));
	const auto connections = raw.connections();
	ASSERT_EQ(connections.size(), 1u);
	(void)raw.sendTo(*connections.begin(), maliciousSnapshot(5, 16));
	spk::Message::Writer ready(Net::Protocol::Ready);
	put(ready, Net::Protocol::Version, 1);
	put(ready, 1, 8);
	(void)raw.sendTo(*connections.begin(), std::move(ready).build());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.isSynchronized() && client.world().revision(key) == 5;
	}, 5s));
	spk::Message::Writer stale(Net::Protocol::Patch);
	put(stale, Net::Protocol::Version, 1);
	put(stale, 0, 12);
	put(stale, 3, 8);
	put(stale, 4, 8);
	put(stale, 1, 4);
	put(stale, 0, 12);
	put(stale, edit(0, 0, 0, 12).value, 4);
	(void)raw.sendTo(*connections.begin(), std::move(stale).build());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.world().revision(key) == 5;
	}, 5s));
	EXPECT_TRUE(client.isSynchronized());
	EXPECT_EQ(client.world().chunk(key)->atRaw({0, 0, 0}), 0u);
	client.disconnect();
	raw.stop();
}

TEST(VoxelNetworkTest, InvalidNetworkEditPatchIsRejectedAtomically)
{
	const auto definitions = registry();
	spk::Server raw;
	Net::Client client(Dimensions, definitions, Catalogue);
	const Position key{0, 0, 0};
	raw.start(0);
	client.connect("127.0.0.1", raw.port());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.isConnected();
	}, 5s));
	const auto connections = raw.connections();
	ASSERT_EQ(connections.size(), 1u);
	(void)raw.sendTo(*connections.begin(), maliciousSnapshot(1, 16));
	spk::Message::Writer ready(Net::Protocol::Ready);
	put(ready, Net::Protocol::Version, 1);
	put(ready, 1, 8);
	(void)raw.sendTo(*connections.begin(), std::move(ready).build());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.isSynchronized();
	}, 5s));

	// The first valid edit must not be committed if the second edit is invalid.
	spk::Message::Writer malformed(Net::Protocol::Patch);
	put(malformed, Net::Protocol::Version, 1);
	put(malformed, 0, 12);
	put(malformed, 1, 8);
	put(malformed, 2, 8);
	put(malformed, 2, 4);
	put(malformed, 0, 12);
	put(malformed, edit(0, 0, 0, 7).value, 4);
	put(malformed, 1, 4);
	put(malformed, 0, 4);
	put(malformed, 0, 4);
	put(malformed, edit(1, 0, 0, 99).value, 4);
	(void)raw.sendTo(*connections.begin(), std::move(malformed).build());
	ASSERT_TRUE(NetworkTestUtils::waitUntil([&] {
		client.update();
		return client.lastError() == Net::Error::InvalidRequest;
	}, 5s));
	EXPECT_EQ(client.world().revision(key), 1u);
	EXPECT_EQ(client.world().chunk(key)->atRaw({0, 0, 0}), 0u);
	client.disconnect();
	raw.stop();
}
