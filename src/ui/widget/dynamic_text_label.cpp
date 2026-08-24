#include "ui/widget/dynamic_text_label.hpp"

#include <utility>

#include "core/context/update_context.hpp"

namespace spk
{
	DynamicTextLabel::DynamicTextLabel(std::string name, Widget *parent) :
		TextLabel(std::move(name), parent)
	{
	}

	DynamicTextLabel::DynamicTextLabel(std::string name, Font *font, TextProducer producer, Widget *parent) :
		TextLabel(std::move(name), font, parent)
	{
		setTextProducer(std::move(producer));
	}

	void DynamicTextLabel::_updateState(UpdateContext &context)
	{
		if (!_producer)
		{
			return;
		}
		if (_refreshDuration <= Duration::zero())
		{
			refresh();
			return;
		}

		_elapsed += context.deltaTime;
		if (_elapsed >= _refreshDuration)
		{
			refresh();
		}
	}

	void DynamicTextLabel::setTextProducer(TextProducer producer)
	{
		_producer = std::move(producer);
		refresh();
	}

	void DynamicTextLabel::setRefreshDuration(Duration duration)
	{
		_refreshDuration = duration;
		_elapsed = Duration::zero();
	}

	void DynamicTextLabel::refresh()
	{
		_elapsed = Duration::zero();
		if (_producer)
		{
			setText(_producer());
		}
	}

	const DynamicTextLabel::TextProducer &DynamicTextLabel::textProducer() const noexcept
	{
		return _producer;
	}

	DynamicTextLabel::Duration DynamicTextLabel::refreshDuration() const noexcept
	{
		return _refreshDuration;
	}
}
