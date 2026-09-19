#include "ui/widget/container_widget.hpp"

#include <stdexcept>
#include <utility>

namespace spk
{
	ContainerWidget::ContainerWidget(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		activate();
	}

	void ContainerWidget::_updateContentGeometry()
	{
		if (_content != nullptr)
		{
			_content->setGeometry({_contentAnchor, _contentSize});
		}
	}

	void ContainerWidget::_onGeometryChange()
	{
		_updateContentGeometry();
	}

	void ContainerWidget::setContent(Widget *content)
	{
		if (content != nullptr && content->parent() != this)
		{
			throw std::invalid_argument("ContainerWidget content must already be a child of the container");
		}
		if (_content == content)
		{
			return;
		}

		_contentParentEditionContract.resign();
		_content = content;
		if (_content != nullptr)
		{
			Widget *observedContent = _content;
			_contentParentEditionContract = _content->subscribeToParentEdition([this, observedContent](const Widget *parent) {
				if (_content == observedContent && parent != this)
				{
					_content = nullptr;
					_contentParentEditionContract.resign();
				}
			});
		}
		_updateContentGeometry();
	}

	void ContainerWidget::setContentAnchor(const Vector2Int &anchor)
	{
		if (_contentAnchor == anchor)
		{
			return;
		}
		_contentAnchor = anchor;
		_updateContentGeometry();
	}

	void ContainerWidget::setContentSize(const Vector2UInt &size)
	{
		if (_contentSize == size)
		{
			return;
		}
		_contentSize = size;
		_updateContentGeometry();
	}

	Widget *ContainerWidget::content() noexcept
	{
		return _content;
	}

	const Widget *ContainerWidget::content() const noexcept
	{
		return _content;
	}

	const Vector2Int &ContainerWidget::contentAnchor() const noexcept
	{
		return _contentAnchor;
	}

	const Vector2UInt &ContainerWidget::contentSize() const noexcept
	{
		return _contentSize;
	}
}
