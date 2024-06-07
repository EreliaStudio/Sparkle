#include "widget/spk_text_entry.hpp"
#include "application/spk_application.hpp"

namespace spk
{
	void TextEntry::_onGeometryChange()
	{
		_box.setGeometry(anchor(), size());
		_box.setLayer(layer());

		_label.setGeometry(anchor() + _box.cornerSize() + _padding, size() - _box.cornerSize() * 2 - _padding * 2);
		_label.setLayer(layer() + 0.01f);

		_cursorBox.setLayer(layer() + 0.02f);
	}

	void TextEntry::_onRender()
	{
		_box.render();
		if (_isTextEdited == true)
		{
			_updateRenderedText();
			_isTextEdited = false;
		}
		_label.render();
		if (_isSelected == true && _text != "")
		{
			_cursorBox.render();
		}
	}

	void TextEntry::_updateCursorBox()
	{
		spk::Vector2Int previousTextSize = _label.font()->computeStringSize(_label.text().substr(0, _currentCursorPosition - _openingCursorPosition), _label.textSize(), _label.outlineSize());
		spk::Vector2Int fullTextSize = _label.font()->computeStringSize(_label.text(), _label.textSize(), _label.outlineSize());

		spk::Vector2Int cursorSize = spk::Vector2Int(5, _label.textSize());
		spk::Vector2Int textLabelAnchor = 0;

		switch (_label.horizontalAlignment())
		{
		case spk::HorizontalAlignment::Centered:
		{
			textLabelAnchor.x = (size().x - fullTextSize.x) / 2;
			break;
		}

		case spk::HorizontalAlignment::Right:
		default:
		{
			textLabelAnchor.x = size().x - fullTextSize.x;
			break;
		}
		}

		switch (_label.verticalAlignment())
		{
		case spk::VerticalAlignment::Centered:
		{
			textLabelAnchor.y = (size().y - fullTextSize.y) / 2;
			break;
		}

		case spk::VerticalAlignment::Down:
		default:
		{
			textLabelAnchor.y = size().y - fullTextSize.y;
			break;
		}
		}
		_cursorBox.setGeometry(anchor() + spk::Vector2Int(previousTextSize.x, 0) + textLabelAnchor, cursorSize);
	}

	void TextEntry::_updateSelectionStatus()
	{
		if (spk::Application::activeApplication()->mouse().getButton(spk::Mouse::Left) == spk::InputState::Released)
		{
			if (hitTest(spk::Application::activeApplication()->mouse().position()) == true)
			{
				_isSelected = true;
				_updateCursorBox();
			}
			else
			{
				_isSelected = false;
			}
		}
	}

	void TextEntry::_updateRenderedText()
	{
		if (_text.size() == 0)
		{
			_label.setText(_placeholderText);
		}
		else
		{
			if (_currentCursorPosition == _closingCursorPosition)
				_openingCursorPosition = _currentCursorPosition;

			while (true)
			{
				std::string stringToTest = _text.substr(_openingCursorPosition, _currentCursorPosition - _openingCursorPosition + 1);
				spk::Vector2UInt textSize = _label.font()->computeStringSize(stringToTest, _label.textSize(), _label.outlineSize());

				if (textSize.x > (size().x - (_cursorBox.size().x * 2)))
				{
					_openingCursorPosition++;
					break;
				}
				else if (_openingCursorPosition == 0)
				{
					break;
				}
				else
				{
					_openingCursorPosition--;
				}
			}

			_closingCursorPosition = _currentCursorPosition;
			while (true)
			{
				
				std::string stringToTest = _text.substr(_openingCursorPosition, _closingCursorPosition - _openingCursorPosition);
				spk::Vector2UInt textSize = _label.font()->computeStringSize(stringToTest, _label.textSize(), _label.outlineSize());

				if (textSize.x > (size().x - (_cursorBox.size().x * 2)))
				{	
					_closingCursorPosition--;
					break;
				}
				else if (_closingCursorPosition == _text.size())
				{
					break;
				}
				else
				{
					_closingCursorPosition++;
				}
			}

			_label.setText(_text.substr(_openingCursorPosition, _closingCursorPosition - _openingCursorPosition));
		}

		_updateCursorBox();
	}

	void TextEntry::_appendToText(const wchar_t& newChar)
	{
		if (_currentCursorPosition == _closingCursorPosition)
			_closingCursorPosition++;

		_text.insert(_currentCursorPosition, 1, static_cast<char>(newChar));

		_currentCursorPosition++;

		_updateCursorBox();

		_isTextEdited = true;
	}
	
	void TextEntry::_removeFromText()
	{
		if (_text.size() != 0 && _currentCursorPosition > 0)
		{
			if (_currentCursorPosition == _closingCursorPosition)
				_closingCursorPosition--;

			_text.erase(_currentCursorPosition - 1, 1);

			_currentCursorPosition--;

			_isTextEdited = true;
		}
	}  

	void TextEntry::_deleteFromText()
	{
		if (_currentCursorPosition < _text.length())
		{

			_text.erase(_currentCursorPosition, 1);

			_isTextEdited = true;
		}
	}

	void TextEntry::_moveCursor(int p_delta)
	{
		if (p_delta == -1)
		{
			if (_currentCursorPosition > 0)
			{
				_currentCursorPosition--;

				_isTextEdited = true;
			}
		}
		else
		{
			if (_currentCursorPosition <= _text.size())
			{
				_currentCursorPosition++;

				_isTextEdited = true;
			}
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

	TextEntry::TextEntry(const std::string& p_name, Widget* p_parent) :
		Widget(p_name, p_parent),
		_isSelected(false),
		_currentCursorPosition(0),
		_inputs(
			{
				spk::KeyboardCharInput([&](){ _appendToText(spk::Application::activeApplication()->keyboard().getChar()); _isTextEdited = true; }),
				spk::KeyboardInput(spk::Keyboard::LeftArrow, spk::InputState::Down, 150, [&](){_moveCursor(-1);}),
				spk::KeyboardInput(spk::Keyboard::RightArrow, spk::InputState::Down, 150, [&](){_moveCursor(1);}),
				spk::KeyboardInput(spk::Keyboard::Backspace, spk::InputState::Down, 150, [&](){_removeFromText();}),
				spk::KeyboardInput(spk::Keyboard::Delete, spk::InputState::Down, 150, [&](){_deleteFromText();})
			}
		),
		_cursorBox(),
		_placeholderText("Enter text ..."),
		_text(""),
		_isTextEdited(true),
		_padding(0)
	{
		_cursorBox.setColor(spk::Color(10, 10, 10, 255));
	}

	TextEntry::TextEntry(Widget* p_parent) :
		TextEntry("Anonymous TextEntry", p_parent)
	{

	}

	void TextEntry::setPlaceholder(const std::string& p_placeholderText)
	{
		_placeholderText = p_placeholderText;
		
		_isTextEdited = true;
	}
	
	void TextEntry::setPredefinedText(const std::string& p_text)
	{
		_text = p_text;
		
		_isTextEdited = true;
	}

	spk::WidgetComponent::FontRenderer& TextEntry::label()
	{
		return (_label);
	}

	spk::WidgetComponent::ColoredBox& TextEntry::cursor()
	{
		return (_cursorBox);
	}

	spk::WidgetComponent::NineSlicedBox& TextEntry::box()
	{
		return (_box);
	}

	const std::string& TextEntry::placeholder() const
	{
		return (_placeholderText);
	}
	
	const std::string& TextEntry::text() const
	{
		return (_text);
	}
}