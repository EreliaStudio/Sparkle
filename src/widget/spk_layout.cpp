#include "widget/spk_layout.hpp"

namespace spk
{
	Layout::Element::Element() : 
		_widget(nullptr),
		_sizePolicy(SizePolicy::Extend),
		_size({1, 1})
	{
	}

	Layout::Element::Element(spk::SafePointer<spk::Widget> p_widget, const SizePolicy &p_sizePolicy, const spk::Vector2UInt &p_size) :
		_widget(p_widget),
		_sizePolicy(p_sizePolicy),
		_size(p_size)
	{
	}

	const spk::SafePointer<spk::Widget> &Layout::Element::widget() const
	{
		return (_widget);
	}

	void Layout::Element::setSizePolicy(const Layout::SizePolicy &p_sizePolicy)
	{
		_sizePolicy = p_sizePolicy;
	}
	const Layout::SizePolicy &Layout::Element::sizePolicy() const
	{
		return (_sizePolicy);
	}

	void Layout::Element::setSize(const spk::Vector2UInt &p_size)
	{
		_size = p_size;
	}
	const spk::Vector2UInt &Layout::Element::size() const
	{
		return (_size);
	}

	void Layout::resizeElements(const size_t &p_size)
	{
		_elements.resize(p_size);
	}

	spk::SafePointer<Layout::Element> Layout::addWidget(spk::SafePointer<spk::Widget> p_widget, const SizePolicy &p_sizePolicy)
	{
		_elements.push_back(std::unique_ptr<Element>(new Element(p_widget, p_sizePolicy, {1, 1})));

		return (_elements.back().get());
	}

	void Layout::removeWidget(spk::SafePointer<Element> p_widget)
	{
		if (p_widget == nullptr)
		{
			return;
		}

		for (auto it = _elements.begin(); it != _elements.end(); ++it)
		{
			if (it->get() == p_widget.get())
			{
				_elements.erase(it);
				resizeElements(_elements.size());
				break;
			}
		}
	}

	void Layout::setElementPadding(const spk::Vector2UInt &p_padding)
	{
		_elementPadding = p_padding;
	}

	const spk::Vector2UInt &Layout::elementPadding() const
	{
		return (_elementPadding);
	}

	const std::vector<std::unique_ptr<Layout::Element>> &Layout::elements() const
	{
		return _elements;
	}

	std::vector<std::unique_ptr<Layout::Element>> &Layout::elements()
	{
		return _elements;
	}
}