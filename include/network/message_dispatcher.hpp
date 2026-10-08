#pragma once

#include "container/thread_safe_fifo.hpp"
#include "design_pattern/contract_provider.hpp"
#include "exception.hpp"
#include "network/message.hpp"

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace spk
{
	// TIncoming is Message or an envelope with a public Message member named message.
	template <typename TIncoming = Message>
	class MessageDispatcher final
	{
	public:
		using Provider = ContractProvider<const TIncoming &>;
		using Callback = typename Provider::callback_type;
		using Contract = typename Provider::Contract;
		using MessageQueue = ThreadSafeFIFO<TIncoming>;
		using TreatmentProvider = ContractProvider<>;
		using TreatmentContract = TreatmentProvider::Contract;

	private:
		TreatmentProvider _treatmentProvider;
		std::mutex _mutex;
		std::unordered_map<Message::Type, std::shared_ptr<Provider>> _subscriptions;
		std::vector<TIncoming> _pending;
		std::size_t _next = 0;
		std::atomic_flag _treating = ATOMIC_FLAG_INIT;

		struct TreatmentGuard
		{
			std::atomic_flag &flag;
			~TreatmentGuard()
			{
				flag.clear();
			}
		};

		[[nodiscard]] static Message::Type _type(const TIncoming &incoming)
		{
			if constexpr (std::is_same_v<TIncoming, Message>)
			{
				return incoming.type();
			}
			else
			{
				return incoming.message.type();
			}
		}

		void _dispatch(const TIncoming &incoming)
		{
			std::shared_ptr<Provider> provider;
			{
				const std::scoped_lock lock(_mutex);
				auto found = _subscriptions.find(_type(incoming));
				if (found == _subscriptions.end())
				{
					return;
				}
				provider = found->second;
			}
			provider->trigger(incoming);
		}

	public:
		// Runs on the treating thread, even for an empty queue, before message callbacks.
		[[nodiscard]] TreatmentContract subscribeToTreatment(TreatmentProvider::callback_type callback)
		{
			if (!callback)
			{
				throw Exception("Cannot subscribe an empty treatment callback.");
			}
			return _treatmentProvider.subscribe(std::move(callback));
		}
		[[nodiscard]] Contract subscribeTo(Message::Type type, Callback callback)
		{
			if (!callback)
			{
				throw Exception("Cannot subscribe an empty message callback.");
			}
			std::shared_ptr<Provider> provider;
			{
				const std::scoped_lock lock(_mutex);
				auto &slot = _subscriptions[type];
				if (slot == nullptr)
				{
					slot = std::make_shared<Provider>();
				}
				provider = slot;
			}
			return provider->subscribe(std::move(callback));
		}

		// Callbacks run synchronously. Unknown types are consumed without notification.
		// A throwing callback consumes its message; remaining messages resume next call.
		// Recursive/concurrent treatment is rejected. Use one queue consumer.
		void treatMessages(MessageQueue &messages)
		{
			if (_treating.test_and_set())
			{
				throw Exception("Message treatment is already in progress.");
			}
			const TreatmentGuard guard{_treating};
			_treatmentProvider.trigger();
			if (_next == _pending.size())
			{
				_next = 0;
				(void)messages.drain(_pending);
			}
			while (_next < _pending.size())
			{
				_dispatch(_pending[_next++]);
			}
			_pending.clear();
			_next = 0;
		}
	};
}
