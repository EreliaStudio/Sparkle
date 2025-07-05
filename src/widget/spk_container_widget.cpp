#include "widget/spk_container_widget.hpp"

namespace spk
{
	void ContainerWidget::_onGeometryChange()
	{
		if (_content != nullptr)
		{
			_content->setGeometry(_contentAnchor, _contentSize);
		}
	}

	ContainerWidget::ContainerWidget(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_content(nullptr)
	{
	}

	void ContainerWidget::setContent(spk::SafePointer<spk::Widget> p_content)
	{
		_content = p_content;
		requireGeometryUpdate();
	}

	spk::SafePointer<spk::Widget> ContainerWidget::content()
	{
		return (_content);
	}

	void ContainerWidget::setContentAnchor(const spk::Vector2Int &p_contentAnchor)
	{
		_contentAnchor = p_contentAnchor;
		requireGeometryUpdate();
	}

	const spk::Vector2Int &ContainerWidget::contentAnchor() const
	{
		return (_contentAnchor);
	}

	void ContainerWidget::setContentSize(const spk::Vector2UInt &p_contentSize)
	{
		_contentSize = p_contentSize;
		requireGeometryUpdate();
	}

	const spk::Vector2UInt &ContainerWidget::contentSize() const
	{
		return (_contentSize);
	}
}