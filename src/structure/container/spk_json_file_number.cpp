#include "structure/container/spk_JSON_file.hpp"

#include <fstream>
#include <algorithm>
#include <cwctype>

#include "utils/spk_string_utils.hpp"

namespace spk
{
	namespace JSON
	{
		static bool _isNumberMalformatted(bool p_isNegative, const size_t& p_decimalPos,
			const size_t& p_exponentPos, const std::wstring& p_unitSubString)
		{
			return (p_exponentPos == p_unitSubString.size() - 1 ||
				(p_isNegative == true && p_unitSubString.size() == 1) ||
				(p_isNegative == true && p_unitSubString.size() > 1 && ::isdigit(p_unitSubString[1]) == false) ||
				p_decimalPos == p_unitSubString.size() - 1 ||
				(p_decimalPos != std::wstring::npos && ::isdigit(p_unitSubString[p_decimalPos + 1]) == false) ||
				(p_decimalPos != std::wstring::npos && p_exponentPos != std::wstring::npos && p_decimalPos > p_exponentPos) ||
				(p_unitSubString[p_isNegative] == L'0' && p_unitSubString.size() > static_cast<size_t>(p_isNegative) + 1u &&
					std::wstring(L".eE").find(p_unitSubString[p_isNegative + 1]) == std::wstring::npos) ||
				std::count(p_unitSubString.begin(), p_unitSubString.end(), L'.') > 1);
		}

		static long _extractExponent(const std::wstring& p_exponentSubstring)
		{
			bool isExponentSigned(p_exponentSubstring[0] == L'-' || p_exponentSubstring[0] == L'+');
			long result(0);

			if (p_exponentSubstring.find_first_not_of(L"0123456789", isExponentSigned) != std::wstring::npos)
				throw std::runtime_error("Invalid numbers JSON exponent value: " + spk::StringUtils::wstringToString(p_exponentSubstring));
			try
			{
				result = std::stol(p_exponentSubstring);
			}
			catch (const std::exception&)
			{
				throw std::runtime_error("Invalid numbers JSON value: " + spk::StringUtils::wstringToString(p_exponentSubstring) + " too big (number overflow)");
			}
			return (result);
		}

		template <typename NumericType>
		static unsigned short numberLength(NumericType p_number, unsigned short base = 10)
		{
			unsigned short result(1);

			if (base == 0)
				throw std::runtime_error("numberLength: Base cannot be 0");
			if (p_number < 0)
			{
				++result;
				p_number = -p_number;
			}
			while (p_number >= base)
			{
				p_number /= base;
				++result;
			}
			return (result);
		}

		static bool _resultWillBeDouble(const size_t& p_decimalPos, bool p_hasExponent, const size_t& p_exponentPos,
			bool p_isNegative, const long& p_exponent)
		{
			if (p_decimalPos == std::wstring::npos && p_exponentPos == false)
				return false;
			return (p_decimalPos != std::wstring::npos || (p_hasExponent == true &&
				p_exponent > numberLength(std::numeric_limits<long>::max()) ||
				p_exponentPos - p_isNegative > numberLength(std::numeric_limits<long>::max()) ||
				p_exponentPos - p_isNegative + p_exponent <= 0));
		}

		static long _safePowerOfTen(const long& p_number, const long& p_exponent, const std::wstring& p_unitSubString)
		{
			long result(0);
			errno = 0;
			std::feclearexcept(FE_ALL_EXCEPT);

			result = static_cast<long>(p_number * std::pow(10, p_exponent));

			if (errno == EDOM || errno == ERANGE || std::fetestexcept(FE_ALL_EXCEPT ^ FE_INEXACT) != 0)
				throw std::runtime_error("Invalid numbers JSON value: " + spk::StringUtils::wstringToString(p_unitSubString) + " too big (power overflow)");

			return (result);
		}

		void _loadUnitNumbers(spk::JSON::Object& p_objectToFill, const std::wstring& p_unitSubString)
		{
			std::variant<long, double> result;

			bool isNegative(p_unitSubString[0] == L'-');

			size_t exponentPos(p_unitSubString.find_first_of(L"eE"));
			long exponent(0);

			size_t decimalPos(p_unitSubString.find_last_of(L"."));

			if (_isNumberMalformatted(isNegative, decimalPos, exponentPos, p_unitSubString) == true)
				throw std::runtime_error("Malformatted JSON number: " + spk::StringUtils::wstringToString(p_unitSubString));

			exponentPos = (exponentPos == std::wstring::npos) ? p_unitSubString.size() : exponentPos;

			if (p_unitSubString.substr(isNegative, exponentPos - isNegative).find_first_not_of(L".0123456789") != std::wstring::npos)
				throw std::runtime_error("JSON number value is not Numerical: " + spk::StringUtils::wstringToString(p_unitSubString));

			if (exponentPos != p_unitSubString.size())
				exponent = _extractExponent(p_unitSubString.substr(exponentPos + 1));

			try
			{
				(_resultWillBeDouble(decimalPos, p_unitSubString.size() != exponentPos,
					exponentPos, isNegative, exponent) == true) ?
					result = std::stod(p_unitSubString) : result = std::stol(p_unitSubString);
			}
			catch (const std::exception&)
			{
				throw std::runtime_error("Invalid numbers JSON value: " + spk::StringUtils::wstringToString(p_unitSubString) + " too big (number overflow)");
			}

			if (exponentPos != p_unitSubString.size() && std::holds_alternative<long>(result) == true)
				result = _safePowerOfTen(std::get<long>(result), exponent, p_unitSubString);

			if (std::holds_alternative<double>(result) == true)
				p_objectToFill.set(std::get<double>(result));
			else
				p_objectToFill.set(std::get<long>(result));
		}
	}
}