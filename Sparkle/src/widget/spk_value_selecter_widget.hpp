#pragma once

#include "widget/spk_button.hpp"
#include "widget/spk_text_label.hpp"

namespace spk
{
	/**
	 * @class ValueSelector
	 * @brief A templated widget class for selecting a value from a list using increment and decrement buttons.
	 *
	 * This class extends Widget and includes buttons for incrementing and decrementing the value, as well as a 
	 * text label for displaying the current value. It is designed to handle both the graphical representation 
	 * and interaction logic for value selection, supporting dynamic resizing and customizable value display.
	 *
	 * ValueSelector can be used to select values of any type specified by the template parameter TType. The 
	 * class provides methods to add values, define a conversion callback for displaying values as strings, 
	 * and manage the currently selected value.
	 *
	 * Usage example:
	 * @code
	 * spk::ValueSelector<int>* myValueSelector = new spk::ValueSelector<int>(parentWidget);
	 * myValueSelector->addValue(1);
	 * myValueSelector->addValue(2);
	 * myValueSelector->addValue(3);
	 * myValueSelector->defineConvertionCallback([](const int &value) { return std::to_string(value); });
	 * myValueSelector->selectValue(2);
	 * @endcode
	 *
	 * @note The class manages layout changes by overriding the `_onGeometryChange` and `_onRender` methods
	 *       to ensure the buttons and value display are correctly sized and rendered.
	 */
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
		/**
		 * @brief Constructor for ValueSelector with an unnamed instance.
		 * @param p_parent The parent widget.
		 */
		ValueSelector(spk::Widget *p_parent) : ValueSelector("Unnamed ValueSelector", p_parent) {}

		/**
		 * @brief Constructor for ValueSelector with a specified name.
		 * @param p_name The name of the ValueSelector.
		 * @param p_parent The parent widget.
		 */
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

		/**
		 * @brief Gets the decrement value button.
		 * @return A pointer to the decrement value button.
		 */
		Button *decrementValueButton()
		{
			return (_decrementValueButton);
		}

		/**
		 * @brief Gets the increment value button.
		 * @return A pointer to the increment value button.
		 */
		Button *incrementValueButton()
		{
			return (_incrementValueButton);
		}

		/**
		 * @brief Gets the value displayer label.
		 * @return A pointer to the value displayer label.
		 */
		TextLabel *valueDisplayer()
		{
			return (_valueDisplayer);
		}

		/**
		 * @brief Defines a callback function for converting values to strings for display.
		 * @param p_callback The callback function to convert values.
		 */
		void defineConvertionCallback(const std::function<std::string(const TType &)> &p_callback)
		{
			_converterCallback = p_callback;
		}

		/**
		 * @brief Adds a value to the ValueSelector.
		 * @param p_value The value to add.
		 */
		void addValue(const TType &p_value)
		{
			_values.push_back(p_value);
		}

		/**
		 * @brief Selects a specific value if it exists in the ValueSelector.
		 * @param p_value The value to select.
		 * @throws std::invalid_argument if the value is not found.
		 */
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

		/**
		 * @brief Sets the index of the currently selected value.
		 * @param p_index The index to set.
		 */
		void setIndex(const size_t& p_index)
		{
			_index = p_index;
			
			_valueEdited = true;
		}

		/**
		 * @brief Gets the currently selected value.
		 * @return The currently selected value.
		 * @throws std::runtime_error if the selected index is invalid.
		 */
		const TType &value()
		{
			if (_values.size() >= _index)
				throw std::runtime_error("Can't access invalid selected data inside ValueSelector");
			return (_values[_index]);
		}
	};
}
