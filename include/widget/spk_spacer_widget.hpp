#pragma once

#include "widget/spk_widget.hpp"
#include "structure/math/spk_vector2.hpp"

namespace spk
{
    class SpacerWidget : public Widget
    {
    private:
        spk::Vector2UInt _minimalSize = {0, 0};
        spk::Vector2UInt _maximalSize = {
				std::numeric_limits<uint32_t>::max(),
				std::numeric_limits<uint32_t>::max()
			};

    public:
        SpacerWidget(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
            Widget(p_name, p_parent)
        {
        }
		
        ~SpacerWidget()
        {
        }

		void setMinimalSize(const spk::Vector2UInt &p_size)
		{
			_minimalSize = p_size;
		}

		void setMaximalSize(const spk::Vector2UInt &p_size)
		{
			_maximalSize = p_size;
		}

        spk::Vector2UInt minimalSize() const override
        {
            return _minimalSize;
        }

        spk::Vector2UInt maximalSize() const override
        {
            return _maximalSize;
        }
    };
}