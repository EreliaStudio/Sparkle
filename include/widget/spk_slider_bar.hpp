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

		using Job = spk::TContractProvider<float>::Job;
		using VoidJob = spk::TContractProvider<float>::VoidJob;
		using Contract = spk::TContractProvider<float>::Contract;
	
	private:
		class Body : public spk::Widget
		{
		private:
			spk::NineSliceRenderer _renderer;
			spk::Vector2UInt _cornerSize;

			void _onGeometryChange()
			{
				_renderer.clear();
				_renderer.prepare(geometry(), layer(), _cornerSize);
				_renderer.validate();
			}

			void _onPaintEvent(spk::PaintEvent& p_event)
			{
				_renderer.render();
			}

		public:
			Body(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
				spk::Widget(p_name, p_parent)
			{

			}
        	
			void setSpriteSheet(const SafePointer<SpriteSheet>& p_spriteSheet)
			{
				_renderer.setSpriteSheet(p_spriteSheet);
			}

			void setCornerSize(const spk::Vector2UInt& p_cornerSize)
			{
				_cornerSize = p_cornerSize;
				requireGeometryUpdate();
			}
		};

		static spk::SpriteSheet _defaultSliderBody;

		spk::Vector2UInt _cornerSize;
		spk::NineSliceRenderer _backgroundRenderer;
		Body _body;


		spk::TContractProvider<float> _onEditionContractProvider;

		Orientation _orientation = Orientation::Horizontal;

		bool _isClicked = false;
		spk::Vector2UInt _clickedMousePosition;
		float _clickedRatio;
		float _scale = 0;
		float _ratio = 0;
		float _minValue = 0;
		float _maxValue = 100;

		void _onGeometryChange() override;
		void _onPaintEvent(spk::PaintEvent& p_event) override;
		void _onMouseEvent(spk::MouseEvent& p_event) override;

	public:
		SliderBar(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);

		Contract subscribe(const Job& p_job);

		void setOrientation(const Orientation& p_orientation);
		void setCornerSize(const spk::Vector2UInt& p_cornerSize);
		void setBodyCornerSize(const spk::Vector2UInt& p_bodyCornerSize);
		void setSpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet);
		void setBodySpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet);
		void setScale(const float& p_scale);
		void setRange(float p_minValue, float p_maxValue);
		void setRatio(float p_ratio);

		float value();
		float ratio();
		const Orientation& orientation() const;
	};

}