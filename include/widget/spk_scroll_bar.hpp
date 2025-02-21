#pragma once

#include "widget/spk_widget.hpp"
#include "widget/spk_slider_bar.hpp"
#include "widget/spk_push_button.hpp"

namespace spk
{
	class ScrollBar : public spk::Widget
	{
	public:
		using Contract = spk::SliderBar::Contract;
		using Job = spk::SliderBar::Job;
		using VoidJob = spk::SliderBar::VoidJob;
		using Orientation = spk::SliderBar::Orientation;
		
	private:
		spk::TContractProvider<float> _onEditionContractProvider;

		spk::PushButton _positiveButton;
		spk::PushButton::Contract _positiveButtonContract;
		spk::PushButton _negativeButton;
		spk::PushButton::Contract _negativeButtonContract;
		spk::SliderBar _sliderBar;
		spk::SliderBar::Contract _sliderBarContract;

		void _onGeometryChange() override;
		
	public:
		ScrollBar(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);

		Contract subscribe(const Job& p_job);
		Contract subscribe(const VoidJob& p_job);

		void setOrientation(const Orientation& p_orientation);

		void setScale(const float p_scale);

		float ratio();
		void setRatio(const float& p_ratio);
	};
}