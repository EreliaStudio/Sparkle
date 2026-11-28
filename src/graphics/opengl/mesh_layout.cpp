#include "graphics/opengl/mesh_layout.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <utility>

namespace spk
{
	namespace
	{
		class MeshLayoutCollection
		{
		private:
			struct Entry
			{
				std::unique_ptr<LayoutBuffer> layout;
				MeshData::DestructionContract contract;
			};

			std::mutex _mutex;
			std::map<std::uint64_t, Entry> _entries;

		public:
			void remove(std::uint64_t identifier)
			{
				const std::scoped_lock lock(_mutex);
				_entries.erase(identifier);
			}

			const LayoutBuffer &obtain(const MeshData &mesh, const std::weak_ptr<MeshLayoutCollection> &owner)
			{
				const std::scoped_lock lock(_mutex);
				const auto identifier = mesh.identifier();
				const auto found = _entries.find(identifier);
				if (found != _entries.end())
				{
					return *found->second.layout;
				}

				auto layout = std::make_unique<LayoutBuffer>();
				layout->vertexBuffer().setLayout(mesh.vertexLayout());
				layout->indexBuffer().setType(IndexBuffer::Type::UnsignedInt);
				layout->vertexBuffer().bind(&mesh.vertexBuffer());
				layout->indexBuffer().bind(&mesh.indexBuffer());
				layout->validate();
				auto contract = mesh.subscribeToDestruction([owner, identifier] {
					if (auto collection = owner.lock())
					{
						collection->remove(identifier);
					}
				});
				return *_entries.emplace(identifier, Entry{std::move(layout), std::move(contract)}).first->second.layout;
			}
		};
	}

	const LayoutBuffer &MeshLayout::obtain(const MeshData &mesh)
	{
		static const auto collection = std::make_shared<MeshLayoutCollection>();
		return collection->obtain(mesh, collection);
	}
}
