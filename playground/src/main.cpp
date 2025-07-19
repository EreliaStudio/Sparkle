#include <sparkle.hpp>

#include "structure/graphics/lumina/compiler/spk_compiler.hpp"

int main()
{
	std::wstring luminaCode = LR"(#include <toInclude.lum> //Coucou

Input -> VertexPass : Vector2 modelPosition;
Input -> VertexPass : Color modelColor;

VertexPass -> FragmentPass : Color fragmentColor;

VertexPass()
{
	fragmentColor = modelColor; // End line comment
	pixelPosition = Vector4(modelPosition, 0.0, 1.0);
}

struct Test
{
	int name;
	Vector2 name2;
};

AttributeBlock myAttribute
{
	Vector3 attribute1;
	Vector4 attribute2;
};

ConstantBlock myConstants
{
	Matrix4x4 constant1;
	Color constant2;

	int methodName(Vector2Int param1, float param2)
	{
		return 10;
	}
};

/*

Multiline commentary
*/

int someFunction1()
{
return 0;
}

int someFunction2(int a, int b, int c)
{
return (0);
}

Vector2 someFunction3(int a, int b)
{
return Vector2(0, a - b);
}

FragmentPass()
{
	if (fragmentColor.a == 0)
	{
		discard;
	}

	int value = someFunction1();
	Vector2 value2 = someFunction2(1, value, someFunction3(1, 2));

	pixelColor = fragmentColor;
}
	)";

	spk::Lumina::Compiler compiler;

	spk::Lumina::Shader compiledCode = compiler.compile(luminaCode);

	return (0);
}