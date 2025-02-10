#pragma once

#include "widget/spk_widget.hpp"
#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"
#include "structure/math/spk_vector2.hpp"

#include "structure/graphics/renderer/spk_nine_slice_renderer.hpp"

namespace spk
{
	class SliderBar : public spk::Widget
	{
	public:
		enum class Orientation
		{
			Horizontal,
			Vertical
		};

		using Job = spk::ContractProvider::Job;
		using Contract = spk::ContractProvider::Contract;
	
	private:
		static spk::SpriteSheet _defaultSliderBody;

		spk::Vector2UInt _cornerSize;
		spk::Vector2UInt _bodyCornerSize;
		spk::NineSliceRenderer _backgroundRenderer;
		spk::NineSliceRenderer _bodyRenderer;

		spk::ContractProvider _onEditionContractProvider;

		Orientation _orientation = Orientation::Horizontal;

		bool _isClicked = false;
		float _scale = 0;
		float _ratio = 0;
		float _unit = 1.0f;
		float _minValue = 0;
		float _maxValue = 100;

		void _onGeometryChange() override;
		void _onPaintEvent(spk::PaintEvent& p_event) override;
		void _onMouseEvent(spk::MouseEvent& p_event) override;

	public:
		SliderBar(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);

		Contract subscribe(const Job& p_job);

		void setCornerSize(const spk::Vector2UInt& p_cornerSize);
		void setBodyCornerSize(const spk::Vector2UInt& p_bodyCornerSize);
		void setSpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet);
		void setBodySpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet);
		void setScale(const float& p_scale);
		void setRange(float p_minValue, float p_maxValue);

		float value();
	};

}