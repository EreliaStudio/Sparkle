#pragma once

#include <atomic>
#include <cstdint>
#include <utility>

#include "container/buffer.hpp"
#include "design_pattern/contract_provider.hpp"
#include "geometry/vertex_layout.hpp"

namespace spk
{
	// A published CPU mesh. Its address and buffer addresses remain stable.
	class MeshData
	{
	public:
		using DestructionContract = ContractProvider<>::Contract;

	private:
		inline static std::atomic_uint64_t _nextIdentifier{1};

		const std::uint64_t _identifier = _nextIdentifier.fetch_add(1, std::memory_order_relaxed);
		Buffer::Immutable _vertices;
		Buffer::Immutable _indices;
		VertexLayout _layout;
		mutable ContractProvider<> _destructionProvider;

	public:
		MeshData(Buffer::Immutable vertices, Buffer::Immutable indices, VertexLayout layout) :
			_vertices(std::move(vertices)),
			_indices(std::move(indices)),
			_layout(std::move(layout))
		{
		}

		~MeshData()
		{
			try
			{
				if (!_destructionProvider.empty())
				{
					_destructionProvider.trigger();
				}
			} catch (...)
			{
				// Destruction must not propagate subscriber exceptions.
			}
		}

		[[nodiscard]] std::uint64_t identifier() const noexcept
		{
			return _identifier;
		}

		[[nodiscard]] const Buffer::Immutable &vertexBuffer() const noexcept
		{
			return _vertices;
		}

		[[nodiscard]] const Buffer::Immutable &indexBuffer() const noexcept
		{
			return _indices;
		}

		[[nodiscard]] const VertexLayout &vertexLayout() const noexcept
		{
			return _layout;
		}

		[[nodiscard]] DestructionContract subscribeToDestruction(ContractProvider<>::callback_type callback) const
		{
			return _destructionProvider.subscribe(std::move(callback));
		}
	};
}
