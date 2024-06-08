#include "widget/spk_button.hpp"

#include "application/spk_application.hpp"

namespace spk
{
    void Button::_configureFontRendererGeometry()
    {
        for (size_t i = 0; i < 2; i++)
        {
            _labels[i].setGeometry(anchor() + _boxes[i].cornerSize() + _padding, size() - _boxes[i].cornerSize() * 2 - _padding * 2);
			_labels[i].setLayer(layer() + 0.01f);
        }
    }

    void Button::_onGeometryChange()
    {
        for (size_t i = 0; i < 2; i++)
        {
            _boxes[i].setGeometry(anchor(), size());
			_boxes[i].setLayer(layer());
        }
        _configureFontRendererGeometry();
    }

    void Button::_onRender()
    {
        _boxes[static_cast<int>(_stateMachine.state())].render();
        _labels[static_cast<int>(_stateMachine.state())].render();
    }

    void Button::_onUpdate()
    {
        _stateMachine.update();
    }

    Button::Button(const std::string& p_name, Widget* p_parent) :
        Widget(p_parent),
        _onClickCallback(nullptr),
        _stateMachine(),
        _padding(0)
    {
        _stateMachine.addState(State::Released, StateMachine<State>::Action(
                                                    [&]() {},
                                                    [&]()
                                                    {
                                                        const auto& mouse = Application::activeApplication()->mouse();

                                                        if (mouse.getButton(Mouse::Left) == InputState::Down &&
                                                            hitTest(mouse.position()) == true)
                                                        {
                                                            _stateMachine.enterState(State::Pressed);
                                                        }
                                                    },
                                                    [&]() {}));
        _stateMachine.addState(State::Pressed, StateMachine<State>::Action(
                                                   [&]()
                                                   {
                                                       if (_onClickCallback != nullptr)
                                                           _onClickCallback();
                                                   },
                                                   [&]()
                                                   {
                                                       const auto& mouse = Application::activeApplication()->mouse();

                                                       if (mouse.getButton(Mouse::Left) == InputState::Released ||
                                                           hitTest(mouse.position()) == false)
                                                       {
                                                           _stateMachine.enterState(State::Released);
                                                       }
                                                   },
                                                   [&]() {}));

        _stateMachine.enterState(State::Released);
    }

    Button::Button(Widget* p_parent) :
        Button("Anonymous Button", p_parent)
    {
    }

    spk::Font::Size Button::computeOptimalFontSize(const float& p_ratio)
    {
		if (needGeometryChange() == true)
			applyGeometryChange();

		if (_labels[0].font() == nullptr || _labels[1].font() == nullptr)
			throw std::runtime_error("No font set inside Button::computeOptimalFontSize");

        return (std::min({_labels[0].font()->computeOptimalTextSize(_labels[0].text(), p_ratio, _labels[0].size()),
                          _labels[1].font()->computeOptimalTextSize(_labels[1].text(), p_ratio, _labels[1].size())}));
    }

    void Button::setPadding(const spk::Vector2Int& p_padding)
    {
        _padding = p_padding;
        updateGeometry();
    }

    void Button::setOnClickCallback(const Callback p_onClickCallback)
    {
        _onClickCallback = p_onClickCallback;
    }

    WidgetComponent::NineSlicedBox& Button::box(const State& p_state)
    {
        return (_boxes[static_cast<int>(p_state)]);
    }

    WidgetComponent::FontRenderer& Button::label(const State& p_state)
    {
        return (_labels[static_cast<int>(p_state)]);
    }

    void Button::setCornerSize(const spk::Vector2Int& p_cornerSize)
	{
		for (size_t i = 0; i < 2; i++)
        {
            _boxes[i].setCornerSize(p_cornerSize);
        }
        updateGeometry();
	}

	void Button::setSpriteSheet(const spk::SpriteSheet* p_releasedSpriteSheet, const spk::SpriteSheet* p_pressedSpriteSheet)
	{
		_boxes[static_cast<int>(State::Released)].setSpriteSheet(p_releasedSpriteSheet);
		_boxes[static_cast<int>(State::Pressed)].setSpriteSheet(p_pressedSpriteSheet);
	}

    void Button::setFont(spk::Font* p_font)
	{
        for (size_t i = 0; i < 2; i++)
        {
            _labels[i].setFont(p_font);
        }
	}

    void Button::setFontSize(const spk::Font::Size& p_fontSize)
	{
        for (size_t i = 0; i < 2; i++)
        {
            _labels[i].setFontSize(p_fontSize);
        }
	}

    void Button::setText(const std::string& p_text)
    {
        for (size_t i = 0; i < 2; i++)
        {
            _labels[i].setText(p_text);
        }
    }

    void Button::setTextColor(const spk::Color& p_textColor)
	{
        for (size_t i = 0; i < 2; i++)
        {
            _labels[i].setTextColor(p_textColor);
        }
	}

    void Button::setTextSize(const size_t& p_textSize)
	{
        for (size_t i = 0; i < 2; i++)
        {
            _labels[i].setTextSize(p_textSize);
        }
	}

    void Button::setTextEdgeStrenght(const float& p_textEdgeStrenght)
	{
        for (size_t i = 0; i < 2; i++)
        {
            _labels[i].setTextEdgeStrenght(p_textEdgeStrenght);
        }
	}

    void Button::setOutlineColor(const spk::Color& p_outlineColor)
	{
        for (size_t i = 0; i < 2; i++)
        {
            _labels[i].setOutlineColor(p_outlineColor);
        }
	}

    void Button::setOutlineSize(const size_t& p_outlineSize)
	{
        for (size_t i = 0; i < 2; i++)
        {
            _labels[i].setOutlineSize(p_outlineSize);
        }
	}

    void Button::setOutlineEdgeStrenght(const float& p_outlineEdgeStrenght)
	{
        for (size_t i = 0; i < 2; i++)
        {
            _labels[i].setOutlineEdgeStrenght(p_outlineEdgeStrenght);
        }
	}

    void Button::setVerticalAlignment(const spk::VerticalAlignment& p_verticalAlignment)
	{
        for (size_t i = 0; i < 2; i++)
        {
            _labels[i].setVerticalAlignment(p_verticalAlignment);
        }
	}

    void Button::setHorizontalAlignment(const spk::HorizontalAlignment& p_horizontalAlignment)
	{
        for (size_t i = 0; i < 2; i++)
        {
            _labels[i].setHorizontalAlignment(p_horizontalAlignment);
        }
	}
}