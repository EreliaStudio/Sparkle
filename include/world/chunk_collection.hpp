#pragma once

#include <concepts>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "design_pattern/contract_provider.hpp"
#include "engine/engine_headless.hpp"
#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "exception.hpp"
#include "world/chunk.hpp"
#include "world/chunk_coordinate.hpp"
#include "world/chunk_filter.hpp"

namespace spk::World
{
	// A headless, value-owning collection. No chunk, entity or component ownership
	// escapes through the collection's public API.
	template <typename TChunk>
	class ChunkCollection
	{
		static_assert(
			std::derived_from<TChunk, Chunk<TChunk::Size.x, TChunk::Size.y, TChunk::Size.z>>,
			"TChunk must derive from spk::World::Chunk<SX, SY, SZ>");

	public:
		using EditionProvider = ContractProvider<ChunkCoordinate>;
		using EditionCallback = EditionProvider::callback_type;
		using EditionContract = EditionProvider::Contract;

		class ChunkEntry final
		{
			friend class ChunkCollection;

		private:
			// Member order ensures the Entity3D and its components die before
			// the referenced chunk, even if the collection is simply destroyed.
			TChunk _chunk;
			std::unique_ptr<Entity3D> _entity;
			typename TChunk::EditionContract _editionContract;

		public:
			explicit ChunkEntry(TChunk chunk, Entity3D &root, const ChunkCoordinate &coordinate) :
				_chunk(std::move(chunk)),
				_entity(std::make_unique<Entity3D>("Chunk", &root))
			{
				const ChunkWorldCoordinate origin = TChunk::chunkCoordinateToWorld(coordinate);
				const float x = static_cast<float>(origin.x);
				const float y = static_cast<float>(origin.y);
				const float z = static_cast<float>(origin.z);
				_entity->transform().place({x, y, z});
				_entity->addComponent<ChunkFilter<TChunk>>(_chunk);
			}

		public:
			ChunkEntry(const ChunkEntry &) = delete;
			ChunkEntry &operator=(const ChunkEntry &) = delete;
			ChunkEntry(ChunkEntry &&) = delete;
			ChunkEntry &operator=(ChunkEntry &&) = delete;

			~ChunkEntry()
			{
				_editionContract.resign();
				if (_entity != nullptr)
				{
					if (ChunkFilter<TChunk> *filter = _entity->template getComponent<ChunkFilter<TChunk>>(); filter != nullptr)
					{
						filter->_unbind();
					}
					_entity.reset();
				}
			}

			[[nodiscard]] TChunk &chunk() noexcept
			{
				return _chunk;
			}

			[[nodiscard]] const TChunk &chunk() const noexcept
			{
				return _chunk;
			}

			[[nodiscard]] Entity3D &entity() noexcept
			{
				return *_entity;
			}

			[[nodiscard]] const Entity3D &entity() const noexcept
			{
				return *_entity;
			}
		};

		using Container = std::map<ChunkCoordinate, ChunkEntry>;
		using iterator = typename Container::iterator;
		using const_iterator = typename Container::const_iterator;
		using reverse_iterator = typename Container::reverse_iterator;
		using const_reverse_iterator = typename Container::const_reverse_iterator;

	private:
		// Keep the root alive until every entry has been torn down.
		Entity3D _root;
		Container _chunks;
		EditionProvider _editionProvider;

	public:
		explicit ChunkCollection(const std::string &name = "Chunk collection") :
			_root(name)
		{
		}

		ChunkCollection(const ChunkCollection &) = delete;
		ChunkCollection &operator=(const ChunkCollection &) = delete;
		ChunkCollection(ChunkCollection &&) = delete;
		ChunkCollection &operator=(ChunkCollection &&) = delete;

		~ChunkCollection() noexcept
		{
			try
			{
				clear();
				detach();
			} catch (...)
			{
				// Entry and root members remain RAII-owned during teardown.
			}
		}

		[[nodiscard]] Entity3D &root() noexcept
		{
			return _root;
		}

		[[nodiscard]] const Entity3D &root() const noexcept
		{
			return _root;
		}

		[[nodiscard]] EngineHeadless *engine() const noexcept
		{
			return _root.context();
		}

		void attach(EngineHeadless &engine)
		{
			engine.addEntity(&_root);
		}

		void detach()
		{
			if (EngineHeadless *current = _root.context(); current != nullptr)
			{
				current->removeEntity(&_root);
			}
		}

		// An immediate signal for insertion, replacement and explicit edits.
		[[nodiscard]] EditionContract subscribeToChunkEdition(EditionCallback callback)
		{
			return _editionProvider.subscribe(std::move(callback));
		}

		void addChunk(const ChunkCoordinate &coordinate, TChunk chunk)
		{
			if (auto it = _chunks.find(coordinate); it != _chunks.end())
			{
				it->second._chunk = std::move(chunk);
				_editionProvider.trigger(coordinate);
				return;
			}

			auto [it, inserted] = _chunks.try_emplace(coordinate, std::move(chunk), _root, coordinate);
			(void)inserted;
			try
			{
				it->second._editionContract = it->second._chunk.subscribeToChunkEdition([this, coordinate]() {
					_editionProvider.trigger(coordinate);
				});
			} catch (...)
			{
				_chunks.erase(it);
				throw;
			}
			_editionProvider.trigger(coordinate);
		}

		[[nodiscard]] bool removeChunk(const ChunkCoordinate &coordinate)
		{
			auto entry = _chunks.extract(coordinate);
			return !entry.empty();
		}

		void clear()
		{
			while (!_chunks.empty())
			{
				(void)removeChunk(_chunks.begin()->first);
			}
		}

		[[nodiscard]] bool contains(const ChunkCoordinate &coordinate) const noexcept
		{
			return _chunks.contains(coordinate);
		}

		[[nodiscard]] std::size_t size() const noexcept
		{
			return _chunks.size();
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return _chunks.empty();
		}

		// Output is a non-owning pointer to the stored chunk, or nullptr on failure.
		[[nodiscard]] bool tryGet(const ChunkCoordinate &coordinate, TChunk *&chunk) noexcept
		{
			if (auto it = _chunks.find(coordinate); it != _chunks.end())
			{
				chunk = &it->second._chunk;
				return true;
			}
			chunk = nullptr;
			return false;
		}

		[[nodiscard]] bool tryGet(const ChunkCoordinate &coordinate, const TChunk *&chunk) const noexcept
		{
			if (auto it = _chunks.find(coordinate); it != _chunks.end())
			{
				chunk = &it->second._chunk;
				return true;
			}
			chunk = nullptr;
			return false;
		}

		[[nodiscard]] TChunk &at(const ChunkCoordinate &coordinate)
		{
			TChunk *chunk = nullptr;
			if (tryGet(coordinate, chunk))
			{
				return *chunk;
			}
			throw spk::Exception("Chunk coordinate not loaded");
		}

		[[nodiscard]] const TChunk &at(const ChunkCoordinate &coordinate) const
		{
			const TChunk *chunk = nullptr;
			if (tryGet(coordinate, chunk))
			{
				return *chunk;
			}
			throw spk::Exception("Chunk coordinate not loaded");
		}

		[[nodiscard]] TChunk &operator[](const ChunkCoordinate &coordinate)
		{
			return at(coordinate);
		}

		[[nodiscard]] const TChunk &operator[](const ChunkCoordinate &coordinate) const
		{
			return at(coordinate);
		}

		[[nodiscard]] iterator begin() noexcept
		{
			return _chunks.begin();
		}
		[[nodiscard]] const_iterator begin() const noexcept
		{
			return _chunks.begin();
		}
		[[nodiscard]] const_iterator cbegin() const noexcept
		{
			return _chunks.cbegin();
		}
		[[nodiscard]] iterator end() noexcept
		{
			return _chunks.end();
		}
		[[nodiscard]] const_iterator end() const noexcept
		{
			return _chunks.end();
		}
		[[nodiscard]] const_iterator cend() const noexcept
		{
			return _chunks.cend();
		}
		[[nodiscard]] reverse_iterator rbegin() noexcept
		{
			return _chunks.rbegin();
		}
		[[nodiscard]] const_reverse_iterator rbegin() const noexcept
		{
			return _chunks.rbegin();
		}
		[[nodiscard]] const_reverse_iterator crbegin() const noexcept
		{
			return _chunks.crbegin();
		}
		[[nodiscard]] reverse_iterator rend() noexcept
		{
			return _chunks.rend();
		}
		[[nodiscard]] const_reverse_iterator rend() const noexcept
		{
			return _chunks.rend();
		}
		[[nodiscard]] const_reverse_iterator crend() const noexcept
		{
			return _chunks.crend();
		}
	};
}
