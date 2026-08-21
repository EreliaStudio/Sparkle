#include "resizeable_trait.hpp"

#include <utility>

namespace spk
{
	ResizeableTrait::~ResizeableTrait() = default;

	void ResizeableTrait::_triggerEdition()
	{
		_onSizeHintEditionContractProvider.trigger(this);
	}

	const ResizeableTrait::SizeHint &ResizeableTrait::sizeHint() const
	{
		return _sizeHint;
	}

	const Vector2 &ResizeableTrait::minimalSize() const
	{
		return _sizeHint.minimal;
	}

	const Vector2 &ResizeableTrait::maximalSize() const
	{
		return _sizeHint.maximal;
	}

	const Vector2 &ResizeableTrait::preferredSize() const
	{
		return _sizeHint.preferred;
	}

	void ResizeableTrait::setSizeHint(const SizeHint &sizeHint)
	{
		if (_sizeHint == sizeHint)
		{
			return;
		}

		_sizeHint = sizeHint;
		_triggerEdition();
	}

	void ResizeableTrait::setMinimalSize(const Vector2 &size)
	{
		if (_sizeHint.minimal == size)
		{
			return;
		}

		_sizeHint.minimal = size;
		_triggerEdition();
	}

	void ResizeableTrait::setMaximalSize(const Vector2 &size)
	{
		if (_sizeHint.maximal == size)
		{
			return;
		}

		_sizeHint.maximal = size;
		_triggerEdition();
	}

	void ResizeableTrait::setPreferredSize(const Vector2 &size)
	{
		if (_sizeHint.preferred == size)
		{
			return;
		}

		_sizeHint.preferred = size;
		_triggerEdition();
	}

	ResizeableTrait::Contract ResizeableTrait::subscribeToSizeHintEdition(callback_type job)
	{
		return _onSizeHintEditionContractProvider.subscribe(std::move(job));
	}
}
