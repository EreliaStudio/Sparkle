#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk
{
	Pipeline::Constant::Constant(const GLuint& p_program, const Layout& p_layout) :
		Pipeline::UniformObject(p_program, p_layout)
	{

	}
}