#include "design_pattern/publishable.hpp"

#include "container/double_buffer.hpp"
#include "exception.hpp"

#include <algorithm>
#include <atomic>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace spk
{
    struct Publishable::Registration
    {
        PartID id;
        Part *part;
        Flags<Publication> flags;
        std::atomic<bool> active = true;
    };

    struct Publishable::State
    {
        mutable std::mutex registrationsMutex;
        mutable std::mutex editionMutex;
        std::unordered_map<PartID, std::shared_ptr<Registration>> registrations;
        mutable DoubleBuffer<std::unordered_set<std::shared_ptr<Registration>>> edited;

        void markEdited(const std::shared_ptr<Registration> &registration)
        {
            if (!registration->active.load(std::memory_order_acquire) ||
                !registration->flags.has(Publication::Edition))
            {
                return;
            }

            edited.modify([&](auto &pending) {
                pending.insert(registration);
            });
        }

        void detach(const std::shared_ptr<Registration> &registration) noexcept
        {
            const std::scoped_lock lock(registrationsMutex);
            registration->active.store(false, std::memory_order_release);
            const auto it = registrations.find(registration->id);
            if (it != registrations.end() && it->second == registration)
            {
                registrations.erase(it);
            }
        }
    };

    Publishable::Part::Part() :
        _snapshot{
            CachedData<ByteStream>([this]() { return _buildEdition(); }),
            CachedData<ByteStream>([this]() { return _buildUpdate(); })}
    {
    }

    Publishable::Part::~Part()
    {
        std::vector<Subscriber> subscribers;
        {
            const std::scoped_lock lock(_cacheMutex);
            subscribers.swap(_subscribers);
        }

        for (const Subscriber &subscriber : subscribers)
        {
            const auto state = subscriber.state.lock();
            const auto registration = subscriber.registration.lock();
            if (state && registration)
            {
                state->detach(registration);
            }
        }
    }

    void Publishable::Part::invalidate()
    {
        std::vector<Subscriber> subscribers;
        {
            const std::scoped_lock lock(_cacheMutex);
            _snapshot.edition.invalidate();
            _snapshot.update.invalidate();
            subscribers = _subscribers;
        }

        for (const Subscriber &subscriber : subscribers)
        {
            const auto state = subscriber.state.lock();
            const auto registration = subscriber.registration.lock();
            if (state && registration)
            {
                state->markEdited(registration);
            }
        }
    }

    ByteStream Publishable::Part::_buildEdition() const
    {
        ByteStream::Writer writer;
        _encode(writer);
        return std::move(writer).build();
    }

    ByteStream Publishable::Part::_buildUpdate() const
    {
        ByteStream::Writer writer;
        _encodeUpdate(writer);
        return std::move(writer).build();
    }

    ByteStream Publishable::Part::_editionSnapshot() const
    {
        const std::scoped_lock lock(_cacheMutex);
        return _snapshot.edition.get();
    }

    ByteStream Publishable::Part::_updateSnapshot() const
    {
        const std::scoped_lock lock(_cacheMutex);
        return _snapshot.update.get();
    }

    void Publishable::Part::_subscribe(
        const std::shared_ptr<State> &state,
        const std::shared_ptr<Registration> &registration)
    {
        const std::scoped_lock lock(_cacheMutex);
        _subscribers.push_back({state, registration});
    }

    void Publishable::Part::_unsubscribe(const std::shared_ptr<Registration> &registration) noexcept
    {
        const std::scoped_lock lock(_cacheMutex);
        std::erase_if(_subscribers, [&](const Subscriber &subscriber) {
            return subscriber.registration.lock() == registration;
        });
    }

    void Publishable::Part::_encodeUpdate(ByteStream::Writer &writer) const
    {
        _encode(writer);
    }

    void Publishable::Part::_decodeUpdate(const ByteStream::Reader &reader)
    {
        _decode(reader);
    }

    Publishable::Publishable() :
        _state(std::make_shared<State>())
    {
    }

    Publishable::~Publishable()
    {
        std::unordered_map<PartID, std::shared_ptr<Registration>> registrations;
        {
            const std::scoped_lock lock(_state->registrationsMutex);
            registrations.swap(_state->registrations);
            for (const auto &[id, registration] : registrations)
            {
                registration->active.store(false, std::memory_order_release);
            }
        }

        for (const auto &[id, registration] : registrations)
        {
            registration->part->_unsubscribe(registration);
        }
    }

    void Publishable::add(PartID id, Part *part, Flags<Publication> flags)
    {
        if (part == nullptr)
        {
            throw spk::Exception("Cannot register a null Publishable Part.");
        }
        if ((flags.raw() & ~static_cast<Flags<Publication>::Mask>(3)) != 0)
        {
            throw spk::Exception("Invalid Publishable publication flags.");
        }

        auto registration = std::make_shared<Registration>(id, part, flags);
        {
            const std::scoped_lock lock(_state->registrationsMutex);
            if (_state->registrations.contains(id))
            {
                throw spk::Exception("Publishable Part ID is already registered.");
            }
            for (const auto &[otherID, existing] : _state->registrations)
            {
                if (existing->part == part)
                {
                    throw spk::Exception("Publishable Part is already registered.");
                }
            }
            _state->registrations.emplace(id, registration);
        }

        try
        {
            part->_subscribe(_state, registration);
        }
        catch (...)
        {
            _state->detach(registration);
            throw;
        }
    }

    void Publishable::remove(PartID id)
    {
        std::shared_ptr<Registration> registration;
        {
            const std::scoped_lock lock(_state->registrationsMutex);
            const auto it = _state->registrations.find(id);
            if (it == _state->registrations.end())
            {
                throw spk::Exception("Cannot remove an unknown Publishable Part ID.");
            }
            registration = it->second;
            registration->active.store(false, std::memory_order_release);
            _state->registrations.erase(it);
        }
        registration->part->_unsubscribe(registration);
    }

    bool Publishable::contains(PartID id) const
    {
        const std::scoped_lock lock(_state->registrationsMutex);
        return _state->registrations.contains(id);
    }

    namespace
    {
        using Mode = std::uint8_t;
        constexpr Mode InitializationMode = 0;
        constexpr Mode EditionMode = 1;
        constexpr Mode UpdateMode = 2;

        struct Section
        {
            Publishable::PartID id;
            ByteStream payload;
        };

        [[nodiscard]] ByteStream encodeSections(Mode mode, const std::vector<Section> &sections)
        {
            if (sections.empty())
            {
                return {};
            }
            if (sections.size() > std::numeric_limits<std::uint32_t>::max())
            {
                throw spk::Exception("Publishable Part count exceeds the binary format.");
            }

            ByteStream::Writer writer;
            writer << mode << static_cast<std::uint32_t>(sections.size());
            for (const Section &section : sections)
            {
                writer << section.id << static_cast<std::uint64_t>(section.payload.size());
                writer.append(section.payload.data().data(), section.payload.size());
            }
            return std::move(writer).build();
        }
    }

    ByteStream Publishable::encodeInitialization() const
    {
        std::vector<std::shared_ptr<Registration>> registrations;
        {
            const std::scoped_lock lock(_state->registrationsMutex);
            for (const auto &[id, registration] : _state->registrations)
            {
                registrations.push_back(registration);
            }
        }

        std::sort(registrations.begin(), registrations.end(), [](const auto &lhs, const auto &rhs) {
            return lhs->id < rhs->id;
        });

        std::vector<Section> sections;
        sections.reserve(registrations.size());
        for (const auto &registration : registrations)
        {
            sections.push_back({registration->id, registration->part->_editionSnapshot()});
        }
        return encodeSections(InitializationMode, sections);
    }

    ByteStream Publishable::encodeEdited() const
    {
        const std::scoped_lock editionLock(_state->editionMutex);
        auto &batch = _state->edited.read();
        if (batch.empty())
        {
            _state->edited.swap();
        }

        std::vector<std::shared_ptr<Registration>> registrations;
        registrations.reserve(batch.size());
        for (const auto &registration : batch)
        {
            if (registration->active.load(std::memory_order_acquire))
            {
                registrations.push_back(registration);
            }
        }
        std::sort(registrations.begin(), registrations.end(), [](const auto &lhs, const auto &rhs) {
            return lhs->id < rhs->id;
        });

        std::vector<Section> sections;
        sections.reserve(registrations.size());
        for (const auto &registration : registrations)
        {
            sections.push_back({registration->id, registration->part->_editionSnapshot()});
        }

        ByteStream result = encodeSections(EditionMode, sections);
        batch.clear();
        return result;
    }

    ByteStream Publishable::encodeUpdate() const
    {
        std::vector<std::shared_ptr<Registration>> registrations;
        {
            const std::scoped_lock lock(_state->registrationsMutex);
            for (const auto &[id, registration] : _state->registrations)
            {
                if (registration->flags.has(Publication::Update))
                {
                    registrations.push_back(registration);
                }
            }
        }
        std::sort(registrations.begin(), registrations.end(), [](const auto &lhs, const auto &rhs) {
            return lhs->id < rhs->id;
        });

        std::vector<Section> sections;
        sections.reserve(registrations.size());
        for (const auto &registration : registrations)
        {
            sections.push_back({registration->id, registration->part->_updateSnapshot()});
        }
        return encodeSections(UpdateMode, sections);
    }

    void Publishable::decode(const ByteStream::Reader &reader, UnknownPartBehaviour behaviour)
    {
        if (behaviour != UnknownPartBehaviour::Throw && behaviour != UnknownPartBehaviour::Skip)
        {
            throw spk::Exception("Invalid unknown-Part decoding behaviour.");
        }
        if (reader.remaining() == 0)
        {
            return;
        }

        Mode mode;
        std::uint32_t count;
        reader >> mode >> count;
        if (mode != InitializationMode && mode != EditionMode && mode != UpdateMode)
        {
            throw spk::Exception("Invalid Publishable publication mode.");
        }

        constexpr std::size_t SectionHeaderSize = sizeof(PartID) + sizeof(std::uint64_t);
        if (count > reader.remaining() / SectionHeaderSize)
        {
            throw spk::Exception("Publishable collection is truncated.");
        }

        struct DecodedSection
        {
            Part *part;
            std::size_t begin;
            std::size_t end;
        };

        std::vector<DecodedSection> sections;
        std::unordered_set<PartID> seen;
        sections.reserve(count);
        seen.reserve(count);

        for (std::uint32_t index = 0; index < count; ++index)
        {
            PartID id;
            std::uint64_t length;
            reader >> id >> length;
            if (!seen.insert(id).second)
            {
                throw spk::Exception("Duplicate Publishable Part ID in collection.");
            }
            if (length > reader.remaining())
            {
                throw spk::Exception("Publishable Part payload exceeds reader bounds.");
            }

            Part *part = nullptr;
            {
                const std::scoped_lock lock(_state->registrationsMutex);
                const auto it = _state->registrations.find(id);
                if (it != _state->registrations.end())
                {
                    part = it->second->part;
                }
            }
            if (part == nullptr && behaviour == UnknownPartBehaviour::Throw)
            {
                throw spk::Exception("Unknown Publishable Part ID.");
            }
            if (part != nullptr)
            {
                const std::size_t begin = reader.readOffset();
                sections.push_back({part, begin, begin + static_cast<std::size_t>(length)});
            }
            reader.skip(static_cast<std::size_t>(length));
        }

        for (const DecodedSection &section : sections)
        {
            const auto payload = reader.subreader(section.begin, section.end);
            if (mode == UpdateMode)
            {
                section.part->_decodeUpdate(payload);
            }
            else
            {
                section.part->_decode(payload);
            }
            if (payload.remaining() != 0)
            {
                throw spk::Exception("Publishable Part payload not fully consumed.");
            }
        }
    }
}
