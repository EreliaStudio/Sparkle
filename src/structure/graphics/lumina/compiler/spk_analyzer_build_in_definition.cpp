#include "structure/graphics/lumina/compiler/spk_analyzer.hpp"

namespace spk::Lumina
{
	namespace
	{
		void registerBuiltinTypes(NamespaceSymbol &p_namespace)
		{
			auto &types = p_namespace.types;

			types.emplace(L"float", TypeSymbol{L"float", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"int", TypeSymbol{L"int", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"uint", TypeSymbol{L"uint", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"bool", TypeSymbol{L"bool", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Vector2", TypeSymbol{L"Vector2", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Vector2Int", TypeSymbol{L"Vector2Int", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Vector2UInt", TypeSymbol{L"Vector2UInt", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Vector3", TypeSymbol{L"Vector3", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Vector3Int", TypeSymbol{L"Vector3Int", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Vector3UInt", TypeSymbol{L"Vector3UInt", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Vector4", TypeSymbol{L"Vector4", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Vector4Int", TypeSymbol{L"Vector4Int", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Vector4UInt", TypeSymbol{L"Vector4UInt", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Matrix2", TypeSymbol{L"Matrix2", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Matrix3", TypeSymbol{L"Matrix3", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Matrix4", TypeSymbol{L"Matrix4", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Color", TypeSymbol{L"Color", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"void", TypeSymbol{L"void", TypeSymbol::Role::Structure, {}, {}, {}, {}});

			types[L"int"].convertible.insert(&types[L"float"]);
			types[L"uint"].convertible.insert(&types[L"float"]);
			types[L"float"].convertible.insert(&types[L"int"]);
			types[L"float"].convertible.insert(&types[L"uint"]);
			types[L"uint"].convertible.insert(&types[L"int"]);
			types[L"Vector2Int"].convertible.insert(&types[L"Vector2"]);
			types[L"Vector2UInt"].convertible.insert(&types[L"Vector2"]);
			types[L"Vector2"].convertible.insert(&types[L"Vector2Int"]);
			types[L"Vector2"].convertible.insert(&types[L"Vector2UInt"]);
			types[L"Vector2UInt"].convertible.insert(&types[L"Vector2Int"]);
			types[L"Vector2Int"].convertible.insert(&types[L"Vector2UInt"]);
			types[L"Vector3Int"].convertible.insert(&types[L"Vector3"]);
			types[L"Vector3UInt"].convertible.insert(&types[L"Vector3"]);
			types[L"Vector3"].convertible.insert(&types[L"Vector3Int"]);
			types[L"Vector3"].convertible.insert(&types[L"Vector3UInt"]);
			types[L"Vector3UInt"].convertible.insert(&types[L"Vector3Int"]);
			types[L"Vector3Int"].convertible.insert(&types[L"Vector3UInt"]);
			types[L"Vector4Int"].convertible.insert(&types[L"Vector4"]);
			types[L"Vector4UInt"].convertible.insert(&types[L"Vector4"]);
			types[L"Vector4"].convertible.insert(&types[L"Vector4Int"]);
			types[L"Vector4"].convertible.insert(&types[L"Vector4UInt"]);
			types[L"Vector4UInt"].convertible.insert(&types[L"Vector4Int"]);
			types[L"Vector4Int"].convertible.insert(&types[L"Vector4UInt"]);
		}

		void registerBuiltinFunctions(NamespaceSymbol &p_namespace)
		{
			FunctionSymbol vertexPass;
			vertexPass.name = L"VertexPass";
			vertexPass.returnType = &p_namespace.types[L"void"];
			vertexPass.signature = L"VertexPass()";
			p_namespace.functionSignatures[L"VertexPass"].push_back(vertexPass);
			p_namespace.functions.push_back(vertexPass);

			FunctionSymbol fragmentPass;
			fragmentPass.name = L"FragmentPass";
			fragmentPass.returnType = &p_namespace.types[L"void"];
			fragmentPass.signature = L"FragmentPass()";
			p_namespace.functionSignatures[L"FragmentPass"].push_back(fragmentPass);
			p_namespace.functions.push_back(fragmentPass);

			auto &type = p_namespace.types;
			auto makeVar = [&](const std::wstring &p_name, const std::wstring &p_typeName)
			{
				Variable variable;
				variable.name = p_name;
				variable.type = &type[p_typeName];
				return variable;
			};

			{
				FunctionSymbol symbol;

				symbol.name = L"float";
				symbol.returnType = &type[L"float"];
				symbol.signature = L"float()";
				type[L"float"].constructors.push_back(symbol);

				symbol.signature = L"float(float value)";
				symbol.parameters.push_back(makeVar(L"value", L"float"));
				type[L"float"].constructors.push_back(symbol);

				symbol.signature = L"float(int value)";
				symbol.parameters.push_back(makeVar(L"value", L"int"));
				type[L"float"].constructors.push_back(symbol);
			}

			{
				FunctionSymbol functionSymbol;

				functionSymbol.name = L"abs";
				functionSymbol.returnType = &type[L"float"];
				functionSymbol.parameters.push_back(makeVar(L"value", L"float"));
				functionSymbol.signature = L"float abs(float value)";

				p_namespace.functionSignatures[functionSymbol.name].push_back(functionSymbol);
				p_namespace.functions.push_back(functionSymbol);

				type[L"float"].operators[functionSymbol.name].push_back(functionSymbol);
			}
			{
				FunctionSymbol functionSymbol;

				functionSymbol.name = L"length";
				functionSymbol.returnType = &type[L"float"];
				functionSymbol.parameters.push_back(makeVar(L"v", L"Vector2"));
				functionSymbol.signature = L"float length(Vector2 v)";

				p_namespace.functionSignatures[functionSymbol.name].push_back(functionSymbol);
				p_namespace.functions.push_back(functionSymbol);

				type[L"Vector2"].operators[functionSymbol.name].push_back(functionSymbol);
			}
			{
				FunctionSymbol functionSymbol;

				functionSymbol.name = L"normalize";
				functionSymbol.returnType = &type[L"Vector2"];
				functionSymbol.parameters.push_back(makeVar(L"v", L"Vector2"));
				functionSymbol.signature = L"Vector2 normalize(Vector2 v)";

				p_namespace.functionSignatures[functionSymbol.name].push_back(functionSymbol);
				p_namespace.functions.push_back(functionSymbol);

				type[L"Vector2"].operators[functionSymbol.name].push_back(functionSymbol);
			}
			{
				FunctionSymbol functionSymbol;

				functionSymbol.name = L"length";
				functionSymbol.returnType = &type[L"float"];
				functionSymbol.parameters.push_back(makeVar(L"v", L"Vector3"));
				functionSymbol.signature = L"float length(Vector3 v)";

				p_namespace.functionSignatures[functionSymbol.name].push_back(functionSymbol);
				p_namespace.functions.push_back(functionSymbol);

				type[L"Vector3"].operators[functionSymbol.name].push_back(functionSymbol);
			}
			{
				FunctionSymbol functionSymbol;

				functionSymbol.name = L"cross";
				functionSymbol.returnType = &type[L"Vector3"];
				functionSymbol.parameters.push_back(makeVar(L"a", L"Vector3"));
				functionSymbol.parameters.push_back(makeVar(L"b", L"Vector3"));
				functionSymbol.signature = L"Vector3 cross(Vector3 a, Vector3 b)";

				p_namespace.functionSignatures[functionSymbol.name].push_back(functionSymbol);
				p_namespace.functions.push_back(functionSymbol);

				type[L"Vector3"].operators[functionSymbol.name].push_back(functionSymbol);
			}
		}
	} // namespace

	void Analyzer::_defineBuiltinTypes()
	{
		registerBuiltinTypes(_shaderData.globalNamespace);
	}

	void Analyzer::_defineBuiltinFunctions()
	{
		registerBuiltinFunctions(_shaderData.globalNamespace);
	}

} // namespace spk::Lumina
