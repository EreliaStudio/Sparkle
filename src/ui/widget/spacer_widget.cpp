#include "ui/widget/spacer_widget.hpp"

#include <limits>
#include <utility>

namespace spk
{
	SpacerWidget::SpacerWidget(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		setMaximalSize({std::numeric_limits<float>::max(), std::numeric_limits<float>::max()});
		activate();
	}
}
