#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace spk
{
	template <typename... TParameterTypes>
	class TContractProvider
	{
	public:
		class Contract
		{
			friend class TContractProvider<TParameterTypes...>;

		public:
			using Job = std::function<void(TParameterTypes...)>;

		private:
			std::shared_ptr<Job> _job = nullptr;
			TContractProvider *_originator = nullptr;

			Contract(TContractProvider *p_originator, const std::shared_ptr<Job> &p_job) :
				_originator(p_originator),
				_job(p_job)
			{
			}

		public:
			Contract() = default;

			Contract(const Contract &) = delete;
			Contract &operator=(const Contract &) = delete;

			Contract(Contract &&p_other) :
				_job(p_other._job),
				_originator(p_other._originator)
			{
				p_other._job = nullptr;
				p_other._originator = nullptr;
			}
			Contract &operator=(Contract &&p_other)
			{
				if (this != &p_other)
				{
					_job = p_other._job;
					_originator = p_other._originator;

					p_other._job = nullptr;
					p_other._originator = nullptr;
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
				return _job && *_job;
			}

			void resign()
			{
				if (!isValid())
				{
					throw std::runtime_error("Can't resign an already resigned contract.");
				}
				*_job = nullptr;
				if (_originator)
				{
					_originator->unsubscribe(*this);
				}
				_job = nullptr;
				_originator = nullptr;
			}

			void relinquish()
			{
				if (!isValid())
				{
					throw std::runtime_error("Can't relinquish an already resigned contract.");
				}
				_originator->relinquish(std::move(*this));
				_job = nullptr;
				_originator = nullptr;
			}

			void trigger(TParameterTypes... args) const
			{
				if (isValid())
				{
					(*_job)(std::forward<TParameterTypes>(args)...);
				}
			}
		};

		using Job = typename Contract::Job;
		using VoidJob = std::function<void()>;

	private:
		std::vector<std::shared_ptr<Job>> _subscribedJobs;
		std::vector<Contract> _relinquishedContracts;
		mutable std::recursive_mutex _mutex;

	public:
		TContractProvider() = default;
		~TContractProvider()
		{
			invalidateContracts();
		}

		TContractProvider(const TContractProvider &) = delete;
		TContractProvider &operator=(const TContractProvider &) = delete;

		TContractProvider(TContractProvider &&p_other) :
			_subscribedJobs(std::move(p_other._subscribedJobs)),
			_relinquishedContracts(std::move(p_other._relinquishedContracts))
		{
		}
		TContractProvider &operator=(TContractProvider &&p_other)
		{
			if (this != &p_other)
			{
				_relinquishedContracts = std::move(p_other._relinquishedContracts);
				_subscribedJobs = std::move(p_other._subscribedJobs);
			}
			return (*this);
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
			// Capture p_job by value to avoid dangling references.
			return subscribe([p_job](TParameterTypes... /*args*/) { p_job(); });
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

		void trigger(TParameterTypes... args) const
		{
			std::lock_guard<std::recursive_mutex> lock(_mutex);
			for (auto &job : _subscribedJobs)
			{
				if (job && *job)
				{
					(*job)(std::forward<TParameterTypes>(args)...);
				}
			}
		}
	};

	using ContractProvider = TContractProvider<>;
}
