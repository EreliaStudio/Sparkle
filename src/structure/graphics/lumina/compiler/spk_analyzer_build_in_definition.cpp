#include "structure/graphics/lumina/compiler/spk_analyzer.hpp"

namespace spk::Lumina
{
	namespace
	{

		void registerBuiltinTypes(NamespaceSymbol &p_namespace)
		{
			auto &types = p_namespace.types;

			p_namespace.addType(L"float");
			p_namespace.addType(L"int");
			p_namespace.addType(L"uint");
			p_namespace.addType(L"bool");
			p_namespace.addType(L"Vector2");
			p_namespace.addType(L"Vector2Int");
			p_namespace.addType(L"Vector2UInt");
			p_namespace.addType(L"Vector3");
			p_namespace.addType(L"Vector3Int");
			p_namespace.addType(L"Vector3UInt");
			p_namespace.addType(L"Vector4");
			p_namespace.addType(L"Vector4Int");
			p_namespace.addType(L"Vector4UInt");
			p_namespace.addType(L"Matrix2x2");
			p_namespace.addType(L"Matrix3x3");
			p_namespace.addType(L"Matrix4x4");
			p_namespace.addType(L"Color");
			p_namespace.addType(L"void");

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
			types[L"Vector4"].convertible.insert(&types[L"Vector4UInt"]);
			types[L"Vector4UInt"].convertible.insert(&types[L"Vector4Int"]);
			types[L"Vector4Int"].convertible.insert(&types[L"Vector4UInt"]);

			// Vector2 members
			types[L"Vector2"].members.push_back({L"x", &types[L"float"]});
			types[L"Vector2"].members.push_back({L"y", &types[L"float"]});

			// Vector2 constructors
			types[L"Vector2"].addConstructor({});
			types[L"Vector2"].addConstructor({{L"x", &types[L"float"]}, {L"y", &types[L"float"]}});
			types[L"Vector2"].addConstructor({{L"vec", &types[L"Vector2Int"]}});
			types[L"Vector2"].addConstructor({{L"vec", &types[L"Vector2UInt"]}});

			// Vector2 methods
			types[L"Vector2"].addMethod(&types[L"float"], L"length", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"normalize", {});
			types[L"Vector2"].addMethod(&types[L"float"], L"dot", {{L"other", &types[L"Vector2"]}});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"reflect", {{L"normal", &types[L"Vector2"]}});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"abs", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"floor", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"ceil", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"fract", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"mod", {{L"divisor", &types[L"Vector2"]}});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"min", {{L"other", &types[L"Vector2"]}});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"max", {{L"other", &types[L"Vector2"]}});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"clamp", {{L"minVal", &types[L"Vector2"]}, {L"maxVal", &types[L"Vector2"]}});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"lerp", {{L"other", &types[L"Vector2"]}, {L"t", &types[L"float"]}});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"step", {{L"edge", &types[L"Vector2"]}});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"smoothstep", {{L"edge0", &types[L"Vector2"]}, {L"edge1", &types[L"Vector2"]}});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"pow", {{L"exponent", &types[L"Vector2"]}});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"exp", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"log", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"exp2", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"log2", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"sqrt", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"inversesqrt", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"sin", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"cos", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"tan", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"asin", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"acos", {});
			types[L"Vector2"].addMethod(&types[L"Vector2"], L"atan", {});

			// Vector2 operators
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"+", {{L"other", &types[L"Vector2"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"-", {{L"other", &types[L"Vector2"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"*", {{L"other", &types[L"Vector2"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"/", {{L"other", &types[L"Vector2"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"+", {{L"scalar", &types[L"float"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"-", {{L"scalar", &types[L"float"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"*", {{L"scalar", &types[L"float"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"/", {{L"scalar", &types[L"float"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"+", {});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"-", {});

			types[L"Vector2"].addOperator(&types[L"Vector2"], L"+=", {{L"other", &types[L"Vector2"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"-=", {{L"other", &types[L"Vector2"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"*=", {{L"other", &types[L"Vector2"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"/=", {{L"other", &types[L"Vector2"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"+=", {{L"scalar", &types[L"float"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"-=", {{L"scalar", &types[L"float"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"*=", {{L"scalar", &types[L"float"]}});
			types[L"Vector2"].addOperator(&types[L"Vector2"], L"/=", {{L"scalar", &types[L"float"]}});

			types[L"Vector2"].addOperator(&types[L"bool"], L"==", {{L"other", &types[L"Vector2"]}});
			types[L"Vector2"].addOperator(&types[L"bool"], L"!=", {{L"other", &types[L"Vector2"]}});
		}

		void registerBuiltinFunctions(NamespaceSymbol &p_namespace)
		{
			FunctionSymbol vertexPass(L"VertexPass", &p_namespace.types[L"void"], {});
			p_namespace.functionSignatures[L"VertexPass"].push_back(vertexPass);
			p_namespace.functions.push_back(vertexPass);

			FunctionSymbol fragmentPass(L"FragmentPass", &p_namespace.types[L"void"], {});
			p_namespace.functionSignatures[L"FragmentPass"].push_back(fragmentPass);
			p_namespace.functions.push_back(fragmentPass);
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
