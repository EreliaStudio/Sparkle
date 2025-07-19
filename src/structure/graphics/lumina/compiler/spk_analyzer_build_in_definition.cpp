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
			p_namespace.addType(L"Texture");
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

			// Vector3 members
			types[L"Vector3"].members.push_back({L"x", &types[L"float"]});
			types[L"Vector3"].members.push_back({L"y", &types[L"float"]});
			types[L"Vector3"].members.push_back({L"z", &types[L"float"]});

			// Vector3 constructors
			types[L"Vector3"].addConstructor({});
			types[L"Vector3"].addConstructor({{L"x", &types[L"float"]}, {L"y", &types[L"float"]}, {L"z", &types[L"float"]}});
			types[L"Vector3"].addConstructor({{L"vec", &types[L"Vector3Int"]}});
			types[L"Vector3"].addConstructor({{L"vec", &types[L"Vector3UInt"]}});
			types[L"Vector3"].addConstructor({{L"vec", &types[L"Vector2"]}, {L"z", &types[L"float"]}});
			types[L"Vector3"].addConstructor({{L"x", &types[L"float"]}, {L"vec", &types[L"Vector2"]}});

			// Vector3 methods
			types[L"Vector3"].addMethod(&types[L"float"], L"length", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"normalize", {});
			types[L"Vector3"].addMethod(&types[L"float"], L"dot", {{L"other", &types[L"Vector3"]}});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"cross", {{L"other", &types[L"Vector3"]}});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"reflect", {{L"normal", &types[L"Vector3"]}});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"abs", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"floor", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"ceil", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"fract", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"mod", {{L"divisor", &types[L"Vector3"]}});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"min", {{L"other", &types[L"Vector3"]}});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"max", {{L"other", &types[L"Vector3"]}});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"clamp", {{L"minVal", &types[L"Vector3"]}, {L"maxVal", &types[L"Vector3"]}});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"lerp", {{L"other", &types[L"Vector3"]}, {L"t", &types[L"float"]}});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"step", {{L"edge", &types[L"Vector3"]}});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"smoothstep", {{L"edge0", &types[L"Vector3"]}, {L"edge1", &types[L"Vector3"]}});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"pow", {{L"exponent", &types[L"Vector3"]}});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"exp", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"log", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"exp2", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"log2", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"sqrt", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"inversesqrt", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"sin", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"cos", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"tan", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"asin", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"acos", {});
			types[L"Vector3"].addMethod(&types[L"Vector3"], L"atan", {});

			// Vector3 operators
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"+", {{L"other", &types[L"Vector3"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"-", {{L"other", &types[L"Vector3"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"*", {{L"other", &types[L"Vector3"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"/", {{L"other", &types[L"Vector3"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"+", {{L"scalar", &types[L"float"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"-", {{L"scalar", &types[L"float"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"*", {{L"scalar", &types[L"float"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"/", {{L"scalar", &types[L"float"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"+", {});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"-", {});

			types[L"Vector3"].addOperator(&types[L"Vector3"], L"+=", {{L"other", &types[L"Vector3"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"-=", {{L"other", &types[L"Vector3"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"*=", {{L"other", &types[L"Vector3"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"/=", {{L"other", &types[L"Vector3"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"+=", {{L"scalar", &types[L"float"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"-=", {{L"scalar", &types[L"float"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"*=", {{L"scalar", &types[L"float"]}});
			types[L"Vector3"].addOperator(&types[L"Vector3"], L"/=", {{L"scalar", &types[L"float"]}});

			types[L"Vector3"].addOperator(&types[L"bool"], L"==", {{L"other", &types[L"Vector3"]}});
			types[L"Vector3"].addOperator(&types[L"bool"], L"!=", {{L"other", &types[L"Vector3"]}});

			// Vector4 members
			types[L"Vector4"].members.push_back({L"x", &types[L"float"]});
			types[L"Vector4"].members.push_back({L"y", &types[L"float"]});
			types[L"Vector4"].members.push_back({L"z", &types[L"float"]});
			types[L"Vector4"].members.push_back({L"w", &types[L"float"]});

			// Vector4 constructors
			types[L"Vector4"].addConstructor({});
			types[L"Vector4"].addConstructor(
				{{L"x", &types[L"float"]}, {L"y", &types[L"float"]}, {L"z", &types[L"float"]}, {L"w", &types[L"float"]}});
			types[L"Vector4"].addConstructor({{L"vec", &types[L"Vector4Int"]}});
			types[L"Vector4"].addConstructor({{L"vec", &types[L"Vector4UInt"]}});
			types[L"Vector4"].addConstructor({{L"vec1", &types[L"Vector2"]}, {L"vec2", &types[L"Vector2"]}});
			types[L"Vector4"].addConstructor({{L"vec", &types[L"Vector3"]}, {L"w", &types[L"float"]}});
			types[L"Vector4"].addConstructor({{L"x", &types[L"float"]}, {L"vec", &types[L"Vector3"]}});

			// Vector4 methods
			types[L"Vector4"].addMethod(&types[L"float"], L"length", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"normalize", {});
			types[L"Vector4"].addMethod(&types[L"float"], L"dot", {{L"other", &types[L"Vector4"]}});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"reflect", {{L"normal", &types[L"Vector4"]}});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"abs", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"floor", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"ceil", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"fract", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"mod", {{L"divisor", &types[L"Vector4"]}});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"min", {{L"other", &types[L"Vector4"]}});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"max", {{L"other", &types[L"Vector4"]}});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"clamp", {{L"minVal", &types[L"Vector4"]}, {L"maxVal", &types[L"Vector4"]}});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"lerp", {{L"other", &types[L"Vector4"]}, {L"t", &types[L"float"]}});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"step", {{L"edge", &types[L"Vector4"]}});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"smoothstep", {{L"edge0", &types[L"Vector4"]}, {L"edge1", &types[L"Vector4"]}});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"pow", {{L"exponent", &types[L"Vector4"]}});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"exp", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"log", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"exp2", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"log2", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"sqrt", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"inversesqrt", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"sin", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"cos", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"tan", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"asin", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"acos", {});
			types[L"Vector4"].addMethod(&types[L"Vector4"], L"atan", {});

			// Vector4 operators
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"+", {{L"other", &types[L"Vector4"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"-", {{L"other", &types[L"Vector4"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"*", {{L"other", &types[L"Vector4"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"/", {{L"other", &types[L"Vector4"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"+", {{L"scalar", &types[L"float"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"-", {{L"scalar", &types[L"float"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"*", {{L"scalar", &types[L"float"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"/", {{L"scalar", &types[L"float"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"+", {});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"-", {});

			types[L"Vector4"].addOperator(&types[L"Vector4"], L"+=", {{L"other", &types[L"Vector4"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"-=", {{L"other", &types[L"Vector4"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"*=", {{L"other", &types[L"Vector4"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"/=", {{L"other", &types[L"Vector4"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"+=", {{L"scalar", &types[L"float"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"-=", {{L"scalar", &types[L"float"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"*=", {{L"scalar", &types[L"float"]}});
			types[L"Vector4"].addOperator(&types[L"Vector4"], L"/=", {{L"scalar", &types[L"float"]}});

			types[L"Vector4"].addOperator(&types[L"bool"], L"==", {{L"other", &types[L"Vector4"]}});
			types[L"Vector4"].addOperator(&types[L"bool"], L"!=", {{L"other", &types[L"Vector4"]}});

			// Matrix2x2 constructors
			types[L"Matrix2x2"].addConstructor({});

			// Matrix2x2 methods
			types[L"Matrix2x2"].addMethod(&types[L"Matrix2x2"], L"inverse", {});
			types[L"Matrix2x2"].addMethod(&types[L"Matrix2x2"], L"transpose", {});
			types[L"Matrix2x2"].addMethod(&types[L"float"], L"determinant", {});

			// Matrix2x2 operators
			types[L"Matrix2x2"].addOperator(&types[L"Matrix2x2"], L"*", {{L"other", &types[L"Matrix2x2"]}});
			types[L"Matrix2x2"].addOperator(&types[L"Vector2"], L"*", {{L"other", &types[L"Vector2"]}});

			// Matrix3x3 constructors
			types[L"Matrix3x3"].addConstructor({});
			types[L"Matrix3x3"].addConstructor({{L"mat", &types[L"Matrix4x4"]}});

			// Matrix3x3 methods
			types[L"Matrix3x3"].addMethod(&types[L"Matrix3x3"], L"inverse", {});
			types[L"Matrix3x3"].addMethod(&types[L"Matrix3x3"], L"transpose", {});
			types[L"Matrix3x3"].addMethod(&types[L"float"], L"determinant", {});

			// Matrix3x3 operators
			types[L"Matrix3x3"].addOperator(&types[L"Matrix3x3"], L"*", {{L"other", &types[L"Matrix3x3"]}});
			types[L"Matrix3x3"].addOperator(&types[L"Vector3"], L"*", {{L"other", &types[L"Vector3"]}});

			// Matrix4x4 constructors
			types[L"Matrix4x4"].addConstructor({});

			// Matrix4x4 methods
			types[L"Matrix4x4"].addMethod(&types[L"Matrix4x4"], L"inverse", {});
			types[L"Matrix4x4"].addMethod(&types[L"Matrix4x4"], L"transpose", {});
			types[L"Matrix4x4"].addMethod(&types[L"float"], L"determinant", {});
			types[L"Matrix4x4"].addMethod(&types[L"Matrix4x4"], L"rotation", {{L"angles", &types[L"Vector3"]}});
			types[L"Matrix4x4"].addMethod(&types[L"Matrix4x4"], L"translation", {{L"offset", &types[L"Vector3"]}});
			types[L"Matrix4x4"].addMethod(&types[L"Matrix4x4"], L"scale", {{L"factors", &types[L"Vector3"]}});
			types[L"Matrix4x4"].addMethod(
				&types[L"Matrix4x4"],
				L"perspective",
				{{L"fov", &types[L"float"]}, {L"ratio", &types[L"float"]}, {L"nearPlane", &types[L"float"]}, {L"farPlane", &types[L"float"]}});
			types[L"Matrix4x4"].addMethod(&types[L"Matrix4x4"],
										  L"ortho",
										  {{L"left", &types[L"float"]},
										   {L"right", &types[L"float"]},
										   {L"bottom", &types[L"float"]},
										   {L"top", &types[L"float"]},
										   {L"nearPlane", &types[L"float"]},
										   {L"farPlane", &types[L"float"]}});

			// Matrix4x4 operators
			types[L"Matrix4x4"].addOperator(&types[L"Matrix4x4"], L"*", {{L"other", &types[L"Matrix4x4"]}});
			types[L"Matrix4x4"].addOperator(&types[L"Vector4"], L"*", {{L"other", &types[L"Vector4"]}});
			types[L"Matrix4x4"].addOperator(&types[L"Vector3"], L"*", {{L"other", &types[L"Vector3"]}});

			// Color members
			types[L"Color"].members.push_back({L"r", &types[L"float"]});
			types[L"Color"].members.push_back({L"g", &types[L"float"]});
			types[L"Color"].members.push_back({L"b", &types[L"float"]});
			types[L"Color"].members.push_back({L"a", &types[L"float"]});

			// Color constructors
			types[L"Color"].addConstructor({});
			types[L"Color"].addConstructor({{L"r", &types[L"float"]}, {L"g", &types[L"float"]}, {L"b", &types[L"float"]}, {L"a", &types[L"float"]}});

			// Color methods
			types[L"Color"].addMethod(&types[L"Color"], L"lerp", {{L"other", &types[L"Color"]}, {L"t", &types[L"float"]}});
			types[L"Color"].addMethod(&types[L"Color"], L"clamp", {{L"minVal", &types[L"Color"]}, {L"maxVal", &types[L"Color"]}});
			types[L"Color"].addMethod(&types[L"Color"], L"saturate", {});
			types[L"Color"].addMethod(&types[L"Color"], L"min", {{L"other", &types[L"Color"]}});
			types[L"Color"].addMethod(&types[L"Color"], L"max", {{L"other", &types[L"Color"]}});
			types[L"Color"].addMethod(&types[L"Color"], L"abs", {});
			types[L"Color"].addMethod(&types[L"Color"], L"smoothstep", {{L"edge0", &types[L"Color"]}, {L"edge1", &types[L"Color"]}});
			types[L"Color"].addMethod(&types[L"Color"], L"step", {{L"edge", &types[L"Color"]}});

			// Color operators
			types[L"Color"].addOperator(&types[L"Color"], L"+", {{L"other", &types[L"Color"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"-", {{L"other", &types[L"Color"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"*", {{L"other", &types[L"Color"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"/", {{L"other", &types[L"Color"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"+", {{L"scalar", &types[L"float"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"-", {{L"scalar", &types[L"float"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"*", {{L"scalar", &types[L"float"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"/", {{L"scalar", &types[L"float"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"+", {});
			types[L"Color"].addOperator(&types[L"Color"], L"-", {});

			types[L"Color"].addOperator(&types[L"Color"], L"+=", {{L"other", &types[L"Color"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"-=", {{L"other", &types[L"Color"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"*=", {{L"other", &types[L"Color"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"/=", {{L"other", &types[L"Color"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"+=", {{L"scalar", &types[L"float"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"-=", {{L"scalar", &types[L"float"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"*=", {{L"scalar", &types[L"float"]}});
			types[L"Color"].addOperator(&types[L"Color"], L"/=", {{L"scalar", &types[L"float"]}});

			types[L"Color"].addOperator(&types[L"bool"], L"==", {{L"other", &types[L"Color"]}});
			types[L"Color"].addOperator(&types[L"bool"], L"!=", {{L"other", &types[L"Color"]}});

			// Texture methods
			types[L"Texture"].addMethod(&types[L"Color"], L"getPixel", {{L"UVs", &types[L"Vector2"]}});
			types[L"Texture"].addMethod(&types[L"Vector2"], L"size", {});
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
