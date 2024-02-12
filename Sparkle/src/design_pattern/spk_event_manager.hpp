#pragma once

#include <vector>
#include "design_pattern/spk_notifier.hpp"

namespace spk
{
	template <typename TEventType>
	class EventManager
	{
	public:
		using Callback = spk::Notifier::Callback;
		using Contract = spk::Notifier::Contract;

	private:
		std::vector<spk::Notifier> _notifiers;

	public:
		EventManager()
		{

		}

		std::unique_ptr<Contract> subscribe(const TEventType& p_event, const Callback& p_callback)
		{
			size_t index = static_cast<size_t>(p_event);

			if (_notifiers.size() <= index)
				_notifiers.resize(index + 1);

			std::unique_ptr<Contract> newContract = _notifiers[index].subscribe(p_callback);
			return (newContract);
		}

		void notify(const TEventType& p_event)
		{
			size_t index = static_cast<size_t>(p_event);

			if (_notifiers.size() <= index)
				_notifiers.resize(index + 1);

			_notifiers[index].notify_all();
		}
	};
}