#include "graphics/opengl/gpu_resource.hpp"

#include <atomic>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

#include "core/context/render_context.hpp"
#include "exception.hpp"
#include "graphics/opengl/gpu_resource_collection.hpp"

namespace spk
{
	class GPUResource::State::LifeTime
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
				} catch (...)
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

	GPUResource::State::State() :
		_identifier(_generateIdentifier()),
		_lifeTime(std::make_shared<LifeTime>(_identifier))
	{
	}

	GPUResource::GPUResource(std::shared_ptr<State> state) :
		_state(std::move(state))
	{
		if (_state == nullptr)
		{
			throw std::invalid_argument("GPU resource state cannot be null");
		}
	}

	GPUResource::Identifier GPUResource::_generateIdentifier() noexcept
	{
		static std::atomic<Identifier> nextIdentifier = 1;
		return nextIdentifier.fetch_add(1, std::memory_order_relaxed);
	}

	void GPUResource::State::_subscribeToRelease(std::function<void(Identifier)> callback) const
	{
		if (_lifeTime == nullptr)
		{
			throw std::logic_error("Cannot subscribe to a moved-from GPU resource");
		}
		_lifeTime->subscribe(std::move(callback));
	}

	GPUResource::RecyclingScore GPUResource::State::_recyclingScore(const Instance &) const noexcept
	{
		return 1;
	}

	void GPUResource::validate()
	{
		if (_state == nullptr)
		{
			throw std::logic_error("Cannot validate a moved-from GPU resource");
		}
		_state->invalidate();
	}

	void GPUResource::activate(RenderContext &context) const
	{
		if (_state == nullptr)
		{
			throw std::logic_error("Cannot activate a moved-from GPU resource");
		}
		_activate(*_state, context);
	}

	void GPUResource::_activate(const State &state, RenderContext &context)
	{
		if (context.targetSurface == nullptr)
		{
			throw std::invalid_argument("Cannot activate a GPU resource without a target surface");
		}
		try
		{
			auto &entry = context.targetSurface->_gpuResources()._entry(state, context);
			if (entry.generation != state.version())
			{
				state._synchronize(*entry.instance, context);
				entry.generation = state.version();
			}

			state._bind(*entry.instance, context);
		} catch (...)
		{
			throw spk::Exception(
				"Exception while activating GPU resource [" + std::to_string(state._identifier) + "]",
				std::current_exception());
		}
	}

	GPUResource::Identifier GPUResource::identifier() const noexcept
	{
		return _state ? _state->_identifier : 0;
	}

	GPUResource::Generation GPUResource::generation() const noexcept
	{
		return _state ? _state->version() : 0;
	}
}
