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
