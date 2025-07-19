#include "structure/graphics/lumina/compiler/spk_analyzer.hpp"

namespace spk::Lumina
{
namespace
{
    void registerBuiltinTypes(NamespaceSymbol &ns)
    {
        auto &types = ns.types;
        auto add = [&](const std::wstring &name)
        {
            types.emplace(name, TypeSymbol{name, TypeSymbol::Role::Structure, {}, {}, {}, {}});
        };
        add(L"float");
        add(L"int");
        add(L"uint");
        add(L"bool");
        add(L"Vector2");
        add(L"Vector2Int");
        add(L"Vector2UInt");
        add(L"Vector3");
        add(L"Vector3Int");
        add(L"Vector3UInt");
        add(L"Vector4");
        add(L"Vector4Int");
        add(L"Vector4UInt");
        add(L"Matrix2");
        add(L"Matrix3");
        add(L"Matrix4");
        add(L"Color");
        add(L"void");
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

    void registerBuiltinFunctions(NamespaceSymbol &ns)
    {
        FunctionSymbol vert;
        vert.name = L"VertexPass";
        vert.returnType = &ns.types[L"void"];
        vert.signature = L"VertexPass()";
        ns.functionSignatures[L"VertexPass"].push_back(vert);
        ns.functions.push_back(vert);

        FunctionSymbol frag;
        frag.name = L"FragmentPass";
        frag.returnType = &ns.types[L"void"];
        frag.signature = L"FragmentPass()";
        ns.functionSignatures[L"FragmentPass"].push_back(frag);
        ns.functions.push_back(frag);

        auto &t = ns.types;
        auto makeVar = [&](const std::wstring &n, const std::wstring &ty)
        {
            Variable v; v.name = n; v.type = &t[ty]; return v;
        };

        {
            FunctionSymbol s; s.name = L"float"; s.returnType = &t[L"float"]; s.signature = L"float()"; t[L"float"].constructors.push_back(s);
            FunctionSymbol s1 = s; s1.signature = L"float(float value)"; s1.parameters.push_back(makeVar(L"value", L"float")); t[L"float"].constructors.push_back(s1);
            FunctionSymbol s2 = s; s2.signature = L"float(int value)"; s2.parameters.push_back(makeVar(L"value", L"int")); t[L"float"].constructors.push_back(s2);
        }

        {
            FunctionSymbol f; f.name = L"abs"; f.returnType = &t[L"float"]; f.parameters.push_back(makeVar(L"value", L"float")); f.signature = L"float abs(float value)"; ns.functionSignatures[f.name].push_back(f); ns.functions.push_back(f); t[L"float"].operators[f.name].push_back(f);
        }
        {
            FunctionSymbol f; f.name = L"length"; f.returnType = &t[L"float"]; f.parameters.push_back(makeVar(L"v", L"Vector2")); f.signature = L"float length(Vector2 v)"; ns.functionSignatures[f.name].push_back(f); ns.functions.push_back(f); t[L"Vector2"].operators[f.name].push_back(f);
        }
        {
            FunctionSymbol f; f.name = L"normalize"; f.returnType = &t[L"Vector2"]; f.parameters.push_back(makeVar(L"v", L"Vector2")); f.signature = L"Vector2 normalize(Vector2 v)"; ns.functionSignatures[f.name].push_back(f); ns.functions.push_back(f); t[L"Vector2"].operators[f.name].push_back(f);
        }
        {
            FunctionSymbol f; f.name = L"length"; f.returnType = &t[L"float"]; f.parameters.push_back(makeVar(L"v", L"Vector3")); f.signature = L"float length(Vector3 v)"; ns.functionSignatures[f.name].push_back(f); ns.functions.push_back(f); t[L"Vector3"].operators[f.name].push_back(f);
        }
        {
            FunctionSymbol f; f.name = L"cross"; f.returnType = &t[L"Vector3"]; f.parameters.push_back(makeVar(L"a", L"Vector3")); f.parameters.push_back(makeVar(L"b", L"Vector3")); f.signature = L"Vector3 cross(Vector3 a, Vector3 b)"; ns.functionSignatures[f.name].push_back(f); ns.functions.push_back(f); t[L"Vector3"].operators[f.name].push_back(f);
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

