#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace spk
{
	template <typename... TArguments>
	class ContractProvider final
	{
		static_assert((!std::is_rvalue_reference_v<TArguments> && ...), "Use a value or lvalue-reference callback argument instead of an rvalue reference");

	public:
		using callback_type = std::function<void(TArguments...)>;

	private:
		using stored_arguments_type = std::tuple<TArguments...>;

		struct State;

		struct Registration final
		{
			callback_type callback;
			std::weak_ptr<State> state;
			bool active = true;
		};

	public:
		class Contract final
		{
		public:
			Contract() = default;
			~Contract()
			{
				resign();
			}

			Contract(const Contract &) = delete;
			Contract &operator=(const Contract &) = delete;

			Contract(Contract &&) noexcept = default;
			Contract &operator=(Contract &&other) noexcept
			{
				if (this != &other)
				{
					resign();
					_registration = std::move(other._registration);
				}
				return *this;
			}

			void resign() noexcept
			{
				const std::shared_ptr<Registration> registration = _registration.lock();
				_registration.reset();
				if (registration == nullptr || !registration->active)
				{
					return;
				}
				if (const std::shared_ptr<State> state = registration->state.lock(); state != nullptr)
				{
					state->requestRemoval(registration);
				}
			}

			[[nodiscard]] bool isValid() const noexcept
			{
				const std::shared_ptr<Registration> registration = _registration.lock();
				return registration != nullptr && registration->active && !registration->state.expired();
			}

			[[nodiscard]] explicit operator bool() const noexcept
			{
				return isValid();
			}

		private:
			friend class ContractProvider;

			explicit Contract(const std::shared_ptr<Registration> &registration) noexcept :
				_registration(registration)
			{
			}

			std::weak_ptr<Registration> _registration;
		};

		ContractProvider() :
			_state(std::make_shared<State>())
		{
		}

		~ContractProvider()
		{
			_state->requestInvalidation();
		}

		ContractProvider(const ContractProvider &) = delete;
		ContractProvider(ContractProvider &&) = delete;
		ContractProvider &operator=(const ContractProvider &) = delete;
		ContractProvider &operator=(ContractProvider &&) = delete;

		[[nodiscard]] Contract subscribe(callback_type callback)
		{
			auto registration = std::make_shared<Registration>(Registration{std::move(callback), _state, true});
			_state->requestAddition(registration);
			return Contract(registration);
		}

		void trigger(TArguments... arguments)
		{
			_state->trigger(stored_arguments_type(std::forward<TArguments>(arguments)...));
		}

		void invalidate() noexcept
		{
			_state->requestInvalidation();
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return _state->empty();
		}

	private:
		enum class MutationKind : std::uint8_t
		{
			Addition,
			Removal,
			Invalidation
		};

		struct Mutation final
		{
			MutationKind kind;
			std::shared_ptr<Registration> registration;
		};

		struct State final : std::enable_shared_from_this<State>
		{
			void requestAddition(const std::shared_ptr<Registration> &registration)
			{
				if (dispatching)
				{
					mutations.push_back(Mutation{MutationKind::Addition, registration});
					return;
				}
				registrations.push_back(registration);
			}

			void requestRemoval(const std::shared_ptr<Registration> &registration) noexcept
			{
				if (dispatching)
				{
					registration->active = false;
					try
					{
						mutations.push_back(Mutation{MutationKind::Removal, registration});
					} catch (...)
					{
						std::terminate();
					}
					return;
				}
				remove(registration);
			}

			void requestInvalidation() noexcept
			{
				if (dispatching)
				{
					for (const std::shared_ptr<Registration> &registration : registrations)
					{
						registration->active = false;
					}
					try
					{
						mutations.push_back(Mutation{MutationKind::Invalidation, nullptr});
					} catch (...)
					{
						std::terminate();
					}
					return;
				}
				invalidateImmediately();
			}

			void trigger(stored_arguments_type &&arguments)
			{
				const std::shared_ptr<State> dispatchLifetime = this->shared_from_this();
				if (dispatching)
				{
					pendingArguments.emplace(std::move(arguments));
					return;
				}
				dispatching = true;
				try
				{
					dispatchLoop(std::move(arguments));
					dispatching = false;
				} catch (...)
				{
					recoverFromDispatchFailure();
					throw;
				}
			}

			[[nodiscard]] bool empty() const noexcept
			{
				for (const std::shared_ptr<Registration> &registration : registrations)
				{
					if (registration->active)
					{
						return false;
					}
				}
				for (const Mutation &mutation : mutations)
				{
					if (mutation.kind == MutationKind::Addition && mutation.registration->active)
					{
						return false;
					}
				}
				return true;
			}

		private:
			void dispatchLoop(stored_arguments_type &&arguments)
			{
				std::optional<stored_arguments_type> currentArguments;
				currentArguments.emplace(std::move(arguments));
				while (true)
				{
					dispatch(*currentArguments);
					applyMutations();
					if (!pendingArguments.has_value())
					{
						return;
					}
					currentArguments.emplace(std::move(*pendingArguments));
					pendingArguments.reset();
				}
			}

			void recoverFromDispatchFailure() noexcept
			{
				pendingArguments.reset();
				try
				{
					applyMutations();
				} catch (...)
				{
					std::terminate();
				}
				dispatching = false;
			}

			void dispatch(stored_arguments_type &arguments)
			{
				const std::vector<std::shared_ptr<Registration>> selected = registrations;
				for (const std::shared_ptr<Registration> &registration : selected)
				{
					if (registration->active)
					{
						std::apply(registration->callback, arguments);
					}
				}
			}

			void applyMutations()
			{
				std::vector<Mutation> requested = std::move(mutations);
				mutations.clear();
				for (Mutation &mutation : requested)
				{
					switch (mutation.kind)
					{
					case MutationKind::Addition:
						registrations.push_back(std::move(mutation.registration));
						break;
					case MutationKind::Removal:
						remove(mutation.registration);
						break;
					case MutationKind::Invalidation:
						invalidateImmediately();
						break;
					}
				}
			}

			void remove(const std::shared_ptr<Registration> &removed) noexcept
			{
				removed->active = false;
				removed->callback = nullptr;
				std::erase(registrations, removed);
			}

			void invalidateImmediately() noexcept
			{
				for (const std::shared_ptr<Registration> &registration : registrations)
				{
					registration->active = false;
					registration->callback = nullptr;
				}
				registrations.clear();
			}

			std::vector<std::shared_ptr<Registration>> registrations;
			std::vector<Mutation> mutations;
			std::optional<stored_arguments_type> pendingArguments;
			bool dispatching = false;
		};

		std::shared_ptr<State> _state;
	};
}
