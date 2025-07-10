#include <sparkle.hpp>

#include <fstream>

namespace spk::lumina
{
	class Shader
	{
	public:
		struct Layout
		{
			size_t bindingPoint{};
			std::wstring name;
			spk::OpenGL::LayoutBufferObject::Attribute::Type type{};
		};

		using ExceptionID = size_t;
		struct ExceptionDescriptor
		{
			std::wstring format;
			std::vector<spk::JSON::Object> parameters;
		};

	private:
		std::vector<Layout> _inputLayoutDescriptors;
		std::vector<Layout> _frameBufferLayoutDescriptors;
		std::unordered_map<ExceptionID, ExceptionDescriptor> _exceptionDescriptors;
		std::vector<spk::JSON::Object> _constantDescriptors;
		std::vector<spk::JSON::Object> _attributeDescriptors;
		std::unordered_map<std::wstring, std::wstring> _textureDescriptors;
		std::wstring _vertexShaderCode;
		std::wstring _fragmentShaderCode;

		static std::wstring _extractSection(const std::wstring &p_src, const std::wstring &p_sectionName)
		{
			const std::wstring marker = L"## " + p_sectionName + L" ##";
			size_t header = p_src.find(marker);
			if (header == std::wstring::npos)
			{
				return L"";
			}

			size_t contentStart = p_src.find(L'\n', header);
			if (contentStart == std::wstring::npos)
			{
				return L"";
			}
			++contentStart;

			size_t next = p_src.find(L"## ", contentStart);
			while (next != std::wstring::npos && next > 0 && p_src[next - 1] != L'\n')
			{
				next = p_src.find(L"## ", next + 1);
			}

			const size_t len = (next == std::wstring::npos) ? (p_src.size() - contentStart) : (next - contentStart);
			std::wstring out = p_src.substr(contentStart, len);
			while (!out.empty() && (out.back() == L'\n' || out.back() == L'\r'))
			{
				out.pop_back();
			}
			return out;
		}

		static spk::OpenGL::LayoutBufferObject::Attribute::Type _toAttrType(const std::wstring &p_input)
		{
			using AT = spk::OpenGL::LayoutBufferObject::Attribute::Type;
			if (p_input == L"Float")
			{
				return AT::Float;
			}
			if (p_input == L"Vector2")
			{
				return AT::Vector2;
			}
			if (p_input == L"Vector3")
			{
				return AT::Vector3;
			}
			if (p_input == L"Vector4")
			{
				return AT::Vector4;
			}
			if (p_input == L"Int")
			{
				return AT::Int;
			}
			if (p_input == L"Vector2Int")
			{
				return AT::Vector2Int;
			}
			if (p_input == L"Vector3Int")
			{
				return AT::Vector3Int;
			}
			if (p_input == L"Vector4Int")
			{
				return AT::Vector4Int;
			}
			if (p_input == L"UInt")
			{
				return AT::UInt;
			}
			if (p_input == L"Vector2UInt")
			{
				return AT::Vector2UInt;
			}
			if (p_input == L"Vector3UInt")
			{
				return AT::Vector3UInt;
			}
			if (p_input == L"Vector4UInt")
			{
				return AT::Vector4UInt;
			}
			if (p_input == L"Matrix2x2")
			{
				return AT::Matrix2x2;
			}
			if (p_input == L"Matrix3x3")
			{
				return AT::Matrix3x3;
			}
			if (p_input == L"Matrix4x4")
			{
				return AT::Matrix4x4;
			}
			GENERATE_ERROR("Unknown attribute type \"" + spk::StringUtils::wstringToString(p_input) + "\"");
		}

		static std::vector<Layout> _parseLayoutLines(const std::wstring &p_section)
		{
			std::vector<Layout> list;
			std::wistringstream ss(p_section);
			std::wstring line;
			while (std::getline(ss, line))
			{
				if (line.empty())
				{
					continue;
				}
				std::wistringstream ls(line);
				size_t loc{};
				std::wstring type, name;
				ls >> loc >> type >> name;
				if (type.empty() || name.empty())
				{
					continue;
				}
				list.push_back(Layout{loc, name, _toAttrType(type)});
			}
			return list;
		}

		void _parseExceptionSection(const std::wstring &p_section)
		{
			if (p_section.empty())
			{
				return;
			}

			spk::JSON::Object root = spk::JSON::Object::fromString(p_section);
			if (!root.isObject() || !root.contains(L"Exceptions"))
			{
				GENERATE_ERROR("EXCEPTION DEFINITION must contain \"Exceptions\" array.");
			}

			const spk::JSON::Object &arr = root[L"Exceptions"];
			if (!arr.isArray())
			{
				GENERATE_ERROR("\"Exceptions\" is not an array.");
			}

			for (size_t i = 0; i < arr.size(); ++i)
			{
				const spk::JSON::Object &ex = arr[i];
				ExceptionDescriptor desc;

				if (!ex.contains(L"Format"))
				{
					GENERATE_ERROR("Each exception needs a \"Format\" field.");
				}
				desc.format = ex[L"Format"].as<std::wstring>();

				if (ex.contains(L"Parameter"))
				{
					const spk::JSON::Object &params = ex[L"Parameter"];
					if (!params.isArray())
					{
						GENERATE_ERROR("\"Parameter\" must be an array.");
					}
					for (size_t p = 0; p < params.size(); ++p)
					{
						desc.parameters.emplace_back(params[p]);
					}
				}
				_exceptionDescriptors[i] = std::move(desc);
			}
		}

		static std::vector<spk::JSON::Object> _splitIntoJsonObjects(const std::wstring &p_section)
		{
			std::vector<spk::JSON::Object> objs;
			size_t pos = 0, n = p_section.size();

			auto skipWhitespaces = [&](size_t &p_pos)
			{
				while (p_pos < n && iswspace(p_section[p_pos]))
				{
					++p_pos;
				}
			};

			while (true)
			{
				skipWhitespaces(pos);
				if (pos >= n)
				{
					break;
				}
				if (p_section[pos] != L'{' && p_section[pos] != L'[')
				{
					GENERATE_ERROR("Malformed JSON block in section.");
				}

				const wchar_t open = p_section[pos];
				const wchar_t close = (open == L'{') ? L'}' : L']';
				int depth = 0;
				bool inStr = false;
				size_t start = pos;

				for (; pos < n; ++pos)
				{
					wchar_t c = p_section[pos];
					if (!inStr)
					{
						if (c == L'\"')
						{
							inStr = true;
						}
						else if (c == open)
						{
							++depth;
						}
						else if (c == close)
						{
							--depth;
						}
						if (depth == 0)
						{
							++pos;
							break;
						}
					}
					else if (c == L'\\')
					{
						++pos;
					}
					else if (c == L'\"')
					{
						inStr = false;
					}
				}
				objs.emplace_back(spk::JSON::Object::fromString(p_section.substr(start, pos - start)));
				skipWhitespaces(pos);
			}
			return objs;
		}

		void _parseTextureSection(const std::wstring &p_section)
		{
			std::wistringstream ss(p_section);
			std::wstring line;
			while (std::getline(ss, line))
			{
				if (line.empty())
				{
					continue;
				}
				std::wistringstream ls(line);
				std::wstring user, glsl;
				ls >> user >> glsl;
				if (!user.empty() && !glsl.empty())
				{
					_textureDescriptors[user] = glsl;
				}
			}
		}

		explicit Shader(const std::wstring &p_source)
		{
			_inputLayoutDescriptors = _parseLayoutLines(_extractSection(p_source, L"LAYOUTS DEFINITION"));

			_frameBufferLayoutDescriptors = _parseLayoutLines(_extractSection(p_source, L"FRAMEBUFFER DEFINITION"));

			_parseExceptionSection(_extractSection(p_source, L"EXCEPTION DEFINITION"));

			for (auto &obj : _splitIntoJsonObjects(_extractSection(p_source, L"CONSTANTS DEFINITION")))
			{
				_constantDescriptors.push_back(obj);
			}

			for (auto &obj : _splitIntoJsonObjects(_extractSection(p_source, L"ATTRIBUTES DEFINITION")))
			{
				_attributeDescriptors.push_back(obj);
			}

			_parseTextureSection(_extractSection(p_source, L"TEXTURES DEFINITION"));

			_vertexShaderCode = _extractSection(p_source, L"VERTEX SHADER CODE");

			_fragmentShaderCode = _extractSection(p_source, L"FRAGMENT SHADER CODE");
		}

	public:
		Shader() = default;

		static Shader fromSource(const std::wstring &p_src)
		{
			return Shader(p_src);
		}
		static Shader fromFile(const std::wstring &p_path)
		{
			return fromSource(spk::FileUtils::readFileAsWString(p_path));
		}
		
		friend std::wostream& operator << (std::wostream& p_os, const Shader& p_shader)
		{
			auto header = [&](const std::wstring &p_title) { p_os << L"## " << p_title << L" ##\n"; };

			header(L"LAYOUTS DEFINITION");
			for (const auto &l : p_shader._inputLayoutDescriptors)
			{
				p_os << l.bindingPoint << L' ' << spk::OpenGL::to_wstring(l.type) << L' ' << l.name << L'\n';
			}
			p_os << L'\n';

			header(L"FRAMEBUFFER DEFINITION");
			for (const auto &l : p_shader._frameBufferLayoutDescriptors)
			{
				p_os << l.bindingPoint << L' ' << spk::OpenGL::to_wstring(l.type) << L' ' << l.name << L'\n';
			}
			p_os << L'\n';

			header(L"EXCEPTION DEFINITION");
			{
				spk::JSON::Object root(L"root");
				root.setAsObject();
				auto &arr = root.addAttribute(L"Exceptions");
				arr.setAsArray();
				arr.resize(p_shader._exceptionDescriptors.size());

				for (const auto &[id, ex] : p_shader._exceptionDescriptors)
				{
					auto &obj = arr[id];
					obj.setAsObject();
					obj.addAttribute(L"Format") = ex.format;

					auto &paramsArr = obj.addAttribute(L"Parameter");
					paramsArr.setAsArray();
					paramsArr.resize(ex.parameters.size());
					for (size_t i = 0; i < ex.parameters.size(); ++i)
					{
						paramsArr[i] = ex.parameters[i];
					}
				}
				p_os << root << L'\n';
			}
			p_os << L'\n';

			header(L"CONSTANTS DEFINITION");
			for (const auto &c : p_shader._constantDescriptors)
			{
				p_os << c << L'\n';
			}
			p_os << L'\n';

			header(L"ATTRIBUTES DEFINITION");
			for (const auto &a : p_shader._attributeDescriptors)
			{
				p_os << a << L'\n';
			}
			p_os << L'\n';

			header(L"TEXTURES DEFINITION");
			for (const auto &[user, glsl] : p_shader._textureDescriptors)
			{
				p_os << user << L' ' << glsl << L'\n';
			}
			p_os << L'\n';

			header(L"VERTEX SHADER CODE");
			p_os << p_shader._vertexShaderCode << L'\n' << L'\n';

			header(L"FRAGMENT SHADER CODE");
			p_os << p_shader._fragmentShaderCode << L'\n';
			return (p_os);
		}

		friend std::ostream& operator << (std::ostream& p_os, const Shader& p_shader)
		{
			std::wstringstream wos;
			wos << p_shader;
			p_os << spk::StringUtils::wstringToString(wos.str());
			return (p_os);
		}
	};

	class Compiler
	{
	private:

	public:
		static Shader compileFromSource(const std::wstring &p_source)
		{
			if (p_source.empty())
			{
				GENERATE_ERROR("Shader source code is empty.");
			}

			return Shader::fromSource(p_source);
		}
		static Shader compileFromFile(const std::filesystem::path &p_path)
		{
			return Shader::fromFile(p_path);
		}
	};
}

int main()
{
	std::wstring luminaCode = LR"(
		Input -> VertexPass : Vector2 modelPosition;
		Input -> VertexPass : Color modelColor;

		VertexPass -> FragmentPass : Color fragmentColor;

		VertexPass()
		{
			fragmentColor = modelColor;
			pixelPosition = vec4(modelPosition, 0.0, 1.0);
		}

		FragmentPass()
		{
			if (fragmentColor.a == 0)
			{
				discard;
			}

			pixelColor = fragmentColor;
		}
	)";

	spk::lumina::Shader compiledCode = spk::lumina::Compiler::compileFromSource(luminaCode);

	return (0);
}