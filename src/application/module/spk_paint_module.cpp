#include "application/module/spk_paint_module.hpp"

#include "structure/graphics/spk_window.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	void PaintModule::_treatEvent(spk::PaintEvent &&p_event)
	{
		switch (p_event.type)
		{
			case spk::PaintEvent::Type::Resize:
			{
				try
				{
					p_event.window->resize(p_event.geometry.size);
					p_event.window->requestPaint();
				}
				catch (std::exception &e)
				{
					PROPAGATE_ERROR("PaintModule::_treatEvent over spk::PaintEvent::Type::Resize failed", e);
				}
				break;
			}
			case spk::PaintEvent::Type::Paint:
			{
				try
				{
					p_event.window->clear();
					p_event.window->widget()->onPaintEvent(p_event);
					p_event.window->swap();
					p_event.window->allowPaintRequest();
				}
				catch (std::exception &e)
				{
					PROPAGATE_ERROR("PaintModule::_treatEvent over spk::PaintEvent::Type::Paint failed", e);
				}
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