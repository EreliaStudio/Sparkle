#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "structure/system/spk_exception.hpp"

namespace spk
{
	template <typename... TParameterTypes>
	class TContractProvider
	{
	private:
		struct State
		{
			TContractProvider *provider = nullptr;
		};

	public:
		class Contract
		{
			friend class TContractProvider<TParameterTypes...>;

		public:
			using Job = std::function<void(TParameterTypes...)>;

		private:
			std::shared_ptr<Job> _job = nullptr;
			std::shared_ptr<State> _state;

			Contract(TContractProvider *p_originator, const std::shared_ptr<Job> &p_job) :
				_job(p_job),
				_state(p_originator ? p_originator->_state : nullptr)
			{
			}

		public:
			Contract() = default;

			Contract(const Contract &) = delete;
			Contract &operator=(const Contract &) = delete;

			Contract(Contract &&p_other) noexcept :
				_job(std::move(p_other._job)),
				_state(std::move(p_other._state))
			{
				p_other._job.reset();
				p_other._state.reset();
			}

			Contract &operator=(Contract &&p_other) noexcept
			{
				if (this != &p_other)
				{
					if (isValid())
					{
						resign();
					}
					_job = std::move(p_other._job);
					_state = std::move(p_other._state);
					p_other._job.reset();
					p_other._state.reset();
				}
				return (*this);
			}

			~Contract()
			{
				if (isValid())
				{
					resign();
				}
			}

			bool isValid() const
			{
				return (_job && *_job);
			}

			void resign()
			{
				if (!isValid())
				{
					GENERATE_ERROR("Can't resign an already resigned contract.");
				}
				*_job = nullptr;

				if (_state && _state->provider)
				{
					_state->provider->unsubscribe(*this);
				}

				_job.reset();
				_state.reset();
			}

			void relinquish()
			{
				if (!isValid())
				{
					GENERATE_ERROR("Can't relinquish an already resigned contract.");
				}
				if (_state && _state->provider)
				{
					_state->provider->relinquish(std::move(*this));
				}
				_job.reset();
				_state.reset();
			}

			void trigger(TParameterTypes... p_args) const
			{
				if (isValid())
				{
					(*_job)(std::forward<TParameterTypes>(p_args)...);
				}
			}
		};

		class Blocker
		{
			friend class TContractProvider<TParameterTypes...>;

		public:
			enum class Mode
			{
				Ignore = 0,
				Delay = 1,
			};

			Blocker() = default;

			Blocker(const Blocker &) = delete;
			Blocker &operator=(const Blocker &) = delete;

			Blocker(Blocker &&p_other) noexcept :
				_state(std::move(p_other._state)),
				_mode(p_other._mode)
			{
			}

			Blocker &operator=(Blocker &&p_other) noexcept
			{
				if (this != &p_other)
				{
					release();
					_state = std::move(p_other._state);
					_mode = p_other._mode;
				}
				return *this;
			}

			~Blocker()
			{
				release();
			}

			bool active() const noexcept
			{
				return (_state && _state->provider != nullptr);
			}

			Mode mode() const noexcept
			{
				return _mode;
			}

			void release() noexcept
			{
				auto p_state = std::move(_state);
				if (!p_state)
				{
					return;
				}

				if (auto *prov = p_state->provider)
				{
					prov->_releaseBlock(_mode);
				}
			}

		private:
			std::shared_ptr<State> _state;
			Mode _mode = Mode::Ignore;

			Blocker(std::shared_ptr<State> p_state, Mode p_mode) :
				_state(std::move(p_state)),
				_mode(p_mode)
			{
			}
		};

		using Job = typename Contract::Job;
		using VoidJob = std::function<void()>;

	private:
		std::vector<std::shared_ptr<Job>> _subscribedJobs;
		std::vector<Contract> _relinquishedContracts;

		std::shared_ptr<State> _state = std::make_shared<State>(State{this});

		mutable std::recursive_mutex _mutex;

		mutable std::size_t _ignoreCount = 0;
		mutable std::size_t _delayCount = 0;

		using ArgTuple = std::tuple<std::decay_t<TParameterTypes>...>;
		mutable std::optional<ArgTuple> _pendingDelayed;

	public:
		TContractProvider() = default;

		~TContractProvider()
		{
			invalidateContracts();

			if (_state)
			{
				_state->provider = nullptr;
			}
		}

		TContractProvider(const TContractProvider &) = delete;
		TContractProvider &operator=(const TContractProvider &) = delete;

		TContractProvider(TContractProvider &&p_other) noexcept
		{
			std::lock_guard<std::recursive_mutex> lock(p_other._mutex);

			_subscribedJobs = std::move(p_other._subscribedJobs);
			_relinquishedContracts = std::move(p_other._relinquishedContracts);
			_state = std::move(p_other._state);
			_ignoreCount = p_other._ignoreCount;
			_delayCount = p_other._delayCount;
			_pendingDelayed = std::move(p_other._pendingDelayed);

			if (_state)
			{
				_state->provider = this;
			}

			p_other._subscribedJobs.clear();
			p_other._relinquishedContracts.clear();
			p_other._ignoreCount = 0;
			p_other._delayCount = 0;
			p_other._pendingDelayed = std::nullopt;
			p_other._state = std::make_shared<State>(State{&p_other});
		}

		TContractProvider &operator=(TContractProvider &&p_other) noexcept
		{
			if (this != &p_other)
			{
				std::scoped_lock lock(_mutex, p_other._mutex);

				invalidateContracts();
				if (_state)
				{
					_state->provider = nullptr;
				}

				_subscribedJobs = std::move(p_other._subscribedJobs);
				_relinquishedContracts = std::move(p_other._relinquishedContracts);
				_state = std::move(p_other._state);
				_ignoreCount = p_other._ignoreCount;
				_delayCount = p_other._delayCount;
				_pendingDelayed = std::move(p_other._pendingDelayed);

				if (_state)
				{
					_state->provider = this;
				}

				p_other._subscribedJobs.clear();
				p_other._relinquishedContracts.clear();
				p_other._ignoreCount = 0;
				p_other._delayCount = 0;
				p_other._pendingDelayed = std::nullopt;
				p_other._state = std::make_shared<State>(State{&p_other});
			}
			return (*this);
		}

		Blocker block(typename Blocker::Mode mode = Blocker::Mode::Ignore)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			_applyBlock(mode);
			return Blocker(_state, mode);
		}

		void invalidateContracts()
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			for (auto &job : _subscribedJobs)
			{
				if (job)
				{
					*job = nullptr;
				}
			}
			_subscribedJobs.clear();
			_relinquishedContracts.clear();
			_pendingDelayed.reset();
		}

		Contract subscribe(const Job &p_job)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			auto toAdd = std::make_shared<Job>(p_job);
			_subscribedJobs.push_back(toAdd);
			return Contract(this, toAdd);
		}

		template <typename Dummy = void, typename = std::enable_if_t<(sizeof...(TParameterTypes) > 0), Dummy>>
		Contract subscribe(const VoidJob &p_job)
		{
			return subscribe([p_job](TParameterTypes...) { p_job(); });
		}

		void relinquish(Contract &&p_contract)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			_relinquishedContracts.push_back(std::move(p_contract));
		}

		void unsubscribe(const Contract &p_contract)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			auto it = std::remove(_subscribedJobs.begin(), _subscribedJobs.end(), p_contract._job);
			if (it != _subscribedJobs.end())
			{
				if (*it && **it)
				{
					**it = nullptr;
				}
				_subscribedJobs.erase(it, _subscribedJobs.end());
			}
		}

		void trigger(TParameterTypes... p_args) const
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);

			if (_ignoreCount > 0)
			{
				return;
			}

			if (_delayCount > 0)
			{
				_pendingDelayed.emplace(std::forward<TParameterTypes>(p_args)...);
				return;
			}

			for (auto &job : _subscribedJobs)
			{
				if (job && *job)
				{
					(*job)(std::forward<TParameterTypes>(p_args)...);
				}
			}
		}

	private:
		void _applyBlock(typename Blocker::Mode p_mode)
		{
			switch (p_mode)
			{
			case Blocker::Mode::Ignore:
			{
				++_ignoreCount;
				_pendingDelayed.reset();
				break;
			}
			case Blocker::Mode::Delay:
			{
				++_delayCount;
				break;
			}
			}
		}

		void _releaseBlock(typename Blocker::Mode p_mode)
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);

			if (p_mode == Blocker::Mode::Ignore)
			{
				if (_ignoreCount > 0)
				{
					--_ignoreCount;
				}
				return;
			}

			if (_delayCount > 0)
			{
				--_delayCount;
			}

			if (_ignoreCount == 0 && _delayCount == 0 && _pendingDelayed.has_value())
			{
				auto args = std::move(*_pendingDelayed);

				_pendingDelayed.reset();

				std::apply([this](auto &&... xs) {
					this->trigger(std::forward<decltype(xs)>(xs)...);
				}, std::move(args));
			}
		}
	};

	using ContractProvider = TContractProvider<>;
}
