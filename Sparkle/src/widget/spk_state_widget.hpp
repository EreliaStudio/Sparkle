#pragma once

#include <vector>
#include <stdexcept>
#include "math/spk_vector2.hpp"
#include "spk_basic_functions.hpp"
#include "widget/components/spk_nine_sliced.hpp"
#include "widget/components/spk_sprite_sheet_renderer.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	template <typename State>
	class StatefulWidget : public Widget
	{
	private:
		spk::WidgetComponent::NineSlicedBox _box;
		spk::WidgetComponent::SpriteSheetRenderer _spriteRenderer;

		std::vector<std::pair<State, int>> _stateSpritePairs;
		size_t _index;

		void _onGeometryChange()
		{
			_box.setGeometry(anchor(), size());
			_box.setLayer(layer());

			_spriteRenderer.setGeometry(anchor() + _box.cornerSize(), size() - _box.cornerSize() * 2);
			_spriteRenderer.setLayer(layer() + 0.01f);
		}

		void _onRender()
		{
			_box.render();
			_spriteRenderer.render();
		}

		void _onUpdate()
		{
			const auto& mouse = Application::activeApplication()->mouse();

			if (mouse.getButton(Mouse::Left) == InputState::Released && hitTest(mouse.position()))
			{
				_setIndex((_index + 1) % _stateSpritePairs.size());
			}
		}

		void _setIndex(const size_t& p_index)
		{
			_index = p_index;
			_spriteRenderer.setSprite(_stateSpritePairs[_index].second);
			updateGeometry();
		}

	public:
		StatefulWidget(Widget* p_parent) :
			StatefulWidget("Anonymous StatefulWidget", p_parent)
		{
			_index = 0;
		}
		
		StatefulWidget(const std::string& p_name, Widget* p_parent) :
			spk::Widget(p_name, p_parent),
			_index(0)
		{

		}

		void eraseState(const State& p_state)
		{
			auto it = std::find_if(_stateSpritePairs.begin(), _stateSpritePairs.end(), 
				[&p_state](const std::pair<State, int>& pair)
				{
					return pair.first == p_state;
				});
			if (it != _stateSpritePairs.end())
			{
				if (_index == std::distance(_stateSpritePairs.begin(), it))
					_setIndex(0);
				_stateSpritePairs.erase(it);
			}
			else
			{
				throw std::invalid_argument("State not found in StatefulWidget");
			}
		}

		void eraseStates()
		{
			_stateSpritePairs.clear();
			_index = 0;
		}

		void addState(const State& p_state, int p_spriteID)
		{
			_stateSpritePairs.push_back(std::make_pair(p_state, p_spriteID));
		}

		void setState(const State& p_state)
		{
			auto it = std::find_if(_stateSpritePairs.begin(), _stateSpritePairs.end(), 
				[&p_state](const std::pair<State, int>& pair)
				{
					return pair.first == p_state;
				});
			if (it != _stateSpritePairs.end())
			{
				_setIndex(std::distance(_stateSpritePairs.begin(), it));
			}
			else
			{
				throw std::invalid_argument("State not found in StatefulWidget");
			}
		}

		spk::WidgetComponent::SpriteSheetRenderer& spriteRenderer()
		{
			return (_spriteRenderer);
		}

		spk::WidgetComponent::NineSlicedBox& box()
		{
			return (_box);
		}

		void setBackgroundSpriteSheet(const spk::SpriteSheet* p_spriteSheet)
		{
			_box.setSpriteSheet(p_spriteSheet);
		}

		void setCornerSize(const spk::Vector2Int& p_cornerSize)
		{
			_box.setCornerSize(p_cornerSize);
		}

		void setStateSpriteSheet(const spk::SpriteSheet* p_spriteSheet)
		{
			_spriteRenderer.setSpriteSheet(p_spriteSheet);
			updateGeometry();
		}
	};
}