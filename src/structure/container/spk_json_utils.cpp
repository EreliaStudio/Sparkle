#include "structure/container/spk_json_utils.hpp"

#include "structure/container/spk_json_file.hpp"
#include "structure/system/spk_exception.hpp"
#include "utils/spk_string_utils.hpp"

#include <algorithm>
#include <cerrno>
#include <cfenv>
#include <cmath>
#include <cwctype>
#include <limits>
#include <string>
#include <variant>

namespace spk::JSON::utils
{
namespace
{
std::wstring handleEscapeSequence(const std::wstring &p_fileContent)
{
std::wstring result;

for (size_t i = 0; i < p_fileContent.size(); ++i)
{
if (p_fileContent[i] == L'\\')
{
switch (p_fileContent[i + 1])
{
case L'"':
result += L'"';
break;
case L'\\':
result += L"\\";
break;
case L'/':
result += L'/';
break;
case L'b':
result += L'\b';
break;
case L'f':
result += L'\f';
break;
case L'n':
result += L'\n';
break;
case L'r':
result += L'\r';
break;
case L't':
result += L'\t';
break;
case L'u':
try
{
result +=
std::to_wstring(std::stoi(p_fileContent.substr(i + 2, 4), nullptr, 16));
}
catch (const std::exception &)
{
GENERATE_ERROR("Invalid Unicode escape : <" +
spk::StringUtils::wstringToString(p_fileContent).substr(i, 6) + '>' );
}
i += 4;
break;
default:
GENERATE_ERROR("Invalid escape sequence: <" +
spk::StringUtils::wstringToString(p_fileContent).substr(i, 2) + '>' );
}
++i;
}
else
{
result += p_fileContent[i];
}
}
return (result);
}

std::wstring getAttributeName(const std::wstring &p_content, size_t &p_index)
{
if (p_content[p_index] != L'"')
{
GENERATE_ERROR("Invalid attribute name [" +
spk::StringUtils::wstringToString(p_content).substr(p_index, 2) + "]");
}

++p_index;
size_t start = p_index;

for (; p_index < p_content.size() && p_content[p_index] != L'"'; p_index++)
{
if (p_content[p_index] == L'\\' && p_content[p_index + 1] != L'"')
{
++p_index;
}
}
++p_index;
if (p_content[p_index] != L':')
{
GENERATE_ERROR("Invalid attribute name [" +
spk::StringUtils::wstringToString(p_content).substr(start, p_index - start) + "]");
}
++p_index;
return (handleEscapeSequence(p_content.substr(start, p_index - start - 2)));
}

std::wstring extractUnitSubstring(const std::wstring &p_content, size_t &p_index)
{
size_t oldIndex = p_index;
bool isAString = false;

if (p_content[p_index] == L'"')
{
isAString = true;
++p_index;
}
for (; p_index < p_content.size() &&
((p_content[p_index] != L',' && p_content[p_index] != L'}' &&
  p_content[p_index] != L']') || isAString == true);
 p_index++)
{
if (isAString == true &&
(p_index >= 1 && p_content[p_index] == L'"' &&
 p_content[p_index - 1] != L'\\') ||
(p_index >= 2 && p_content[p_index] == L'"' &&
 p_content[p_index - 1] == L'\\' &&
 p_content[p_index - 2] == L'\\'))
{
break;
}
if (isAString == false &&
(p_content[p_index] == L'"' || p_content[p_index] == L'[' ||
 p_content[p_index] == L'{' || p_content[p_index] == L','))
{
GENERATE_ERROR("Invalid JSON unit [" +
spk::StringUtils::wstringToString(p_content).substr(oldIndex, 5) + "]");
}
}
if (isAString == true)
{
if (p_content[p_index] != L'"')
{
GENERATE_ERROR("Invalid JSON unit [" +
spk::StringUtils::wstringToString(p_content).substr(oldIndex, p_index - oldIndex) + "]");
}

++p_index;
}

return (handleEscapeSequence(p_content.substr(oldIndex, p_index - oldIndex)));
}

bool isNumberMalformatted(bool p_isNegative, const size_t &p_decimalPos,
const size_t &p_exponentPos, const std::wstring &p_unitSubString)
{
return (p_exponentPos == p_unitSubString.size() - 1 ||
(p_isNegative == true && p_unitSubString.size() == 1) ||
(p_isNegative == true && p_unitSubString.size() > 1 &&
 ::isdigit(p_unitSubString[1]) == 0) ||
p_decimalPos == p_unitSubString.size() - 1 ||
(p_decimalPos != std::wstring::npos &&
 ::isdigit(p_unitSubString[p_decimalPos + 1]) == 0) ||
(p_decimalPos != std::wstring::npos && p_exponentPos != std::wstring::npos &&
 p_decimalPos > p_exponentPos) ||
(p_unitSubString[static_cast<size_t>(p_isNegative)] == L'0' &&
 p_unitSubString.size() > static_cast<size_t>(p_isNegative) + 1u &&
 std::wstring(L".eE").find(
 p_unitSubString[static_cast<size_t>(p_isNegative) + 1]) ==
 std::wstring::npos) ||
std::count(p_unitSubString.begin(), p_unitSubString.end(), L'.') > 1);
}

int32_t extractExponent(const std::wstring &p_exponentSubstring)
{
bool isExponentSigned(p_exponentSubstring[0] == L'-' ||
   p_exponentSubstring[0] == L'+');
int32_t result(0);

if (p_exponentSubstring.find_first_not_of(L"0123456789", isExponentSigned) !=
std::wstring::npos)
{
GENERATE_ERROR("Invalid numbers JSON exponent value: " +
spk::StringUtils::wstringToString(p_exponentSubstring));
}
try
{
result = std::stol(p_exponentSubstring);
}
catch (const std::exception &)
{
GENERATE_ERROR("Invalid numbers JSON value: " +
spk::StringUtils::wstringToString(p_exponentSubstring) +
" too big (number overflow)");
}
return (result);
}

template <typename NumericType>
unsigned short numberLength(NumericType p_number, unsigned short p_base = 10)
{
unsigned short result(1);

if (p_base == 0)
{
GENERATE_ERROR("numberLength: Base cannot be 0");
}
if (p_number < 0)
{
++result;
p_number = -p_number;
}
while (p_number >= p_base)
{
p_number /= p_base;
++result;
}
return (result);
}

bool resultWillBeDouble(const size_t &p_decimalPos, bool p_hasExponent,
const size_t &p_exponentPos, bool p_isNegative,
const int32_t &p_exponent)
{
if (p_decimalPos == std::wstring::npos && p_hasExponent == false)
{
return (false);
}
return (
p_decimalPos != std::wstring::npos ||
(p_hasExponent == true &&
  p_exponent > numberLength(std::numeric_limits<int32_t>::max()) ||
  p_exponentPos - p_isNegative >
  numberLength(std::numeric_limits<int32_t>::max()) ||
  p_exponentPos - p_isNegative + p_exponent <= 0));
}

int32_t safePowerOfTen(const int32_t &p_number, const int32_t &p_exponent,
 const std::wstring &p_unitSubString)
{
int32_t result(0);
errno = 0;
std::feclearexcept(FE_ALL_EXCEPT);

result = static_cast<int32_t>(p_number * std::pow(10, p_exponent));

if (errno == EDOM || errno == ERANGE ||
std::fetestexcept(FE_ALL_EXCEPT ^ FE_INEXACT) != 0)
{
GENERATE_ERROR("Invalid numbers JSON value: " +
spk::StringUtils::wstringToString(p_unitSubString) +
" too big (power overflow)");
}

return (result);
}

void loadUnitNumbers(spk::JSON::Object &p_objectToFill,
   const std::wstring &p_unitSubString)
{
std::variant<int32_t, double> result;

bool isNegative(p_unitSubString[0] == L'-');

size_t exponentPos(p_unitSubString.find_first_of(L"eE"));
int32_t exponent(0);

size_t decimalPos(p_unitSubString.find_last_of(L'.'));

if (isNumberMalformatted(isNegative, decimalPos, exponentPos,
     p_unitSubString) == true)
{
GENERATE_ERROR("Malformatted JSON number: " +
spk::StringUtils::wstringToString(p_unitSubString));
}

exponentPos = (exponentPos == std::wstring::npos) ? p_unitSubString.size()
: exponentPos;

if (p_unitSubString.substr(isNegative,
 exponentPos - static_cast<int>(isNegative))
.find_first_not_of(L".0123456789") != std::wstring::npos)
{
GENERATE_ERROR("JSON number value is not Numerical: " +
spk::StringUtils::wstringToString(p_unitSubString));
}

if (exponentPos != p_unitSubString.size())
{
exponent = extractExponent(p_unitSubString.substr(exponentPos + 1));
}

try
{
(resultWillBeDouble(decimalPos, p_unitSubString.size() != exponentPos,
exponentPos, isNegative, exponent) == true)
? result = std::stod(p_unitSubString)
: result = std::stol(p_unitSubString);
}
catch (const std::exception &)
{
GENERATE_ERROR("Invalid numbers JSON value: " +
spk::StringUtils::wstringToString(p_unitSubString) +
" too big (number overflow)");
}

if (exponentPos != p_unitSubString.size() &&
std::holds_alternative<int32_t>(result) == true)
{
result = safePowerOfTen(std::get<int32_t>(result), exponent,
 p_unitSubString);
}

if (std::holds_alternative<double>(result) == true)
{
p_objectToFill.set(std::get<double>(result));
}
else
{
p_objectToFill.set(std::get<int32_t>(result));
}
}

void loadUnitString(spk::JSON::Object &p_objectToFill,
 const std::wstring &p_unitSubString)
{
p_objectToFill.set(p_unitSubString.substr(1, p_unitSubString.size() - 2));
}

void loadUnitBoolean(spk::JSON::Object &p_objectToFill,
  const std::wstring &p_unitSubString)
{
if (p_unitSubString == L"true")
{
p_objectToFill.set(true);
}
else if (p_unitSubString == L"false")
{
p_objectToFill.set(false);
}
else
{
GENERATE_ERROR("Invalid boolean JSON value: " +
spk::StringUtils::wstringToString(p_unitSubString));
}
}

void loadUnitNull(spk::JSON::Object &p_objectToFill,
 const std::wstring &p_unitSubString)
{
if (p_unitSubString == L"null")
{
p_objectToFill.set(nullptr);
}
else
{
GENERATE_ERROR("Invalid null JSON value: " +
spk::StringUtils::wstringToString(p_unitSubString));
}
}

void loadUnit(spk::JSON::Object &p_objectToFill,
const std::wstring &p_content, size_t &p_index)
{
std::wstring substring = extractUnitSubstring(p_content, p_index);

switch (substring[0])
{
case '"':
loadUnitString(p_objectToFill, substring);
break;
case '0':
case '1':
case '2':
case '3':
case '4':
case '5':
case '6':
case '7':
case '8':
case '9':
case '-':
loadUnitNumbers(p_objectToFill, substring);
break;
case 't':
case 'f':
loadUnitBoolean(p_objectToFill, substring);
break;
case 'n':
loadUnitNull(p_objectToFill, substring);
break;
default:
GENERATE_ERROR("Invalid JSON value: " +
spk::StringUtils::wstringToString(substring));
break;
}
}

void loadObject(spk::JSON::Object &p_objectToFill,
const std::wstring &p_content, size_t &p_index)
{
p_objectToFill.setAsObject();

if (p_content[p_index] != '{')
{
GENERATE_ERROR("Invalid JSON object (missing '{')");
}
++p_index;
for (; p_index < p_content.size() && p_content[p_index] != '}';)
{
std::wstring attributeName = getAttributeName(p_content, p_index);

spk::JSON::Object &newObject = p_objectToFill.addAttribute(attributeName);

loadContent(newObject, p_content, p_index);

if (p_content[p_index] == ',')
{
++p_index;
}
else if (p_content[p_index] != '}')
{
GENERATE_ERROR("Invalid JSON object (missing ',' or '}')");
}
}
if (p_content[p_index] != '}')
{
GENERATE_ERROR("Invalid JSON object (missing '}')");
}
++p_index;
}

void loadArray(spk::JSON::Object &p_objectToFill,
const std::wstring &p_content, size_t &p_index)
{
p_objectToFill.setAsArray();
if (p_content[p_index] != '[')
{
GENERATE_ERROR("Invalid JSON array (missing '[')");
}
++p_index;
for (; p_index < p_content.size() && p_content[p_index] != ']';)
{
spk::JSON::Object newObject(L"[" +
 std::to_wstring(p_objectToFill.size()) + L"]");

loadContent(newObject, p_content, p_index);

p_objectToFill.push_back(newObject);

if (p_content[p_index] == ',')
{
++p_index;
}
else if (p_content[p_index] != ']')
{
GENERATE_ERROR("Invalid JSON array (missing ',' or ']')");
}
}
if (p_content[p_index] != ']')
{
GENERATE_ERROR("Invalid JSON array (missing ']')");
}
++p_index;
}
} // namespace

std::wstring applyGrammar(const std::wstring &p_fileContent)
{
std::wstring result;
bool isLiteral(false);

for (size_t i(0); i < p_fileContent.size(); ++i)
{
wchar_t c = p_fileContent[i];
wchar_t nextChar = p_fileContent[i + 1];

switch (c)
{
case L'\\':
if (std::wstring(L"\"\\/bfnrtu").find(nextChar) == std::wstring::npos)
{
GENERATE_ERROR("Invalid escape sequence at line " +
   std::to_string(1 +
std::count(p_fileContent.begin(),
   p_fileContent.begin() + i,
   L'\n')) +
   " column " +
   std::to_string(1 +
 i - p_fileContent.rfind('\n', i)) +
   ".");
}
if (nextChar == L'\\')
{
result += L"\\";
++i;
}
result += c;
break;
case L'\"':
if (i == 0 || (i >= 1 && p_fileContent[i - 1] != L'\\') ||
(i >= 2 && p_fileContent[i - 1] == L'\\' &&
 p_fileContent[i - 2] == L'\\'))
{
isLiteral = !isLiteral;
}
result += c;
break;
case L' ':
case L'\t':
case L'\n':
case L'\r':
if (isLiteral == true && c != L'\n' && c != L'\r')
{
result += c;
}
else if (isLiteral == true && (c == L'\n' || c == L'\r'))
{
GENERATE_ERROR("Unexpected end of string " +
   std::to_string(1 +
std::count(p_fileContent.begin(),
   p_fileContent.begin() + i,
   '\n')) +
   " column " +
   std::to_string(i - p_fileContent.rfind('\n', i - 1)) +
   ".");
}
break;
default:
result += c;
}
}
return (result);
}

void loadContent(spk::JSON::Object &p_objectToFill,
const std::wstring &p_content, size_t &p_index)
{
if (p_index >= p_content.size())
{
GENERATE_ERROR("Unexpected end of JSON content");
}

switch (p_content[p_index])
{
case '\"':
case 'f':
case 't':
case 'n':
case '0':
case '1':
case '2':
case '3':
case '4':
case '5':
case '6':
case '7':
case '8':
case '9':
case '-':
loadUnit(p_objectToFill, p_content, p_index);
break;
case '{':
loadObject(p_objectToFill, p_content, p_index);
break;
case '[':
loadArray(p_objectToFill, p_content, p_index);
break;
default:
GENERATE_ERROR(
"Unexpected data type in JSON: [" +
spk::StringUtils::wstringToString(p_content).substr(p_index, 1) +
"] (Char value: " +
std::to_string(static_cast<int>(p_content[p_index])) + ")");
}
}
} // namespace spk::JSON::utils

