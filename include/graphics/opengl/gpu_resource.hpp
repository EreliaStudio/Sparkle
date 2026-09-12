#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>

#include "design_pattern/trait/versioned_trait.hpp"

namespace spk
{
	struct RenderContext;
	class GPUResourceCollection;

	class GPUResource
	{
		friend class GPUResourceCollection;

	public:
		using Identifier = std::uint64_t;
		using Generation = VersionedTrait::Version;
		using RecyclingScore = std::uint32_t;

		enum class Kind : std::uint8_t
		{
			Buffer,
			VertexArray,
			Texture,
			Framebuffer,
			Program,
			Sampler
		};

		static inline constexpr std::size_t NbKind = 6;

	public:
		class Instance
		{
		public:
			virtual ~Instance() = default;
		};

		class State : public VersionedTrait
		{
			friend class GPUResource;
			friend class GPUResourceCollection;

		private:
			class LifeTime;

			Identifier _identifier;
			std::shared_ptr<LifeTime> _lifeTime;

			void _subscribeToRelease(std::function<void(Identifier)> callback) const;

		protected:
			State();
			State(const State &other);

			[[nodiscard]] virtual Kind _kind() const noexcept = 0;
			[[nodiscard]] virtual std::unique_ptr<State> _clone() const
			{
				throw std::logic_error("This GPU resource state does not support cloning");
			}
			[[nodiscard]] virtual RecyclingScore _recyclingScore(const Instance &instance) const noexcept;
			[[nodiscard]] virtual std::unique_ptr<Instance> _create(RenderContext &context) const = 0;
			virtual void _synchronize(Instance &instance, RenderContext &context) const = 0;
			virtual void _bind(Instance &instance, RenderContext &context) const = 0;

		public:
			State(State &&) = delete;
			~State() override = default;

			State &operator=(const State &) = delete;
			State &operator=(State &&) = delete;
		};

		template <typename TState>
			requires std::derived_from<TState, State>
		class Handle
		{
			friend class GPUResource;

		private:
			std::shared_ptr<const TState> _state;

			explicit Handle(std::shared_ptr<const TState> state) :
				_state(std::move(state))
			{
			}

		public:
			Handle() = default;

			[[nodiscard]] bool valid() const noexcept
			{
				return _state != nullptr;
			}
			explicit operator bool() const noexcept
			{
				return valid();
			}

			[[nodiscard]] const TState &state() const
			{
				if (_state == nullptr)
				{
					throw std::logic_error("Cannot access an empty GPU resource handle");
				}
				return *_state;
			}

			[[nodiscard]] const TState *operator->() const
			{
				return std::addressof(state());
			}
			[[nodiscard]] const TState &operator*() const
			{
				return state();
			}

			void activate(RenderContext &context) const
			{
				GPUResource::_activate(state(), context);
			}
			[[nodiscard]] Identifier identifier() const noexcept
			{
				return _state ? _state->_identifier : 0;
			}
			[[nodiscard]] Generation generation() const noexcept
			{
				return _state ? _state->version() : 0;
			}
		};

	private:
		std::shared_ptr<State> _state;

		[[nodiscard]] static Identifier _generateIdentifier() noexcept;
		static void _activate(const State &state, RenderContext &context);

		protected:
			explicit GPUResource(std::shared_ptr<State> state);
			[[nodiscard]] std::shared_ptr<State> _cloneState() const;
			void _setState(std::shared_ptr<State> state);

		template <typename TState>
			requires std::derived_from<TState, State>
		[[nodiscard]] Handle<TState> createHandle() const
		{
			return Handle<TState>(std::static_pointer_cast<const TState>(_state));
		}

		template <typename TState>
			requires std::derived_from<TState, State>
		[[nodiscard]] TState &state() noexcept
		{
			return static_cast<TState &>(*_state);
		}

		template <typename TState>
			requires std::derived_from<TState, State>
		[[nodiscard]] const TState &state() const noexcept
		{
			return static_cast<const TState &>(*_state);
		}

	public:
		GPUResource() = default;
		GPUResource(const GPUResource &other);
		GPUResource(GPUResource &&other) noexcept = default;
		virtual ~GPUResource() = default;

		[[nodiscard]] virtual std::unique_ptr<GPUResource> clone() const;

		GPUResource &operator=(const GPUResource &other);
		GPUResource &operator=(GPUResource &&) = delete;

		void activate(RenderContext &context) const;

		void validate();

		[[nodiscard]] Identifier identifier() const noexcept;
		[[nodiscard]] Generation generation() const noexcept;
	};
}
