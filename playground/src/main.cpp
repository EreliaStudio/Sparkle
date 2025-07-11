#include <sparkle.hpp>

#include "structure/graphics/lumina/spk_compiler.hpp"

int main()
{
	std::wstring luminaCode = LR"(
		#include <test>

		Input -> VertexPass : Vector2 modelPosition;
		Input -> VertexPass : Color modelColor;

		VertexPass -> FragmentPass : Color fragmentColor;

		VertexPass()
		{
			fragmentColor = modelColor; // End line comment
			pixelPosition = vec4(modelPosition, 0.0, 1.0);
		}

		/*
		
		Multiline commentary
		*/

		FragmentPass()
		{
			if (fragmentColor.a == 0)
			{
				discard;
			}

			pixelColor = fragmentColor;
		}
	)";

	spk::Lumina::Compiler compiler;

	spk::Lumina::Shader compiledCode = compiler.compile(luminaCode);

	return (0);
}