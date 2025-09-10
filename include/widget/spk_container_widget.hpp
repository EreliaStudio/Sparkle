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

		void _onGeometryChange() override;

	public:
		ContainerWidget(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent);

		void setContent(spk::SafePointer<spk::Widget> p_content);
		spk::SafePointer<spk::Widget> content();

		void setContentAnchor(const spk::Vector2Int &p_contentAnchor);
		const spk::Vector2Int &contentAnchor() const;

		void setContentSize(const spk::Vector2UInt &p_contentSize);
		const spk::Vector2UInt &contentSize() const;
	};
}