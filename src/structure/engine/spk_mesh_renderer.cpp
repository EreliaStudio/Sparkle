#include "structure/engine/spk_mesh_renderer.hpp"

namespace spk
{
	std::string MeshRenderer::_pipelineCode = R"(## LAYOUTS DEFINITION ##
0 Vector3 modelPosition
1 Vector2 modelUVs
2 Vector3 modelNormals


## FRAMEBUFFER DEFINITION ##
0 Color pixelColor


## CONSTANTS DEFINITION ##
CameraConstants_Type CameraConstants 128 128 {
    viewMatrix 0 64 0 64 1 0 {}
    projectionMatrix 64 64 64 64 1 0 {}
}


## ATTRIBUTES DEFINITION ##
modelInformations_Type modelInformations 128 128 {
    modelMatrix 0 64 0 64 1 0 {}
    inverseModelMatrix 64 64 64 64 1 0 {}
}


## TEXTURES DEFINITION ##
diffuseTexture Texture_diffuseTexture
specularTexture Texture_specularTexture


## VERTEX SHADER CODE ##
#version 450

layout (location = 0) in vec3 modelPosition;
layout (location = 1) in vec2 modelUVs;
layout (location = 2) in vec3 modelNormals;
layout (location = 0) out flat int out_instanceID;
layout (location = 1) out vec2 fragmentUVs;
layout (location = 2) out vec3 fragmentNormal;

layout(attributes) uniform modelInformations_Type
{
    mat4 modelMatrix;
    mat4 inverseModelMatrix;
} modelInformations;

layout(constants) uniform CameraConstants_Type
{
    mat4 viewMatrix;
    mat4 projectionMatrix;
} CameraConstants;

uniform sampler2D Texture_diffuseTexture;

uniform sampler2D Texture_specularTexture;

void main()
{
    vec4 worldPosition = modelInformations.modelMatrix * (vec4(modelPosition, 1.0));
    vec4 viewPosition = CameraConstants.viewMatrix * worldPosition;
    gl_Position = (CameraConstants.projectionMatrix * viewPosition);
    fragmentUVs = modelUVs;
    fragmentNormal = (modelInformations.inverseModelMatrix * (vec4(modelNormals, 0)).xyz);
    out_instanceID = gl_InstanceID;
}

## FRAGMENT SHADER CODE ##
#version 450

layout (location = 0) in flat int instanceID;
layout (location = 1) in vec2 fragmentUVs;
layout (location = 2) in vec3 fragmentNormal;
layout (location = 0) out vec4 pixelColor;

uniform sampler2D Texture_diffuseTexture;

uniform sampler2D Texture_specularTexture;

void main()
{
    vec3 lightDirection = normalize(vec3(0.0, -1.0, 0.0));
    vec3 normal = normalize(fragmentNormal);
    float diffuseIntensity = max(dot(normal, -lightDirection), 0.0);
    vec3 viewDirection = normalize(vec3(0.0, 0.0, 1.0));
    vec3 reflectionDirection = reflect(lightDirection, normal);
    float specularIntensity = pow(max(dot(viewDirection, reflectionDirection), 0.0), 32.0);
    float ambientIntensity = 0.1;
    vec4 texColor = texture(Texture_diffuseTexture, fragmentUVs);
    vec4 specularColor = texture(Texture_specularTexture, fragmentUVs);
    vec4 finalColor = (texColor * (ambientIntensity + diffuseIntensity)) + (specularColor * specularIntensity);
    pixelColor = (vec4(finalColor.rgb, texColor.a));
}
)";

	spk::Pipeline MeshRenderer::_pipeline = spk::Pipeline(_pipelineCode);

	void MeshRenderer::render()
	{
		if (_mesh == nullptr)
		{
			return ;
		}

		if (_mesh->baked() == false)
		{
			_mesh->bake();
			_needMeshUpload = false;
		}

		if (_needMeshUpload == true)
		{
			_object.layout().clear();
			_object.indexes().clear();

			_object.layout() << _mesh->vertices();
			_object.indexes() << _mesh->indexes();

			_object.layout().validate();
			_object.indexes().validate();
		}

		if (_needMaterialUpload == true)
		{
			
			_needMaterialUpload = false;
		}

		_object.render(1);
	}

	MeshRenderer::MeshRenderer(const std::wstring& p_name) :
		spk::Component(p_name),
		_object(_pipeline.createObject()),
		_modelInformations(_object.attribute(L"modelInformations")),
		_modelTexture(_object.sampler2D(L"texture"))
	{
		
	}
}