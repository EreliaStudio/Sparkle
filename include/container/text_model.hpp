#pragma once

#include <string>

#include "container/data_model.hpp"

namespace spk
{
	class TextModel : public DataModel<std::string>
	{
	public:
		using DataModel<std::string>::DataModel;

		class Delegate;

		class View;
	};

}
