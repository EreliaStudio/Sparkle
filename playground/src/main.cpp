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
	pixelPosition = vec4(modelPosition, 0.0, 1.0);
}

struct Test
{
	type name;
	type2 name2;
};

AttributeBlock myAttribute
{
	type attribute1;
	type attribute2;
};

ConstantBlock myConstants
{
	type constant1;
	type constant2;

	type methodName(type param1, type2 param2)
	{
		return returnValue;
	}
};

/*

Multiline commentary
*/

FragmentPass()
{
	if (fragmentColor.a == 0)
	{
		discard;
	}

	int value = someFunction1();
	int value = someFunction2(param1, param2, someFunction3(valueA, valueB));


	pixelColor = fragmentColor;
}
	)";

	spk::Lumina::Compiler compiler;

	spk::Lumina::Shader compiledCode = compiler.compile(luminaCode);

	return (0);
}