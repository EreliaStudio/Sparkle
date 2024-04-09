#include "widget/spk_text_entry.hpp"
#include "application/spk_application.hpp"

namespace spk
{
    void TextEntry::_onGeometryChange()
    {
        _box.setGeometry(anchor(), size());
        _box.setLayer(layer());

        _label.setGeometry(anchor() + _box.cornerSize(), size() - _box.cornerSize() * 2);
        _label.setLayer(layer() + 0.01f);
    }

    void TextEntry::_onRender()
    {
        _box.render();
        _label.render();
    }

	void TextEntry::_updateSelectionStatus()
	{
		if (spk::Application::activeApplication()->mouse().getButton(spk::Mouse::Left) == spk::InputState::Released)
		{
			if (hitTest(spk::Application::activeApplication()->mouse().position()) == true)
			{
				_isSelected = true;
			}
			else
			{
				_isSelected = false;
			}
		}
	}

	void TextEntry::_appendToText(const wchar_t& newChar)
	{
		std::string currentText = _label.text();

		currentText.insert(_cursorPosition, 1, static_cast<char>(newChar)); 

		_label.setText(currentText);

		_cursorPosition++;
	}
	
	void TextEntry::_removeFromText()
	{
		std::string currentText = _label.text();

		if (currentText.size() != 0 && _cursorPosition > 0)
		{
			currentText.erase(_cursorPosition - 1, 1);
			_label.setText(currentText);
			_cursorPosition--;
		}
	}  

	void TextEntry::_deleteFromText()
	{
		std::string currentText = _label.text();

		if (_cursorPosition < currentText.length())
		{
			currentText.erase(_cursorPosition, 1);
			_label.setText(currentText);
		}
	}

	void TextEntry::_onUpdate()
	{
		_updateSelectionStatus();

		if (_isSelected == true)
		{
			for (auto& element : _inputs)
			{
				element.update();
			}
		}
	}

    TextEntry::TextEntry(Widget* p_parent) :
        Widget(p_parent),
		_isSelected(false),
		_cursorPosition(0),
		_inputs(
			{
				spk::KeyboardCharInput([&](){ _appendToText(spk::Application::activeApplication()->keyboard().getChar()); }),
				spk::KeyboardInput(spk::Keyboard::LeftArrow, spk::InputState::Down, 150, [&](){if (_cursorPosition > 0) _cursorPosition--;}),
				spk::KeyboardInput(spk::Keyboard::RightArrow, spk::InputState::Down, 150, [&](){if (_cursorPosition <= _label.text().size()) _cursorPosition++;}),
				spk::KeyboardInput(spk::Keyboard::Backspace, spk::InputState::Down, 150, [&](){_removeFromText();}),
				spk::KeyboardInput(spk::Keyboard::Delete, spk::InputState::Down, 150, [&](){_deleteFromText();})
			}
		)
    {

    }

    spk::WidgetComponent::TextLabel& TextEntry::label()
    {
        return (_label);
    }

    spk::WidgetComponent::NineSlicedBox& TextEntry::box()
    {
        return (_box);
    }
}