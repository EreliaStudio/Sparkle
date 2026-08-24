#pragma once

#include <string>

#include "ui/widget.hpp"

namespace spk
{
	class ContainerWidget : public Widget
	{
	private:
		Widget *_content = nullptr;
		Vector2Int _contentAnchor{0, 0};
		Vector2UInt _contentSize{0, 0};
		InherenceTrait<Widget, WidgetChildComparator>::OnParentEditionContract _contentParentEditionContract;

		void _updateContentGeometry();
		void _onGeometryChange() override;

	public:
		explicit ContainerWidget(std::string name, Widget *parent = nullptr);

		void setContent(Widget *content);
		void setContentAnchor(const Vector2Int &anchor);
		void setContentSize(const Vector2UInt &size);

		[[nodiscard]] Widget *content() noexcept;
		[[nodiscard]] const Widget *content() const noexcept;
		[[nodiscard]] const Vector2Int &contentAnchor() const noexcept;
		[[nodiscard]] const Vector2UInt &contentSize() const noexcept;
	};
}
