#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include "gpu_resource.hpp"

namespace spk
{
	struct RenderContext;

	class GPUResourceCollection
	{
		friend class GPUResource;

	private:
		using Instance = GPUResource::Instance;
		using InstancePool = std::vector<std::unique_ptr<Instance>>;

		struct Entry
		{
			std::unique_ptr<Instance> instance;
			GPUResource::Generation generation = 0;
			GPUResource::Kind kind = GPUResource::Kind::Buffer;
		};

		struct ReclamationQueue;

		std::unordered_map<GPUResource::Identifier, Entry> _entries;
		std::array<InstancePool, GPUResource::NbKind> _pools;
		std::shared_ptr<ReclamationQueue> _reclamationQueue;
		std::vector<GPUResource::Identifier> _releasedIdentifiers;

		[[nodiscard]] static constexpr std::size_t _kindIndex(GPUResource::Kind kind) noexcept;
		[[nodiscard]] std::unique_ptr<Instance> _acquire(const GPUResource &resource);
		[[nodiscard]] Entry &_entry(const GPUResource &resource, RenderContext &context);

		void _subscribe(const GPUResource &resource);
		void _recycle(GPUResource::Identifier identifier);

	public:
		GPUResourceCollection();
		GPUResourceCollection(const GPUResourceCollection &) = delete;
		GPUResourceCollection(GPUResourceCollection &&) = delete;
		~GPUResourceCollection();

		GPUResourceCollection &operator=(const GPUResourceCollection &) = delete;
		GPUResourceCollection &operator=(GPUResourceCollection &&) = delete;

		void reclaimReleased();
		void clear();
	};
}
