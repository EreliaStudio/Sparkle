#pragma once

#include "widget/spk_widget.hpp"

namespace spk
{
	class ContainerWidget : public spk::Widget
	{
	private:
		spk::SafePointer<spk::Widget> _content;

		spk::Vector2Int _contentAnchor;
		spk::Vector2UInt _contentSize;

		void _onGeometryChange()
		{
			if (_content != nullptr)
			{
				_content->setGeometry(_contentAnchor, _contentSize);
			}
		}

	public:
		ContainerWidget(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::Widget(p_name, p_parent),
			_content(nullptr)
		{
		}

		void setContent(spk::SafePointer<spk::Widget> p_content)
		{
			_content = p_content;
			requireGeometryUpdate();
		}

		spk::SafePointer<spk::Widget> content()
		{
			return (_content);
		}

		void setContentAnchor(const spk::Vector2Int &p_contentAnchor)
		{
			_contentAnchor = p_contentAnchor;
			requireGeometryUpdate();
		}

		const spk::Vector2Int &contentAnchor() const
		{
			return (_contentAnchor);
		}

		void setContentSize(const spk::Vector2UInt &p_contentSize)
		{
			_contentSize = p_contentSize;
			requireGeometryUpdate();
		}

		const spk::Vector2UInt &contentSize() const
		{
			return (_contentSize);
		}
	};
}