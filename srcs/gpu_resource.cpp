#include "gpu_resource.hpp"

#include <atomic>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

#include "gpu_resource_collection.hpp"
#include "render_context.hpp"

namespace spk
{
	class GPUResource::LifeTime
	{
	private:
		Identifier _identifier;
		std::mutex _mutex;
		std::vector<std::function<void(Identifier)>> _callbacks;

	public:
		explicit LifeTime(Identifier identifier) :
			_identifier(identifier)
		{
		}

		~LifeTime() noexcept
		{
			std::vector<std::function<void(Identifier)>> callbacks;
			{
				const std::scoped_lock lock(_mutex);
				callbacks = std::move(_callbacks);
			}

			for (auto &callback : callbacks)
			{
				try
				{
					callback(_identifier);
				}
				catch (...)
				{
				}
			}
		}

		void subscribe(std::function<void(Identifier)> callback)
		{
			const std::scoped_lock lock(_mutex);
			_callbacks.push_back(std::move(callback));
		}
	};

	GPUResource::GPUResource() :
		_identifier(_generateIdentifier()),
		_lifeTime(std::make_shared<LifeTime>(_identifier))
	{
	}

	GPUResource::GPUResource(GPUResource &&other) noexcept :
		_identifier(std::exchange(other._identifier, _generateIdentifier())),
		_generation(std::exchange(other._generation, 1)),
		_lifeTime(std::move(other._lifeTime))
	{
	}

	GPUResource::Identifier GPUResource::_generateIdentifier() noexcept
	{
		static std::atomic<Identifier> nextIdentifier = 1;
		return nextIdentifier.fetch_add(1, std::memory_order_relaxed);
	}

	void GPUResource::_subscribeToRelease(std::function<void(Identifier)> callback) const
	{
		if (_lifeTime == nullptr)
			throw std::logic_error("Cannot subscribe to a moved-from GPU resource");
		_lifeTime->subscribe(std::move(callback));
	}

	GPUResource::RecyclingScore GPUResource::_recyclingScore(const Instance &) const noexcept
	{
		return 1;
	}

	void GPUResource::validate() noexcept
	{
		++_generation;
		if (_generation == 0)
			_generation = 1;
	}

	void GPUResource::activate(RenderContext &context) const
	{
		if (context.targetSurface == nullptr)
			throw std::invalid_argument("Cannot activate a GPU resource without a target surface");
		if (_lifeTime == nullptr)
			throw std::logic_error("Cannot activate a moved-from GPU resource");

		auto &entry = context.targetSurface->_gpuResources()._entry(*this, context);
		if (entry.generation != _generation)
		{
			_synchronize(*entry.instance, context);
			entry.generation = _generation;
		}

		_bind(*entry.instance, context);
	}

	GPUResource::Identifier GPUResource::identifier() const noexcept
	{
		return _identifier;
	}

	GPUResource::Generation GPUResource::generation() const noexcept
	{
		return _generation;
	}
}
