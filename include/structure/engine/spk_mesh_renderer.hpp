#pragma once

#include "structure/engine/spk_component.hpp"

namespace spk
{
	class MeshRenderer : public spk::Component
	{
	private:

	public:	
		MeshRenderer(const std::wstring& p_name) :
			spk::Component(p_name)
		{
			
		}
	};
}