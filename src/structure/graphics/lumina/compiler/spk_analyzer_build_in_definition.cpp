#include "structure/graphics/lumina/compiler/spk_analyzer.hpp"

namespace spk::Lumina
{
	namespace
	{
		std::wstring composeSignature(const std::wstring& p_name, const std::vector<Variable> &p_parameters)
		{
			std::wstring result = L"";

			result = p_name + L"(";
			std::wstring parameterString = L"";
			for (const auto &variable : p_parameters)
			{
				if (parameterString.empty() == false)
				{
					parameterString += L", ";
				}
				parameterString += variable.type->name;
			}
			result += parameterString + L")";
		
			return (result);
		}

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
			types.emplace(L"Matrix2x2", TypeSymbol{L"Matrix2x2", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Matrix3x3", TypeSymbol{L"Matrix3x3", TypeSymbol::Role::Structure, {}, {}, {}, {}});
			types.emplace(L"Matrix4x4", TypeSymbol{L"Matrix4x4", TypeSymbol::Role::Structure, {}, {}, {}, {}});
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

			auto addFunction = [&](TypeSymbol* p_returnType, const std::wstring& p_methodName, const std::vector<Variable> &p_parameters)
			{
				FunctionSymbol newMethod;

				newMethod.name = p_methodName;
				newMethod.returnType = p_returnType;
				newMethod.parameters = p_parameters;
				newMethod.signature = composeSignature(newMethod.name, p_parameters);
				newMethod.body = {};

				p_namespace.functions.push_back(newMethod);
			};

			auto addConstructorType = [&](TypeSymbol* p_methodType, const std::vector<Variable> &p_parameters)
			{
				FunctionSymbol constructor;
				
				constructor.name = p_methodType->name;
				constructor.returnType = p_methodType;
				constructor.parameters = p_parameters;
				constructor.signature = composeSignature(constructor.name, p_parameters);
				constructor.body = {};

				p_methodType->constructors.push_back(constructor);
				p_namespace.functions.push_back(constructor);
			};

			auto addMethodToType = [&](TypeSymbol* p_methodType, TypeSymbol* p_returnType, const std::wstring& p_methodName, const std::vector<Variable> &p_parameters)
			{
				FunctionSymbol newMethod;

				newMethod.name = p_methodName;
				newMethod.returnType = p_returnType;
				newMethod.parameters = p_parameters;
				newMethod.signature = composeSignature(newMethod.name, p_parameters);
				newMethod.body = {};

				p_methodType->methods.push_back(newMethod);
				p_namespace.functions.push_back(newMethod);
			};

			auto addOperatorToType = [&](TypeSymbol* p_methodType, TypeSymbol* p_returnType, const std::wstring& p_operatorType, const std::vector<Variable> &p_parameters){
				FunctionSymbol newMethod;

				newMethod.name = L"Operator" + p_operatorType;
				newMethod.returnType = p_returnType;
				newMethod.parameters = p_parameters;
				newMethod.signature = composeSignature(newMethod.name, p_parameters);
				newMethod.body = {};

				p_methodType->methods.push_back(newMethod);
				p_namespace.functions.push_back(newMethod);
			};
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
