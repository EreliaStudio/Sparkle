#include "gpu_resource_collection.hpp"

#include <mutex>
#include <stdexcept>
#include <utility>

#include "render_context.hpp"

namespace spk
{
	struct GPUResourceCollection::ReclamationQueue
	{
		std::mutex mutex;
		std::vector<GPUResource::Identifier> identifiers;

		void push(GPUResource::Identifier identifier)
		{
			const std::scoped_lock lock(mutex);
			identifiers.push_back(identifier);
		}

		void drain(std::vector<GPUResource::Identifier> &output)
		{
			output.clear();
			const std::scoped_lock lock(mutex);
			identifiers.swap(output);
		}

		void clear()
		{
			const std::scoped_lock lock(mutex);
			identifiers.clear();
		}
	};

	GPUResourceCollection::GPUResourceCollection() :
		_reclamationQueue(std::make_shared<ReclamationQueue>())
	{
	}

	GPUResourceCollection::~GPUResourceCollection() = default;

	constexpr std::size_t GPUResourceCollection::_kindIndex(GPUResource::Kind kind) noexcept
	{
		return static_cast<std::size_t>(kind);
	}

	[[nodiscard]] static constexpr bool _isRecyclable(GPUResource::Kind kind) noexcept
	{
		switch (kind)
		{
		case GPUResource::Kind::Buffer:
		case GPUResource::Kind::Texture:
		case GPUResource::Kind::Framebuffer:
			return true;

		case GPUResource::Kind::VertexArray:
		case GPUResource::Kind::Program:
			return false;
		}

		return false;
	}

	std::unique_ptr<GPUResourceCollection::Instance> GPUResourceCollection::_acquire(GPUResource::Kind kind)
	{
		if (!_isRecyclable(kind))
			return nullptr;

		auto &pool = _pools[_kindIndex(kind)];
		if (pool.empty())
			return nullptr;

		auto result = std::move(pool.back());
		pool.pop_back();
		return result;
	}

	void GPUResourceCollection::_subscribe(const GPUResource &resource)
	{
		const std::weak_ptr<ReclamationQueue> queue = _reclamationQueue;

		resource._subscribeToRelease([queue](GPUResource::Identifier identifier) {
			if (auto lockedQueue = queue.lock())
				lockedQueue->push(identifier);
		});
	}

	GPUResourceCollection::Entry &GPUResourceCollection::_entry(const GPUResource &resource, RenderContext &context)
	{
		auto [it, inserted] = _entries.try_emplace(resource._identifier);
		auto &entry = it->second;

		if (inserted)
		{
			entry.kind = resource._kind();
			_subscribe(resource);
		}

		if (entry.instance == nullptr)
		{
			entry.instance = _acquire(entry.kind);
			if (entry.instance == nullptr)
				entry.instance = resource._create(context);
			if (entry.instance == nullptr)
				throw std::logic_error("GPU resource creation returned a null instance");
			entry.generation = 0;
		}

		return entry;
	}

	void GPUResourceCollection::_recycle(GPUResource::Identifier identifier)
	{
		auto it = _entries.find(identifier);
		if (it == _entries.end())
			return;

		auto &entry = it->second;

		if (entry.instance != nullptr && _isRecyclable(entry.kind))
			_pools[_kindIndex(entry.kind)].push_back(std::move(entry.instance));

		_entries.erase(it);
	}

	void GPUResourceCollection::reclaimReleased()
	{
		_reclamationQueue->drain(_releasedIdentifiers);

		for (const auto identifier : _releasedIdentifiers)
			_recycle(identifier);

		_releasedIdentifiers.clear();
	}

	void GPUResourceCollection::clear()
	{
		_entries.clear();

		for (auto &pool : _pools)
			pool.clear();

		_reclamationQueue->clear();
		_releasedIdentifiers.clear();
	}
}