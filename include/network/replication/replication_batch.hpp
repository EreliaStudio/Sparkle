#pragma once

#include "container/byte_stream.hpp"
#include "exception.hpp"
#include "type/uuid.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace spk::Network
{
	class ReplicationBatch final
	{
	public:
		static constexpr std::uint32_t Magic = 0x53504B42;

		struct Component
		{
			spk::UUID identifier;
			std::uint64_t revision = 0;
			spk::ByteStream payload;
		};

		struct Record
		{
			spk::UUID identifier;
			std::uint64_t revision = 0;
			spk::ByteStream::Slice payload;
		};

		class Index final
		{
		private:
			std::uint32_t _componentCount = 0;
			std::uint32_t _componentsPerSection = 0;
			std::vector<std::size_t> _offsets;
			spk::ByteStream::Slice _data;

			friend class ReplicationBatch;
			Index() :
				_data(spk::ByteStream{}.reader())
			{
			}

		public:
			[[nodiscard]] std::size_t componentCount() const noexcept
			{
				return _componentCount;
			}

			[[nodiscard]] std::size_t sectionCount() const noexcept
			{
				return _offsets.size() - 1;
			}

			[[nodiscard]] std::size_t componentsPerSection() const noexcept
			{
				return _componentsPerSection;
			}

			[[nodiscard]] spk::ByteStream::Slice section(std::size_t index) const
			{
				if (index >= sectionCount())
				{
					throw spk::Exception("Replication section index outside bounds.");
				}
				return _data.slice(_offsets[index], _offsets[index + 1]);
			}

			[[nodiscard]] std::size_t count(std::size_t index) const
			{
				if (index >= sectionCount())
				{
					throw spk::Exception("Replication section index outside bounds.");
				}
				const std::size_t start = index * _componentsPerSection;
				return std::min<std::size_t>(_componentsPerSection, _componentCount - start);
			}
		};

		[[nodiscard]] static spk::ByteStream encode(
			std::span<const Component> components,
			std::uint32_t componentsPerSection)
		{
			if (componentsPerSection == 0 || components.size() > std::numeric_limits<std::uint32_t>::max())
			{
				throw spk::Exception("Invalid replication batch configuration.");
			}
			const std::size_t sectionCount =
				components.size() / componentsPerSection + (components.size() % componentsPerSection != 0);
			std::vector<std::uint64_t> offsets{0};
			std::uint64_t offset = 0;
			for (std::size_t index = 0; index < components.size(); ++index)
			{
				const auto size = components[index].payload.size();
				if (size > std::numeric_limits<std::uint32_t>::max() ||
					offset > std::numeric_limits<std::uint64_t>::max() - 28 ||
					size > std::numeric_limits<std::uint64_t>::max() - offset - 28)
				{
					throw spk::Exception("Replication component payload exceeds wire limits.");
				}
				offset += sizeof(spk::UUID::Storage) + sizeof(std::uint64_t) + sizeof(std::uint32_t) + size;
				if ((index + 1) % componentsPerSection == 0 || index + 1 == components.size())
				{
					offsets.push_back(offset);
				}
			}
			spk::ByteStream::Writer writer;
			writer << Magic << static_cast<std::uint32_t>(components.size()) << componentsPerSection
				   << static_cast<std::uint32_t>(sectionCount);
			for (std::uint64_t boundary : offsets)
			{
				writer << boundary;
			}
			for (const Component &component : components)
			{
				writer << component.identifier.bytes() << component.revision
					   << static_cast<std::uint32_t>(component.payload.size());
				writer.append(component.payload.data().data(), component.payload.size());
			}
			return std::move(writer).build();
		}

		[[nodiscard]] static Index decode(const spk::ByteStream &payload)
		{
			auto reader = payload.reader();
			std::uint32_t magic = 0;
			std::uint32_t count = 0;
			std::uint32_t perSection = 0;
			std::uint32_t sectionCount = 0;
			reader >> magic >> count >> perSection >> sectionCount;
			if (magic != Magic || perSection == 0 ||
				sectionCount != count / perSection + (count % perSection != 0) ||
				reader.remaining() / sizeof(std::uint64_t) < std::size_t(sectionCount) + 1)
			{
				throw spk::Exception("Invalid replication section header.");
			}
			Index index;
			index._componentCount = count;
			index._componentsPerSection = perSection;
			index._offsets.reserve(std::size_t(sectionCount) + 1);
			for (std::size_t i = 0; i <= sectionCount; ++i)
			{
				std::uint64_t boundary = 0;
				reader >> boundary;
				if (boundary > std::numeric_limits<std::size_t>::max())
				{
					throw spk::Exception("Replication section offset exceeds platform limits.");
				}
				index._offsets.push_back(static_cast<std::size_t>(boundary));
			}
			index._data = reader.slice(reader.readOffset(), reader.size());
			if (index._offsets.front() != 0 || index._offsets.back() != index._data.size())
			{
				throw spk::Exception("Invalid replication section boundaries.");
			}
			for (std::size_t i = 0; i < sectionCount; ++i)
			{
				if (index._offsets[i] >= index._offsets[i + 1])
				{
					throw spk::Exception("Overlapping or empty replication section.");
				}
			}
			return index;
		}

		[[nodiscard]] static std::vector<Record> decodeSection(
			spk::ByteStream::Slice section,
			std::size_t expectedCount)
		{
			constexpr std::size_t headerSize =
				sizeof(spk::UUID::Storage) + sizeof(std::uint64_t) + sizeof(std::uint32_t);
			if (expectedCount > section.size() / headerSize)
			{
				throw spk::Exception("Replication section cannot contain the declared component count.");
			}
			std::vector<Record> records;
			records.reserve(expectedCount);
			for (std::size_t i = 0; i < expectedCount; ++i)
			{
				spk::UUID::Storage identifier{};
				std::uint64_t revision = 0;
				std::uint32_t size = 0;
				section >> identifier >> revision >> size;
				if (size > section.remaining())
				{
					throw spk::Exception("Replication component payload exceeds section.");
				}
				const std::size_t begin = section.readOffset();
				section.skip(size);
				records.push_back({spk::UUID(identifier), revision, section.slice(begin, section.readOffset())});
			}
			if (section.remaining() != 0)
			{
				throw spk::Exception("Replication section contains trailing bytes.");
			}
			return records;
		}
	};
}
