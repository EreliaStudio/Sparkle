#pragma once

#include <string>

namespace spk
{
	template <typename TType, typename TErrorData = std::wstring>
	struct Expected
	{
		TType value;
		bool hasError;
		TErrorData errorData;

		Expected() :
			hasError(false)
		{
		}
		Expected(const TType &value) :
			value(value),
			hasError(false)
		{
		}

		static Expected<TType> Raise(const TErrorData &errorData)
		{
			Expected<TType> result;

			result.hasError = true;
			result.errorData = errorData;

			return result;
		}
	};
}