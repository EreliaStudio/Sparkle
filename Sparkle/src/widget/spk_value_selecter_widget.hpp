#pragma once

#include "widget/spk_button.hpp"
#include "widget/spk_text_label.hpp"

namespace spk
{
	template <typename TType>
	class ValueSelector : public spk::Widget
	{
	private:
		void _onUpdate() override
		{

		}

		void _onGeometryChange() override
		{
			spk::Vector2Int buttonSize = size().y;
			spk::Vector2Int space = spk::Vector2Int(10, 0);
			spk::Vector2Int labelSize = spk::Vector2Int(size().x - buttonSize.x * 2 - space.x * 2, size().y);

			_decrementValueButton->setGeometry(spk::Vector2Int(0, 0), buttonSize);
			_decrementValueButton->setLayer(layer() + 0.01f);

			_valueDisplayer->setGeometry(_decrementValueButton->anchor() + _decrementValueButton->size() * spk::Vector2Int(1, 0) + space, labelSize);
			_valueDisplayer->setLayer(layer() + 0.01f);

			_incrementValueButton->setGeometry(_valueDisplayer->anchor() + _valueDisplayer->size() * spk::Vector2Int(1, 0) + space, buttonSize);
			_incrementValueButton->setLayer(layer() + 0.01f);

		}

		void _onRender() override
		{
			if (_valueEdited == true)
			{
				_valueDisplayer->label().setText(_converterCallback != nullptr ? _converterCallback(_values[_index]) : "Invalid converter");

				spk::Font::Size fontSize = std::min({
					_decrementValueButton->label().fontSize(),
					_valueDisplayer->computeOptimalFontSize(0.1f)
				});
				_valueDisplayer->label().setFontSize(fontSize);

				_valueEdited = false;	
			}
		}

		bool _valueEdited;
		std::vector<TType> _values;
		size_t _index;
		std::function<std::string(const TType &)> _converterCallback;

		spk::Button *_decrementValueButton;
		spk::Button *_incrementValueButton;
		spk::TextLabel *_valueDisplayer;
		

	public:
		ValueSelector(spk::Widget *p_parent) : ValueSelector("Unnamed ValueSelector", p_parent) {}
		ValueSelector(const std::string &p_name, spk::Widget *p_parent) :
			spk::Widget(p_name, p_parent),
			_decrementValueButton(makeChild<Button>(p_name + " - Decrement button", this)),
			_incrementValueButton(makeChild<Button>(p_name + " - Increment button", this)),
			_valueDisplayer(makeChild<TextLabel>(p_name + " - Value Displayer", this)),
			_converterCallback([&](const TType &value) -> std::string
			{
				std::stringstream ss;
				ss << value;
				return ss.str();
			}),
			_index(0)
		{
			_decrementValueButton->setText("-");
			_decrementValueButton->activate();
			_decrementValueButton->setOnClickCallback([&]()
			{
				_index += _values.size() - 1;
				_index %= _values.size();

				_valueDisplayer->label().setText(_converterCallback != nullptr ? _converterCallback(_values[_index]) : "Invalid converter");
				_valueEdited = true;
			});

			_incrementValueButton->setText("+");
			_incrementValueButton->activate();
			_incrementValueButton->setOnClickCallback([&]()
			{
				_index += 1;
				_index %= _values.size();

				_valueDisplayer->label().setText(_converterCallback != nullptr ? _converterCallback(_values[_index]) : "Invalid converter");
				_valueEdited = true;
			});
			
			_valueDisplayer->label().setVerticalAlignment(spk::VerticalAlignment::Centered);
			_valueDisplayer->label().setHorizontalAlignment(spk::HorizontalAlignment::Centered);
			_valueDisplayer->activate();
		}

		Button *decrementValueButton()
		{
			return (_decrementValueButton);
		}

		Button *incrementValueButton()
		{
			return (_incrementValueButton);
		}

		TextLabel *valueDisplayer()
		{
			return (_valueDisplayer);
		}

		void defineConvertionCallback(const std::function<std::string(const TType &)> &p_callback)
		{
			_converterCallback = p_callback;
		}

		void addValue(const TType &p_value)
		{
			_values.push_back(p_value);
		}

		void selectValue(const TType &p_value)
		{
			auto it = std::find(_values.begin(), _values.end(), p_value);
			if (it != _values.end())
			{
				setIndex(std::distance(_values.begin(), it));
			}
			else
			{
				throw std::invalid_argument("Value not found in ValueSelector");
			}
		}

		void setIndex(const size_t& p_index)
		{
			_index = p_index;
			
			_valueEdited = true;
		}

		const TType &value()
		{
			if (_values.size() >= _index)
				throw std::runtime_error("Can't access invalid selected data inside ValueSelector");
			return (_values[_index]);
		}
	};
		
}