#version 450

layout (location = 0) in flat vec4 inputColor;

layout (location = 0) out vec4 outputColor;

void main()
{
	outputColor = inputColor;
}