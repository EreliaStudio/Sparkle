#pragma once

#include "network/client.hpp"
#include "network/server.hpp"
#include "voxel/world.hpp"

#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace spk::Voxel::Network
{
	// Application-level protocol over Sparkle's existing reliable TCP transport.
	// Message payloads use explicit little-endian integers, never C++ object layouts.
	struct Protocol final
	{
		static constexpr std::uint8_t Version = 1;
		static constexpr spk::Message::Type Hello = 0x56580001;
		static constexpr spk::Message::Type Snapshot = 0x56580002;
		static constexpr spk::Message::Type Patch = 0x56580003;
		static constexpr spk::Message::Type EditRequest = 0x56580004;
		static constexpr spk::Message::Type Ready = 0x56580005;
		static constexpr spk::Message::Type ResyncRequest = 0x56580006;
		static constexpr spk::Message::Type Error = 0x56580007;
		static constexpr std::size_t MaxMessageBytes = 1024 * 1024;
		static constexpr std::uint32_t MaxEdits = 4096;
	};

	enum class Error : std::uint8_t
	{
		None = 0,
		ProtocolMismatch = 1,
		CatalogueMismatch = 2,
		InvalidRequest = 3
	};

	// Drive all public methods from a single world/update thread; TCP I/O workers
	// only publish immutable messages into the underlying receive queue.
	class Server final
	{
	public:
		using EditValidator = std::function<bool(spk::ConnectionID, ChunkPosition, std::span<const CellEdit>)>;

	private:
		spk::Server _transport;
		World _world;
		std::uint64_t _catalogueId;
		std::unordered_set<spk::ConnectionID> _subscribed;
		std::vector<spk::ReceivedMessage> _received;
		EditValidator _validator;

		void _send(spk::ConnectionID connection, const spk::Message &message);
		void _sendSnapshot(spk::ConnectionID connection, ChunkPosition position);
		void _broadcast(const spk::Message &message);
		void _process(const spk::ReceivedMessage &received);

	public:
		Server(spk::Vector3UInt chunkDimensions, Definition::Registry definitions, std::uint64_t catalogueId);

		void start(std::uint16_t port);
		void stop();
		[[nodiscard]] std::uint16_t port() const noexcept;
		void update();
		void setEditValidator(EditValidator validator);
		void setChunk(ChunkPosition position, Volume volume);
		// Returns false if a valid batch makes no logical change.
		[[nodiscard]] bool edit(ChunkPosition position, std::span<const CellEdit> edits);
		[[nodiscard]] const World &world() const noexcept;
		[[nodiscard]] std::vector<ChunkPosition> takeChangedChunks();
	};

	class Client final
	{
	private:
		spk::Client _transport;
		World _world;
		std::uint64_t _catalogueId;
		bool _helloSent = false;
		bool _ready = false;
		Error _error = Error::None;
		std::unordered_set<std::uint64_t> _unusedReserved; // Reserved for future request correlation.
		std::vector<spk::Message> _received;
		std::vector<ChunkPosition> _resyncing;

		void _requestResync(ChunkPosition position);
		void _process(const spk::Message &message);

	public:
		Client(spk::Vector3UInt chunkDimensions, Definition::Registry definitions, std::uint64_t catalogueId);

		void connect(std::string_view address, std::uint16_t port);
		void disconnect();
		void update();
		[[nodiscard]] bool isConnected() const noexcept;
		[[nodiscard]] bool isSynchronized() const noexcept;
		[[nodiscard]] Error lastError() const noexcept;
		void requestEdit(ChunkPosition position, std::span<const CellEdit> edits);
		[[nodiscard]] const World &world() const noexcept;
		[[nodiscard]] std::vector<ChunkPosition> takeChangedChunks();
	};
}
