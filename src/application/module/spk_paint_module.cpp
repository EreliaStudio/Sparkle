#include "application/module/spk_paint_module.hpp"

#include "structure/graphics/spk_window.hpp"

namespace spk
{
	void PaintModule::_treatEvent(spk::PaintEvent &&p_event)
	{
		switch (p_event.type)
		{
			case spk::PaintEvent::Type::Resize:
			{
				p_event.window->resize(p_event.geometry.size);
				break;
			}
			case spk::PaintEvent::Type::Paint:
			{
				p_event.window->clear();
				p_event.window->widget()->onPaintEvent(p_event);
				p_event.window->swap();
				break;
			}
		}
	}

	spk::PaintEvent PaintModule::_convertEventToEventType(spk::Event &&p_event)
	{
		return (p_event.paintEvent);
	}

	PaintModule::PaintModule()
	{
	}
}