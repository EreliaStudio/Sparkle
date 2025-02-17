#include "widget/spk_scroll_bar.hpp"

namespace spk
{
	void ScrollBar::_onGeometryChange()
	{
		switch (_sliderBar.orientation())
		{
		case Orientation::Horizontal:
		{
			spk::Vector2UInt buttonSize = geometry().size.y;
			spk::Vector2UInt sliderSize = spk::Vector2UInt(geometry().size.x - buttonSize.x * 2, geometry().size.y);

			_negativeButton.setGeometry({0, geometry().size.y - buttonSize.y}, buttonSize);
			_sliderBar.setBodyCornerSize(sliderSize.y / 2);
			_sliderBar.setGeometry({buttonSize.x, geometry().size.y - buttonSize.y}, sliderSize);
			_positiveButton.setGeometry({buttonSize.x + sliderSize.x, geometry().size.y - buttonSize.y}, buttonSize);
			break;
		}
		case Orientation::Vertical:
		{
			spk::Vector2UInt buttonSize = geometry().size.x;
			spk::Vector2UInt sliderSize = spk::Vector2UInt(geometry().size.x, geometry().size.y - buttonSize.y * 2);

			_negativeButton.setGeometry({geometry().size.x - buttonSize.x, 0}, buttonSize);
			_sliderBar.setBodyCornerSize(sliderSize.x / 2);
			_sliderBar.setGeometry({geometry().size.x - buttonSize.x, buttonSize.y}, sliderSize);
			_positiveButton.setGeometry({geometry().size.x - buttonSize.x, buttonSize.y + sliderSize.y}, buttonSize);

			break;
		}
		}
	}		
	
	ScrollBar::ScrollBar(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_positiveButton(p_name + L" - Positive button", this),
		_negativeButton(p_name + L" - Negative button", this),
		_sliderBar(p_name + L" - Slider bar", this)
	{
		_positiveButton.setCornerSize(2);
		_positiveButton.setIconset(spk::Widget::defaultIconset());
		_positiveButton.activate();

		_positiveButtonContract = _positiveButton.subscribe([&](){
				_sliderBar.setRatio(_sliderBar.ratio() + 0.1f);
				_onEditionContractProvider.trigger(_sliderBar.ratio());
			});
		
		_negativeButton.setCornerSize(2);
		_negativeButton.setIconset(spk::Widget::defaultIconset());
		_negativeButton.activate();
		_negativeButtonContract = _negativeButton.subscribe([&](){
				_sliderBar.setRatio(_sliderBar.ratio() - 0.1f);
				_onEditionContractProvider.trigger(_sliderBar.ratio());
			});

		_sliderBar.activate();
		_sliderBarContract = _sliderBar.subscribe([&](const float& p_ratio){_onEditionContractProvider.trigger(_sliderBar.ratio());});
	}

	ScrollBar::Contract ScrollBar::subscribe(const Job& p_job)
	{
		return (_onEditionContractProvider.subscribe(p_job));
	}

	ScrollBar::Contract ScrollBar::subscribe(const VoidJob& p_job)
	{
		return (_onEditionContractProvider.subscribe(p_job));
	}

	void ScrollBar::setOrientation(const Orientation& p_orientation)
	{
		_sliderBar.setOrientation(p_orientation);
		switch (p_orientation)
		{
		case Orientation::Horizontal:
		{
			_positiveButton.setSprite(spk::Widget::defaultIconset()->sprite(7));
			_negativeButton.setSprite(spk::Widget::defaultIconset()->sprite(6));
			break;
		}
		case Orientation::Vertical:
		{
			_positiveButton.setSprite(spk::Widget::defaultIconset()->sprite(5));
			_negativeButton.setSprite(spk::Widget::defaultIconset()->sprite(4));
			break;
		}
		}
	}

	void ScrollBar::setScale(const float p_scale)
	{
		_sliderBar.setScale(p_scale);
	}

	float ScrollBar::ratio()
	{
		return (_sliderBar.ratio());
	}

	void ScrollBar::setRatio(const float& p_ratio)
	{
		_sliderBar.setRatio(p_ratio);
		_onEditionContractProvider.trigger(_sliderBar.ratio());
	}
}