#include "design_pattern/publishable.hpp"

#include "exception.hpp"

#include <exception>
#include <limits>
#include <unordered_set>
#include <utility>

namespace spk
{
	struct Publishable::Registration
	{
		Part *part;
		Flags<Publication> flags;
		bool modified;
		bool received = false;
		std::uint64_t receivedGeneration = 0;

		Registration(Part &registeredPart, Flags<Publication> publicationFlags) :
			part(&registeredPart),
			flags(publicationFlags),
			modified(publicationFlags.has(Publication::Update))
		{
		}
	};

	Publishable::Part::~Part()
	{
		if (_owner != nullptr)
		{
			_owner->_forget(*this);
		}
	}

	void Publishable::Part::invalidate()
	{
		if (_owner != nullptr && _owner->_mode == Mode::Replicant)
		{
			return;
		}
		if (_generation == std::numeric_limits<std::uint64_t>::max())
		{
			throw spk::Exception("Publishable Part generation overflow.");
		}

		++_generation;
		_cachedSnapshot.reset();
		if (_owner != nullptr)
		{
			_owner->_markModified(*this);
		}
	}

	std::pair<std::uint64_t, ByteStream> Publishable::Part::_snapshot() const
	{
		if (!_cachedSnapshot.has_value())
		{
			ByteStream::Writer writer;
			_encode(writer);
			_cachedSnapshot.emplace(std::move(writer).build());
		}
		return {_generation, *_cachedSnapshot};
	}

	Publishable::Publishable(Mode mode) :
		_mode(mode)
	{
		if (mode != Mode::Source && mode != Mode::Replicant)
		{
			throw spk::Exception("Invalid Publishable mode.");
		}
	}

	Publishable::~Publishable()
	{
		for (const auto &[id, registration] : _registrations)
		{
			registration->part->_owner = nullptr;
		}
	}

	Publishable::Mode Publishable::mode() const noexcept
	{
		return _mode;
	}

	void Publishable::_markModified(Part &part) noexcept
	{
		for (const auto &[id, registration] : _registrations)
		{
			if (registration->part == &part && registration->flags.has(Publication::Update))
			{
				registration->modified = true;
			}
		}
	}

	void Publishable::_forget(Part &part) noexcept
	{
		for (auto it = _registrations.begin(); it != _registrations.end();)
		{
			if (it->second->part == &part)
			{
				it = _registrations.erase(it);
			}
			else
			{
				++it;
			}
		}
		part._owner = nullptr;
	}

	void Publishable::setPartRegistry(PartRegistry registrations)
	{
		std::map<PartID, std::shared_ptr<Registration>> replacement;
		std::unordered_set<Part *> uniqueParts;
		for (const PartRegistration &item : registrations)
		{
			if (item.flags.raw() & ~static_cast<Flags<Publication>::Mask>(Publication::Update))
			{
				throw spk::Exception("Invalid Publishable publication flags.");
			}
			if (!uniqueParts.insert(&item.part).second)
			{
				throw spk::Exception("Publishable Part registered more than once.");
			}
			if (item.part._owner != nullptr && item.part._owner != this)
			{
				throw spk::Exception("Publishable Part belongs to another Publishable.");
			}
			if (replacement.contains(item.id))
			{
				throw spk::Exception("Duplicate Publishable Part ID.");
			}

			auto previous = _registrations.find(item.id);
			if (previous != _registrations.end() &&
				previous->second->part == &item.part &&
				previous->second->flags.raw() == item.flags.raw())
			{
				replacement.emplace(item.id, previous->second);
			}
			else
			{
				replacement.emplace(item.id, std::make_shared<Registration>(item.part, item.flags));
			}
		}

		for (const auto &[id, registration] : _registrations)
		{
			if (!uniqueParts.contains(registration->part))
			{
				registration->part->_owner = nullptr;
			}
		}
		for (const PartRegistration &item : registrations)
		{
			item.part._owner = this;
		}
		_registrations.swap(replacement);
	}

	namespace
	{
		constexpr std::uint8_t InitializationMode = 0;
		constexpr std::uint8_t UpdateMode = 1;

		struct Section
		{
			Publishable::PartID id;
			std::uint64_t generation;
			ByteStream bytes;
		};

		[[nodiscard]] ByteStream encodeSections(std::uint8_t mode, const std::vector<Section> &sections)
		{
			if (sections.empty())
			{
				return {};
			}
			if (sections.size() > std::numeric_limits<std::uint32_t>::max())
			{
				throw spk::Exception("Publishable section count overflow.");
			}

			ByteStream::Writer writer;
			writer << mode << static_cast<std::uint32_t>(sections.size());
			for (const Section &section : sections)
			{
				writer << section.id << section.generation << static_cast<std::uint64_t>(section.bytes.size());
				writer.append(section.bytes.data().data(), section.bytes.size());
			}
			return std::move(writer).build();
		}
	}

	ByteStream Publishable::encodeInitialization() const
	{
		const auto registrations = _registrations;
		std::vector<Section> sections;
		sections.reserve(registrations.size());
		for (const auto &[id, registration] : registrations)
		{
			auto [generation, bytes] = registration->part->_snapshot();
			sections.push_back({id, generation, std::move(bytes)});
		}
		return encodeSections(InitializationMode, sections);
	}

	ByteStream Publishable::encodeUpdate(UpdateSelection selection) const
	{
		if (selection != UpdateSelection::Modified && selection != UpdateSelection::All)
		{
			throw spk::Exception("Invalid Publishable update selection.");
		}
		const auto registrations = _registrations;
		std::vector<Section> sections;
		std::vector<std::pair<std::shared_ptr<Registration>, std::uint64_t>> captured;
		for (const auto &[id, registration] : registrations)
		{
			if (!registration->flags.has(Publication::Update) ||
				(selection == UpdateSelection::Modified && !registration->modified))
			{
				continue;
			}
			auto [generation, bytes] = registration->part->_snapshot();
			sections.push_back({id, generation, std::move(bytes)});
			captured.emplace_back(registration, generation);
		}

		ByteStream result = encodeSections(UpdateMode, sections);
		if (selection == UpdateSelection::Modified)
		{
			for (const auto &[registration, generation] : captured)
			{
				if (registration->part->_generation == generation)
				{
					registration->modified = false;
				}
			}
		}
		return result;
	}

	void Publishable::decode(const ByteStream::Reader &reader)
	{
		if (reader.remaining() == 0)
		{
			return;
		}
		const auto registrations = _registrations;
		std::uint8_t mode;
		std::uint32_t count;
		reader >> mode >> count;
		if (mode != InitializationMode && mode != UpdateMode)
		{
			throw spk::Exception("Invalid Publishable publication mode.");
		}
		constexpr std::size_t SectionHeaderSize = sizeof(PartID) + 2 * sizeof(std::uint64_t);
		if (count > reader.remaining() / SectionHeaderSize)
		{
			throw spk::Exception("Truncated Publishable section headers.");
		}

		struct Parsed
		{
			std::shared_ptr<Registration> registration;
			std::uint64_t generation;
			std::size_t begin;
			std::size_t end;
		};
		std::vector<Parsed> sections;
		std::unordered_set<PartID> seen;
		sections.reserve(count);
		for (std::uint32_t index = 0; index < count; ++index)
		{
			PartID id;
			std::uint64_t generation;
			std::uint64_t length;
			reader >> id >> generation >> length;
			if (!seen.insert(id).second)
			{
				throw spk::Exception("Duplicate Publishable Part ID in message.");
			}
			if (length > reader.remaining())
			{
				throw spk::Exception("Publishable payload exceeds message bounds.");
			}
			const auto binding = registrations.find(id);
			if (binding == registrations.end())
			{
				throw spk::Exception("Unknown Publishable Part ID.");
			}
			const std::size_t begin = reader.readOffset();
			sections.push_back({binding->second, generation, begin, begin + static_cast<std::size_t>(length)});
			reader.skip(static_cast<std::size_t>(length));
		}
		if (reader.remaining() != 0)
		{
			throw spk::Exception("Trailing bytes in Publishable message.");
		}

		std::exception_ptr firstFailure;
		for (const Parsed &section : sections)
		{
			auto &registration = *section.registration;
			if (registration.received && section.generation <= registration.receivedGeneration)
			{
				continue;
			}
			try
			{
				const auto payload = reader.subreader(section.begin, section.end);
				registration.part->_decode(payload);
				if (payload.remaining() != 0)
				{
					throw spk::Exception("Publishable Part payload not fully consumed.");
				}
				registration.receivedGeneration = section.generation;
				registration.received = true;
				registration.part->_cachedSnapshot.reset();
			} catch (...)
			{
				if (!firstFailure)
				{
					firstFailure = std::current_exception();
				}
			}
		}
		if (firstFailure)
		{
			std::rethrow_exception(firstFailure);
		}
	}
}
