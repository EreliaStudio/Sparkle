#include "widget/spk_layout.hpp"
#include <limits>

namespace spk
{
       Layout::Element::Element() :
               _widget(nullptr),
               _layout(nullptr),
               _sizePolicy(SizePolicy::Extend),
               _size({1, 1})
       {
       }

       Layout::Element::Element(spk::SafePointer<spk::Widget> p_widget, const SizePolicy &p_sizePolicy, const spk::Vector2UInt &p_size) :
               _widget(p_widget),
               _layout(nullptr),
               _sizePolicy(p_sizePolicy),
               _size(p_size)
       {
       }

       Layout::Element::Element(spk::SafePointer<spk::Layout> p_layout, const SizePolicy &p_sizePolicy, const spk::Vector2UInt &p_size) :
               _widget(nullptr),
               _layout(p_layout),
               _sizePolicy(p_sizePolicy),
               _size(p_size)
       {
       }

       const spk::SafePointer<spk::Widget> &Layout::Element::widget() const
       {
               return (_widget);
       }

       const spk::SafePointer<spk::Layout> &Layout::Element::layout() const
       {
               return (_layout);
       }

       bool Layout::Element::isWidget() const
       {
               return (_widget != nullptr);
       }

       bool Layout::Element::isLayout() const
       {
               return (_layout != nullptr);
       }

       spk::Vector2UInt Layout::Element::minimalSize() const
       {
               if (_widget != nullptr)
               {
                       return _widget->minimalSize();
               }
               if (_layout != nullptr)
               {
                       return _layout->minimalSize();
               }
               return {0u, 0u};
       }

       spk::Vector2UInt Layout::Element::maximalSize() const
       {
               if (_widget != nullptr)
               {
                       return _widget->maximalSize();
               }
               if (_layout != nullptr)
               {
                       return _layout->maximalSize();
               }
               return {0u, 0u};
       }

       void Layout::Element::setGeometry(const spk::Geometry2D &p_geometry)
       {
               if (_widget != nullptr)
               {
                       _widget->setGeometry(p_geometry);
               }
               else if (_layout != nullptr)
               {
                       _layout->setGeometry(p_geometry);
               }
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

	void Layout::clear()
	{
		_elements.clear();
	}

       spk::SafePointer<Layout::Element> Layout::addWidget(spk::SafePointer<spk::Widget> p_widget, const SizePolicy &p_sizePolicy)
       {
               _elements.push_back(std::unique_ptr<Element>(new Element(p_widget, p_sizePolicy, {0, 0})));

               return (_elements.back().get());
       }

       spk::SafePointer<Layout::Element> Layout::addLayout(spk::SafePointer<spk::Layout> p_layout, const SizePolicy &p_sizePolicy)
       {
               _elements.push_back(std::unique_ptr<Element>(new Element(p_layout, p_sizePolicy, {0, 0})));

               return (_elements.back().get());
       }

       void Layout::removeWidget(spk::SafePointer<spk::Widget> p_widget)
       {
               if (p_widget == nullptr)
               {
                       return ;
               }

		for (auto it = _elements.begin(); it != _elements.end(); ++it)
		{
			if ((*it)->widget() == p_widget)
			{
				_elements.erase(it);
				resizeElements(_elements.size());
				return ;
                       }
               }

               throw std::runtime_error("Widget [" + spk::StringUtils::wstringToString(p_widget->name()) + "] not contained by the layout");
       }

       void Layout::removeLayout(spk::SafePointer<spk::Layout> p_layout)
       {
               if (p_layout == nullptr)
               {
                       return;
               }

               for (auto it = _elements.begin(); it != _elements.end(); ++it)
               {
                       if ((*it)->layout() == p_layout)
                       {
                               _elements.erase(it);
                               resizeElements(_elements.size());
                               return;
                       }
               }

               throw std::runtime_error("Layout not contained by the layout");
       }

       void Layout::removeWidget(spk::SafePointer<Element> p_element)
       {
               if (p_element == nullptr)
               {
                       return;
               }

               if (p_element->isWidget())
               {
                       removeWidget(p_element->widget());
               }
               else if (p_element->isLayout())
               {
                       removeLayout(p_element->layout());
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

       spk::Vector2UInt Layout::minimalSize() const
       {
               return {0u, 0u};
       }

       spk::Vector2UInt Layout::maximalSize() const
       {
               return {std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max()};
       }
}
