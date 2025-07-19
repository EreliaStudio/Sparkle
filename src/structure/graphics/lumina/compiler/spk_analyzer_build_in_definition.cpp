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

                        types[L"Matrix2"].convertible.insert(&types[L"Matrix2x2"]);
                        types[L"Matrix2x2"].convertible.insert(&types[L"Matrix2"]);
                        types[L"Matrix3"].convertible.insert(&types[L"Matrix3x3"]);
                        types[L"Matrix3x3"].convertible.insert(&types[L"Matrix3"]);
                        types[L"Matrix4"].convertible.insert(&types[L"Matrix4x4"]);
                        types[L"Matrix4x4"].convertible.insert(&types[L"Matrix4"]);
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

                       auto addConstructor = [&](const std::wstring &typeName,
                                                   const std::vector<std::pair<std::wstring, std::wstring>> &params)
                       {
                               FunctionSymbol ctor;
                               ctor.name = typeName;
                               ctor.returnType = &type[typeName];
                               ctor.signature = typeName + L"(";
                               bool first = true;
                               for (const auto &[pName, pType] : params)
                               {
                                       ctor.parameters.push_back(makeVar(pName, pType));
                                       if (!first)
                                               ctor.signature += L", ";
                                       ctor.signature += pType + L" " + pName;
                                       first = false;
                               }
                               ctor.signature += L")";
                               type[typeName].constructors.push_back(ctor);
                       };

                       auto addFunction = [&](const std::wstring &name,
                                              const std::wstring &ret,
                                              const std::vector<std::pair<std::wstring, std::wstring>> &params,
                                              const std::wstring &opType)
                       {
                               FunctionSymbol fs;
                               fs.name = name;
                               fs.returnType = &type[ret];
                               fs.signature = ret + L" " + name + L"(";
                               bool first = true;
                               for (const auto &[pName, pType] : params)
                               {
                                       fs.parameters.push_back(makeVar(pName, pType));
                                       if (!first)
                                               fs.signature += L", ";
                                       fs.signature += pType + L" " + pName;
                                       first = false;
                               }
                               fs.signature += L")";

                               p_namespace.functionSignatures[name].push_back(fs);
                               p_namespace.functions.push_back(fs);
                               if (!opType.empty())
                               {
                                       type[opType].operators[name].push_back(fs);
                               }
                       };

                       // Constructors for remaining scalar types
                       addConstructor(L"int", {});
                       addConstructor(L"int", {{L"value", L"int"}});
                       addConstructor(L"int", {{L"value", L"uint"}});
                       addConstructor(L"int", {{L"value", L"float"}});

                       addConstructor(L"uint", {});
                       addConstructor(L"uint", {{L"value", L"uint"}});
                       addConstructor(L"uint", {{L"value", L"int"}});
                       addConstructor(L"uint", {{L"value", L"float"}});

                       addConstructor(L"bool", {});
                       addConstructor(L"bool", {{L"value", L"bool"}});
                       addConstructor(L"bool", {{L"value", L"int"}});
                       addConstructor(L"bool", {{L"value", L"uint"}});

                       // Vector constructors
                       addConstructor(L"Vector2", {});
                       addConstructor(L"Vector2", {{L"x", L"float"}, {L"y", L"float"}});
                       addConstructor(L"Vector2", {{L"values", L"float"}});
                       addConstructor(L"Vector2", {{L"vec", L"Vector2Int"}});
                       addConstructor(L"Vector2", {{L"vec", L"Vector2UInt"}});

                       addConstructor(L"Vector2Int", {});
                       addConstructor(L"Vector2Int", {{L"x", L"int"}, {L"y", L"int"}});
                       addConstructor(L"Vector2Int", {{L"values", L"int"}});
                       addConstructor(L"Vector2Int", {{L"vec", L"Vector2"}});
                       addConstructor(L"Vector2Int", {{L"vec", L"Vector2UInt"}});

                       addConstructor(L"Vector2UInt", {});
                       addConstructor(L"Vector2UInt", {{L"x", L"uint"}, {L"y", L"uint"}});
                       addConstructor(L"Vector2UInt", {{L"values", L"uint"}});
                       addConstructor(L"Vector2UInt", {{L"vec", L"Vector2"}});
                       addConstructor(L"Vector2UInt", {{L"vec", L"Vector2Int"}});

                       addConstructor(L"Vector3", {});
                       addConstructor(L"Vector3", {{L"x", L"float"}, {L"y", L"float"}, {L"z", L"float"}});
                       addConstructor(L"Vector3", {{L"values", L"float"}});
                       addConstructor(L"Vector3", {{L"vec", L"Vector2"}, {L"z", L"float"}});
                       addConstructor(L"Vector3", {{L"x", L"float"}, {L"vec", L"Vector2"}});
                       addConstructor(L"Vector3", {{L"vec", L"Vector3Int"}});
                       addConstructor(L"Vector3", {{L"vec", L"Vector3UInt"}});

                       addConstructor(L"Vector3Int", {});
                       addConstructor(L"Vector3Int", {{L"x", L"int"}, {L"y", L"int"}, {L"z", L"int"}});
                       addConstructor(L"Vector3Int", {{L"values", L"int"}});
                       addConstructor(L"Vector3Int", {{L"vec", L"Vector2Int"}, {L"z", L"int"}});
                       addConstructor(L"Vector3Int", {{L"x", L"int"}, {L"vec", L"Vector2Int"}});
                       addConstructor(L"Vector3Int", {{L"vec", L"Vector3"}});
                       addConstructor(L"Vector3Int", {{L"vec", L"Vector3UInt"}});

                       addConstructor(L"Vector3UInt", {});
                       addConstructor(L"Vector3UInt", {{L"x", L"uint"}, {L"y", L"uint"}, {L"z", L"uint"}});
                       addConstructor(L"Vector3UInt", {{L"values", L"uint"}});
                       addConstructor(L"Vector3UInt", {{L"vec", L"Vector2UInt"}, {L"z", L"uint"}});
                       addConstructor(L"Vector3UInt", {{L"x", L"uint"}, {L"vec", L"Vector2UInt"}});
                       addConstructor(L"Vector3UInt", {{L"vec", L"Vector3"}});
                       addConstructor(L"Vector3UInt", {{L"vec", L"Vector3Int"}});

                       addConstructor(L"Vector4", {});
                       addConstructor(L"Vector4", {{L"x", L"float"}, {L"y", L"float"}, {L"z", L"float"}, {L"w", L"float"}});
                       addConstructor(L"Vector4", {{L"values", L"float"}});
                       addConstructor(L"Vector4", {{L"vec1", L"Vector2"}, {L"vec2", L"Vector2"}});
                       addConstructor(L"Vector4", {{L"vec", L"Vector3"}, {L"w", L"float"}});
                       addConstructor(L"Vector4", {{L"x", L"float"}, {L"vec", L"Vector3"}});
                       addConstructor(L"Vector4", {{L"vec", L"Vector4Int"}});
                       addConstructor(L"Vector4", {{L"vec", L"Vector4UInt"}});

                       addConstructor(L"Vector4Int", {});
                       addConstructor(L"Vector4Int", {{L"x", L"int"}, {L"y", L"int"}, {L"z", L"int"}, {L"w", L"int"}});
                       addConstructor(L"Vector4Int", {{L"values", L"int"}});
                       addConstructor(L"Vector4Int", {{L"vec1", L"Vector2Int"}, {L"vec2", L"Vector2Int"}});
                       addConstructor(L"Vector4Int", {{L"vec", L"Vector3Int"}, {L"w", L"int"}});
                       addConstructor(L"Vector4Int", {{L"x", L"int"}, {L"vec", L"Vector3Int"}});
                       addConstructor(L"Vector4Int", {{L"vec", L"Vector4"}});
                       addConstructor(L"Vector4Int", {{L"vec", L"Vector4UInt"}});

                       addConstructor(L"Vector4UInt", {});
                       addConstructor(L"Vector4UInt", {{L"x", L"uint"}, {L"y", L"uint"}, {L"z", L"uint"}, {L"w", L"uint"}});
                       addConstructor(L"Vector4UInt", {{L"values", L"uint"}});
                       addConstructor(L"Vector4UInt", {{L"vec1", L"Vector2UInt"}, {L"vec2", L"Vector2UInt"}});
                       addConstructor(L"Vector4UInt", {{L"vec", L"Vector3UInt"}, {L"w", L"uint"}});
                       addConstructor(L"Vector4UInt", {{L"x", L"uint"}, {L"vec", L"Vector3UInt"}});
                       addConstructor(L"Vector4UInt", {{L"vec", L"Vector4"}});
                       addConstructor(L"Vector4UInt", {{L"vec", L"Vector4Int"}});

                       addConstructor(L"Color", {});
                       addConstructor(L"Color", {{L"r", L"float"}, {L"g", L"float"}, {L"b", L"float"}, {L"a", L"float"}});
                       addConstructor(L"Color", {{L"values", L"float"}});

                       // Float functions
                       addFunction(L"sign", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"floor", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"ceil", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"fract", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"mod", L"float", {{L"value", L"float"}, {L"divisor", L"float"}}, L"float");
                       addFunction(L"min", L"float", {{L"a", L"float"}, {L"b", L"float"}}, L"float");
                       addFunction(L"max", L"float", {{L"a", L"float"}, {L"b", L"float"}}, L"float");
                       addFunction(L"clamp", L"float", {{L"value", L"float"}, {L"minVal", L"float"}, {L"maxVal", L"float"}}, L"float");
                       addFunction(L"mix", L"float", {{L"a", L"float"}, {L"b", L"float"}, {L"t", L"float"}}, L"float");
                       addFunction(L"step", L"float", {{L"edge", L"float"}, {L"value", L"float"}}, L"float");
                       addFunction(L"smoothstep", L"float", {{L"edge0", L"float"}, {L"edge1", L"float"}, {L"value", L"float"}}, L"float");
                       addFunction(L"pow", L"float", {{L"value", L"float"}, {L"exponent", L"float"}}, L"float");
                       addFunction(L"exp", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"log", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"exp2", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"log2", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"sqrt", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"inversesqrt", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"sin", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"cos", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"tan", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"asin", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"acos", L"float", {{L"value", L"float"}}, L"float");
                       addFunction(L"atan", L"float", {{L"value", L"float"}}, L"float");

                       // Integer helpers
                       addFunction(L"abs", L"int", {{L"value", L"int"}}, L"int");
                       addFunction(L"sign", L"int", {{L"value", L"int"}}, L"int");
                       addFunction(L"mod", L"int", {{L"value", L"int"}, {L"divisor", L"int"}}, L"int");
                       addFunction(L"min", L"int", {{L"a", L"int"}, {L"b", L"int"}}, L"int");
                       addFunction(L"max", L"int", {{L"a", L"int"}, {L"b", L"int"}}, L"int");
                       addFunction(L"clamp", L"int", {{L"value", L"int"}, {L"minVal", L"int"}, {L"maxVal", L"int"}}, L"int");

                       // Unsigned integer helpers
                       addFunction(L"mod", L"uint", {{L"value", L"uint"}, {L"divisor", L"uint"}}, L"uint");
                       addFunction(L"min", L"uint", {{L"a", L"uint"}, {L"b", L"uint"}}, L"uint");
                       addFunction(L"max", L"uint", {{L"a", L"uint"}, {L"b", L"uint"}}, L"uint");
                       addFunction(L"clamp", L"uint", {{L"value", L"uint"}, {L"minVal", L"uint"}, {L"maxVal", L"uint"}}, L"uint");

                       // Vector math
                       addFunction(L"dot", L"float", {{L"a", L"Vector2"}, {L"b", L"Vector2"}}, L"Vector2");
                       addFunction(L"reflect", L"Vector2", {{L"I", L"Vector2"}, {L"N", L"Vector2"}}, L"Vector2");
                       addFunction(L"dot", L"float", {{L"a", L"Vector3"}, {L"b", L"Vector3"}}, L"Vector3");
                       addFunction(L"reflect", L"Vector3", {{L"I", L"Vector3"}, {L"N", L"Vector3"}}, L"Vector3");
                       addFunction(L"dot", L"float", {{L"a", L"Vector4"}, {L"b", L"Vector4"}}, L"Vector4");
                       addFunction(L"reflect", L"Vector4", {{L"I", L"Vector4"}, {L"N", L"Vector4"}}, L"Vector4");

                       addFunction(L"length", L"float", {{L"v", L"Vector4"}}, L"Vector4");
                       addFunction(L"normalize", L"Vector3", {{L"v", L"Vector3"}}, L"Vector3");
                       addFunction(L"normalize", L"Vector4", {{L"v", L"Vector4"}}, L"Vector4");

                       // Integer vector helpers
                       addFunction(L"length", L"float", {{L"v", L"Vector2Int"}}, L"Vector2Int");
                       addFunction(L"length", L"float", {{L"v", L"Vector3Int"}}, L"Vector3Int");
                       addFunction(L"length", L"float", {{L"v", L"Vector4Int"}}, L"Vector4Int");

                       addFunction(L"length", L"float", {{L"v", L"Vector2UInt"}}, L"Vector2UInt");
                       addFunction(L"length", L"float", {{L"v", L"Vector3UInt"}}, L"Vector3UInt");
                       addFunction(L"length", L"float", {{L"v", L"Vector4UInt"}}, L"Vector4UInt");

                       // Color helpers
                       addFunction(L"lerp", L"Color", {{L"a", L"Color"}, {L"b", L"Color"}, {L"t", L"float"}}, L"Color");
                       addFunction(L"clamp", L"Color", {{L"value", L"Color"}, {L"minVal", L"Color"}, {L"maxVal", L"Color"}}, L"Color");
                       addFunction(L"saturate", L"Color", {{L"value", L"Color"}}, L"Color");
                       addFunction(L"min", L"Color", {{L"a", L"Color"}, {L"b", L"Color"}}, L"Color");
                       addFunction(L"max", L"Color", {{L"a", L"Color"}, {L"b", L"Color"}}, L"Color");
                       addFunction(L"abs", L"Color", {{L"value", L"Color"}}, L"Color");
                       addFunction(L"smoothstep", L"Color", {{L"edge0", L"Color"}, {L"edge1", L"Color"}, {L"value", L"Color"}}, L"Color");
                       addFunction(L"step", L"Color", {{L"edge", L"Color"}, {L"value", L"Color"}}, L"Color");

                       // Texture methods
                       addFunction(L"getPixel", L"Color", {{L"tex", L"Texture"}, {L"UV", L"Vector2"}}, L"Texture");
                       addFunction(L"size", L"Vector2", {{L"tex", L"Texture"}}, L"Texture");
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

                        // Additional vector helpers
                        auto addVecHelpers = [&](const std::wstring &vec) {
                            addFunction(L"abs", vec, {{L"value", vec}}, vec);
                            addFunction(L"floor", vec, {{L"value", vec}}, vec);
                            addFunction(L"ceil", vec, {{L"value", vec}}, vec);
                            addFunction(L"fract", vec, {{L"value", vec}}, vec);
                            addFunction(L"mod", vec, {{L"value", vec}, {L"divisor", vec}}, vec);
                            addFunction(L"min", vec, {{L"a", vec}, {L"b", vec}}, vec);
                            addFunction(L"max", vec, {{L"a", vec}, {L"b", vec}}, vec);
                            addFunction(L"clamp", vec, {{L"value", vec}, {L"minVal", vec}, {L"maxVal", vec}}, vec);
                            addFunction(L"lerp", vec, {{L"a", vec}, {L"b", vec}, {L"t", L"float"}}, vec);
                            addFunction(L"step", vec, {{L"edge", vec}, {L"value", vec}}, vec);
                            addFunction(L"smoothstep", vec, {{L"edge0", vec}, {L"edge1", vec}, {L"value", vec}}, vec);
                            addFunction(L"pow", vec, {{L"value", vec}, {L"exponent", vec}}, vec);
                            addFunction(L"exp", vec, {{L"value", vec}}, vec);
                            addFunction(L"log", vec, {{L"value", vec}}, vec);
                            addFunction(L"exp2", vec, {{L"value", vec}}, vec);
                            addFunction(L"log2", vec, {{L"value", vec}}, vec);
                            addFunction(L"sqrt", vec, {{L"value", vec}}, vec);
                            addFunction(L"inversesqrt", vec, {{L"value", vec}}, vec);
                            addFunction(L"sin", vec, {{L"value", vec}}, vec);
                            addFunction(L"cos", vec, {{L"value", vec}}, vec);
                            addFunction(L"tan", vec, {{L"value", vec}}, vec);
                            addFunction(L"asin", vec, {{L"value", vec}}, vec);
                            addFunction(L"acos", vec, {{L"value", vec}}, vec);
                            addFunction(L"atan", vec, {{L"value", vec}}, vec);
                        };

                        addVecHelpers(L"Vector2");
                        addVecHelpers(L"Vector3");
                        addVecHelpers(L"Vector4");

                        // Integer vector helpers extension
                        auto addIVecHelpers = [&](const std::wstring &vec) {
                            addFunction(L"abs", vec, {{L"value", vec}}, vec);
                            addFunction(L"mod", vec, {{L"value", vec}, {L"divisor", vec}}, vec);
                            addFunction(L"min", vec, {{L"a", vec}, {L"b", vec}}, vec);
                            addFunction(L"max", vec, {{L"a", vec}, {L"b", vec}}, vec);
                            addFunction(L"clamp", vec, {{L"value", vec}, {L"minVal", vec}, {L"maxVal", vec}}, vec);
                            addFunction(L"sign", vec, {{L"value", vec}}, vec);
                        };

                        addIVecHelpers(L"Vector2Int");
                        addIVecHelpers(L"Vector3Int");
                        addIVecHelpers(L"Vector4Int");

                        auto addUIVecHelpers = [&](const std::wstring &vec) {
                            addFunction(L"mod", vec, {{L"value", vec}, {L"divisor", vec}}, vec);
                            addFunction(L"min", vec, {{L"a", vec}, {L"b", vec}}, vec);
                            addFunction(L"max", vec, {{L"a", vec}, {L"b", vec}}, vec);
                            addFunction(L"clamp", vec, {{L"value", vec}, {L"minVal", vec}, {L"maxVal", vec}}, vec);
                        };

                        addUIVecHelpers(L"Vector2UInt");
                        addUIVecHelpers(L"Vector3UInt");
                        addUIVecHelpers(L"Vector4UInt");

                        // Matrix-vector multiplication operators
                        addFunction(L"operator*", L"Vector2", {{L"lhs", L"Matrix2"}, {L"rhs", L"Vector2"}}, L"Matrix2");
                        addFunction(L"operator*", L"Vector2", {{L"lhs", L"Matrix2x2"}, {L"rhs", L"Vector2"}}, L"Matrix2x2");
                        addFunction(L"operator*", L"Vector3", {{L"lhs", L"Matrix3"}, {L"rhs", L"Vector3"}}, L"Matrix3");
                        addFunction(L"operator*", L"Vector3", {{L"lhs", L"Matrix3x3"}, {L"rhs", L"Vector3"}}, L"Matrix3x3");
                        addFunction(L"operator*", L"Vector4", {{L"lhs", L"Matrix4"}, {L"rhs", L"Vector4"}}, L"Matrix4");
                        addFunction(L"operator*", L"Vector4", {{L"lhs", L"Matrix4x4"}, {L"rhs", L"Vector4"}}, L"Matrix4x4");
                        addFunction(L"operator*", L"Vector3", {{L"lhs", L"Matrix4"}, {L"rhs", L"Vector3"}}, L"Matrix4");
                        addFunction(L"operator*", L"Vector3", {{L"lhs", L"Matrix4x4"}, {L"rhs", L"Vector3"}}, L"Matrix4x4");
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
