#include "voxel/network.hpp"

#include "exception.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <set>
#include <tuple>
#include <utility>

namespace spk::Voxel::Network
{
	namespace
	{
		class Decoder final
		{
		private:
			spk::Message::Reader _reader;

		public:
			explicit Decoder(const spk::Message &message) :
				_reader(message.reader())
			{
				if (message.size() > Protocol::MaxMessageBytes)
				{
					throw spk::Exception("Voxel network payload exceeds message limit");
				}
			}

			[[nodiscard]] std::uint64_t number(unsigned count)
			{
				if (count > 8 || _reader.remaining() < count)
				{
					throw spk::Exception("Truncated voxel network message");
				}
				std::uint64_t result = 0;
				for (unsigned i = 0; i < count; ++i)
				{
					std::byte octet{};
					_reader.pull(&octet, 1);
					result |= std::uint64_t(std::to_integer<std::uint8_t>(octet)) << (8 * i);
				}
				return result;
			}

			[[nodiscard]] std::uint8_t u8()
			{
				return static_cast<std::uint8_t>(number(1));
			}

			[[nodiscard]] std::uint32_t u32()
			{
				return static_cast<std::uint32_t>(number(4));
			}

			[[nodiscard]] std::uint64_t u64()
			{
				return number(8);
			}

			void finish() const
			{
				if (_reader.remaining() != 0)
				{
					throw spk::Exception("Voxel network message contains trailing bytes");
				}
			}
		};

		void put(spk::Message::Writer &writer, std::uint64_t value, unsigned count)
		{
			std::array<std::byte, 8> bytes{};
			for (unsigned i = 0; i < count; ++i)
			{
				bytes[i] = std::byte((value >> (8 * i)) & 0xffu);
			}
			writer.append(bytes.data(), count);
		}

		void putPosition(spk::Message::Writer &writer, ChunkPosition position)
		{
			put(writer, std::bit_cast<std::uint32_t>(position.x), 4);
			put(writer, std::bit_cast<std::uint32_t>(position.y), 4);
			put(writer, std::bit_cast<std::uint32_t>(position.z), 4);
		}

		[[nodiscard]] ChunkPosition readPosition(Decoder &decoder)
		{
			const auto x = decoder.u32();
			const auto y = decoder.u32();
			const auto z = decoder.u32();
			return {std::bit_cast<std::int32_t>(x), std::bit_cast<std::int32_t>(y), std::bit_cast<std::int32_t>(z)};
		}

		[[nodiscard]] bool sameDimensions(spk::Vector3UInt lhs, spk::Vector3UInt rhs) noexcept
		{
			return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
		}

		[[nodiscard]] std::uint32_t cellCount(spk::Vector3UInt dimensions)
		{
			return dimensions.x * dimensions.y * dimensions.z;
		}

		void checkEdits(const World &world, const std::vector<CellEdit> &edits)
		{
			if (edits.empty() || edits.size() > Protocol::MaxEdits)
			{
				throw spk::Exception("Voxel network edit count is out of bounds");
			}
			std::set<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>> positions;
			const auto dimensions = world.chunkDimensions();
			for (const auto &edit : edits)
			{
				if (edit.position.x >= dimensions.x || edit.position.y >= dimensions.y || edit.position.z >= dimensions.z ||
					!world.definitions().contains(Volume::PackedCell::definitionId(edit.value)) ||
					!positions.emplace(edit.position.x, edit.position.y, edit.position.z).second)
				{
					throw spk::Exception("Voxel network edit contains an invalid position or Definition ID");
				}
			}
		}

		[[nodiscard]] std::vector<CellEdit> readEdits(Decoder &decoder, const World &world)
		{
			const auto count = decoder.u32();
			if (count == 0 || count > Protocol::MaxEdits)
			{
				throw spk::Exception("Voxel network edit count is out of bounds");
			}
			std::vector<CellEdit> edits;
			edits.reserve(count);
			for (std::uint32_t i = 0; i < count; ++i)
			{
				const auto x = decoder.u32();
				const auto y = decoder.u32();
				const auto z = decoder.u32();
				const auto raw = decoder.u32();
				edits.push_back({{x, y, z}, raw});
			}
			checkEdits(world, edits);
			return edits;
		}

		void putEdits(spk::Message::Writer &writer, const std::vector<CellEdit> &edits)
		{
			put(writer, edits.size(), 4);
			for (const auto &edit : edits)
			{
				put(writer, edit.position.x, 4);
				put(writer, edit.position.y, 4);
				put(writer, edit.position.z, 4);
				put(writer, edit.value, 4);
			}
		}

		[[nodiscard]] spk::Message errorMessage(Error error)
		{
			spk::Message::Writer writer(Protocol::Error);
			put(writer, Protocol::Version, 1);
			put(writer, static_cast<std::uint8_t>(error), 1);
			return std::move(writer).build();
		}

		[[nodiscard]] spk::Message snapshot(ChunkPosition position, std::uint64_t revision, const Volume &volume)
		{
			spk::Message::Writer writer(Protocol::Snapshot);
			put(writer, Protocol::Version, 1);
			putPosition(writer, position);
			put(writer, revision, 8);
			const auto dimensions = volume.dimensions();
			put(writer, cellCount(dimensions), 4);
			for (std::uint32_t z = 0; z < dimensions.z; ++z)
			{
				for (std::uint32_t y = 0; y < dimensions.y; ++y)
				{
					for (std::uint32_t x = 0; x < dimensions.x; ++x)
					{
						put(writer, volume.atRaw({x, y, z}), 4);
					}
				}
			}
			return std::move(writer).build();
		}

		[[nodiscard]] spk::Message patch(
			ChunkPosition position, std::uint64_t previous, std::span<const CellEdit> changes)
		{
			spk::Message::Writer writer(Protocol::Patch);
			put(writer, Protocol::Version, 1);
			putPosition(writer, position);
			put(writer, previous, 8);
			put(writer, previous + 1, 8);
			put(writer, changes.size(), 4);
			for (const auto &edit : changes)
			{
				put(writer, edit.position.x, 4);
				put(writer, edit.position.y, 4);
				put(writer, edit.position.z, 4);
				put(writer, edit.value, 4);
			}
			return std::move(writer).build();
		}
	}

	Server::Server(spk::Vector3UInt dimensions, Definition::Registry definitions, std::uint64_t catalogueId) :
		_world(dimensions, std::move(definitions)),
		_catalogueId(catalogueId)
	{
		if (_catalogueId == 0)
		{
			throw spk::Exception("Voxel networking requires an explicit nonzero catalogue ID");
		}
		_transport.setMaxMessageBytes(Protocol::MaxMessageBytes);
	}

	void Server::start(std::uint16_t port)
	{
		_transport.start(port);
	}

	void Server::stop()
	{
		_transport.stop();
		_subscribed.clear();
	}

	std::uint16_t Server::port() const noexcept
	{
		return _transport.port();
	}

	void Server::setEditValidator(EditValidator validator)
	{
		_validator = std::move(validator);
	}

	const World &Server::world() const noexcept
	{
		return _world;
	}

	std::vector<ChunkPosition> Server::takeChangedChunks()
	{
		return _world.takeChangedChunks();
	}

	bool Server::_send(spk::ConnectionID recipient, const spk::Message &message)
	{
		try
		{
			const auto transfer = _transport.sendTo(_transport.connection(recipient), message);
			const auto status = transfer.status();
			if (status == spk::Transfer::Status::Pending ||
				status == spk::Transfer::Status::TransportAcknowledged)
			{
				return true;
			}
		} catch (const spk::Exception &)
		{
		}
		_subscribed.erase(recipient);
		return false;
	}

	void Server::_broadcast(const spk::Message &message)
	{
		const std::vector<spk::ConnectionID> recipients(_subscribed.begin(), _subscribed.end());
		for (const auto id : recipients)
		{
			_send(id, message);
		}
	}

	bool Server::_sendSnapshot(spk::ConnectionID recipient, ChunkPosition position)
	{
		const auto *volume = _world.chunk(position);
		return volume != nullptr && _send(recipient, snapshot(position, _world.revision(position), *volume));
	}

	void Server::setChunk(ChunkPosition position, Volume volume)
	{
		const auto previous = _world.revision(position);
		if (previous == std::numeric_limits<std::uint64_t>::max())
		{
			throw spk::Exception("Voxel network chunk revision overflow");
		}
		_world._replace(position, std::move(volume), previous + 1);
		_broadcast(snapshot(position, _world.revision(position), *_world.chunk(position)));
	}

	bool Server::edit(ChunkPosition position, std::span<const CellEdit> edits)
	{
		const std::vector<CellEdit> batch(edits.begin(), edits.end());
		checkEdits(_world, batch);
		const auto previous = _world.revision(position);
		const bool changed = _world._apply(position, batch);
		if (changed)
		{
			_broadcast(patch(position, previous, edits));
		}
		return changed;
	}

	void Server::_process(const spk::ReceivedMessage &incoming)
	{
		const auto sender = incoming.emitter;
		const auto &message = incoming.message;
		const auto type = message.type();
		if (type != Protocol::Hello && type != Protocol::EditRequest && type != Protocol::ResyncRequest)
		{
			return;
		}
		try
		{
			Decoder decoder(message);
			if (decoder.u8() != Protocol::Version)
			{
				_send(sender, errorMessage(Error::ProtocolMismatch));
				return;
			}
			if (type == Protocol::Hello)
			{
				const auto catalogue = decoder.u64();
				const spk::Vector3UInt dimensions{decoder.u32(), decoder.u32(), decoder.u32()};
				decoder.finish();
				if (catalogue != _catalogueId || !sameDimensions(dimensions, _world.chunkDimensions()))
				{
					_send(sender, errorMessage(Error::CatalogueMismatch));
					return;
				}
				_subscribed.insert(sender);
				const auto positions = _world.chunks();
				for (const auto position : positions)
				{
					if (!_sendSnapshot(sender, position))
					{
						_subscribed.erase(sender);
						return;
					}
				}
				spk::Message::Writer ready(Protocol::Ready);
				put(ready, Protocol::Version, 1);
				put(ready, positions.size(), 8);
				(void)_send(sender, std::move(ready).build());
				return;
			}
			if (!_subscribed.contains(sender))
			{
				_send(sender, errorMessage(Error::InvalidRequest));
				return;
			}
			const auto position = readPosition(decoder);
			if (type == Protocol::ResyncRequest)
			{
				decoder.finish();
				if (_world.chunk(position) == nullptr)
				{
					_send(sender, errorMessage(Error::InvalidRequest));
					return;
				}
				_sendSnapshot(sender, position);
				return;
			}

			const auto expectedRevision = decoder.u64();
			auto edits = readEdits(decoder, _world);
			decoder.finish();
			if (_world.chunk(position) == nullptr)
			{
				_send(sender, errorMessage(Error::InvalidRequest));
				return;
			}
			if (_world.revision(position) != expectedRevision)
			{
				_sendSnapshot(sender, position);
				return;
			}
			if (_validator && !_validator(sender, position, edits))
			{
				_send(sender, errorMessage(Error::InvalidRequest));
				return;
			}
			(void)edit(position, edits);
		} catch (const spk::Exception &)
		{
			_send(sender, errorMessage(Error::InvalidRequest));
		}
	}

	void Server::update()
	{
		std::unordered_set<spk::ConnectionID> connected;
		for (const auto &connection : _transport.connections())
		{
			connected.insert(connection.id());
		}
		for (auto it = _subscribed.begin(); it != _subscribed.end();)
		{
			if (!connected.contains(*it))
			{
				it = _subscribed.erase(it);
			}
			else
			{
				++it;
			}
		}
		(void)_transport.messages().drain(_received);
		for (const auto &message : _received)
		{
			_process(message);
		}
		_received.clear();
	}

	Client::Client(spk::Vector3UInt dimensions, Definition::Registry definitions, std::uint64_t catalogueId) :
		_world(dimensions, std::move(definitions)),
		_catalogueId(catalogueId)
	{
		if (_catalogueId == 0)
		{
			throw spk::Exception("Voxel networking requires an explicit nonzero catalogue ID");
		}
		_transport.setMaxMessageBytes(Protocol::MaxMessageBytes);
	}

	void Client::connect(std::string_view address, std::uint16_t port)
	{
		_transport.disconnect();
		_world._clear();
		_helloSent = false;
		_ready = false;
		_error = Error::None;
		_resyncing.clear();
		(void)_transport.connect(address, port);
	}

	void Client::disconnect()
	{
		_transport.disconnect();
		_helloSent = false;
		_ready = false;
		_resyncing.clear();
	}

	bool Client::isConnected() const noexcept
	{
		return _transport.isConnected();
	}

	bool Client::isSynchronized() const noexcept
	{
		return isConnected() && _ready && _resyncing.empty();
	}

	Error Client::lastError() const noexcept
	{
		return _error;
	}

	const World &Client::world() const noexcept
	{
		return _world;
	}

	std::vector<ChunkPosition> Client::takeChangedChunks()
	{
		return _world.takeChangedChunks();
	}

	void Client::_requestResync(ChunkPosition position)
	{
		if (std::find(_resyncing.begin(), _resyncing.end(), position) != _resyncing.end())
		{
			return;
		}
		_resyncing.push_back(position);
		spk::Message::Writer writer(Protocol::ResyncRequest);
		put(writer, Protocol::Version, 1);
		putPosition(writer, position);
		(void)_transport.send(std::move(writer).build());
	}

	void Client::_process(const spk::Message &message)
	{
		const auto type = message.type();
		if (type != Protocol::Ready && type != Protocol::Snapshot && type != Protocol::Patch && type != Protocol::Error)
		{
			return;
		}
		try
		{
			Decoder decoder(message);
			if (decoder.u8() != Protocol::Version)
			{
				_error = Error::ProtocolMismatch;
				disconnect();
				return;
			}
			if (type == Protocol::Ready)
			{
				const auto expectedCount = decoder.u64();
				decoder.finish();
				if (_world.chunks().size() != expectedCount)
				{
					throw spk::Exception("Voxel network initial snapshot count does not match");
				}
				_ready = true;
				return;
			}
			if (type == Protocol::Error)
			{
				const auto value = decoder.u8();
				decoder.finish();
				if (value < static_cast<std::uint8_t>(Error::ProtocolMismatch) ||
					value > static_cast<std::uint8_t>(Error::InvalidRequest))
				{
					throw spk::Exception("Unknown voxel network error code");
				}
				_error = static_cast<Error>(value);
				if (_error != Error::InvalidRequest)
				{
					disconnect();
				}
				return;
			}

			const auto position = readPosition(decoder);
			if (type == Protocol::Snapshot)
			{
				const auto revision = decoder.u64();
				const auto count = decoder.u32();
				if (revision == 0 || count != cellCount(_world.chunkDimensions()))
				{
					throw spk::Exception("Invalid voxel chunk snapshot metadata");
				}
				Volume::Builder builder(_world.chunkDimensions());
				builder.bind(_world.definitions());
				const auto dimensions = _world.chunkDimensions();
				for (std::uint32_t z = 0; z < dimensions.z; ++z)
				{
					for (std::uint32_t y = 0; y < dimensions.y; ++y)
					{
						for (std::uint32_t x = 0; x < dimensions.x; ++x)
						{
							const auto raw = decoder.u32();
							const auto id = Volume::PackedCell::definitionId(raw);
							if (!_world.definitions().contains(id))
							{
								throw spk::Exception("Snapshot contains an unknown Voxel Definition ID");
							}
							builder.set({x, y, z}, id, Volume::PackedCell::horizontal(raw), Volume::PackedCell::vertical(raw));
						}
					}
				}
				decoder.finish();
				const bool requested = std::find(_resyncing.begin(), _resyncing.end(), position) != _resyncing.end();
				if (requested || revision > _world.revision(position))
				{
					_world._replace(position, builder.build(), revision);
				}
				std::erase(_resyncing, position);
				return;
			}

			const auto previous = decoder.u64();
			const auto next = decoder.u64();
			if (previous == 0 || previous == std::numeric_limits<std::uint64_t>::max() || next != previous + 1)
			{
				throw spk::Exception("Invalid voxel patch revision transition");
			}
			const auto edits = readEdits(decoder, _world);
			decoder.finish();
			if (std::find(_resyncing.begin(), _resyncing.end(), position) != _resyncing.end())
			{
				return;
			}
			const auto local = _world.revision(position);
			if (local >= next)
			{
				return;
			}
			if (local != previous || !_world._apply(position, edits))
			{
				_requestResync(position);
			}
		} catch (const spk::Exception &)
		{
			_error = Error::InvalidRequest;
			disconnect();
		}
	}

	void Client::update()
	{
		if (!_transport.isConnected())
		{
			_ready = false;
			_helloSent = false;
			return;
		}
		if (!_helloSent)
		{
			spk::Message::Writer writer(Protocol::Hello);
			put(writer, Protocol::Version, 1);
			put(writer, _catalogueId, 8);
			const auto dimensions = _world.chunkDimensions();
			put(writer, dimensions.x, 4);
			put(writer, dimensions.y, 4);
			put(writer, dimensions.z, 4);
			(void)_transport.send(std::move(writer).build());
			_helloSent = true;
		}
		(void)_transport.messages().drain(_received);
		for (const auto &message : _received)
		{
			if (!_transport.isConnected())
			{
				break;
			}
			_process(message);
		}
		_received.clear();
	}

	void Client::requestEdit(ChunkPosition position, std::span<const CellEdit> edits)
	{
		if (!isSynchronized() || _world.chunk(position) == nullptr)
		{
			throw spk::Exception("Voxel network client cannot edit before synchronization or an unknown chunk");
		}
		const std::vector<CellEdit> batch(edits.begin(), edits.end());
		checkEdits(_world, batch);
		spk::Message::Writer writer(Protocol::EditRequest);
		put(writer, Protocol::Version, 1);
		putPosition(writer, position);
		put(writer, _world.revision(position), 8);
		putEdits(writer, batch);
		(void)_transport.send(std::move(writer).build());
	}
}
