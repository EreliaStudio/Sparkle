#include "structure/engine/spk_mesh_renderer.hpp"

#include "structure/engine/spk_entity.hpp"

namespace spk
{
	std::string MeshRenderer::_pipelineCode = R"(## LAYOUTS DEFINITION ##
0 Vector3 modelPosition
1 Vector2 modelUVs
2 Vector3 modelNormals


## FRAMEBUFFER DEFINITION ##
0 Color pixelColor


## CONSTANTS DEFINITION ##
spk_CameraConstants_Type spk::CameraConstants 128 128 {
    view 0 64 0 64 1 0 {}
    projection 64 64 64 64 1 0 {}
}


## ATTRIBUTES DEFINITION ##
modelInformations_Type modelInformations 132 132 {
    modelMatrix 0 64 0 64 1 0 {}
    inverseModelMatrix 64 64 64 64 1 0 {}
    modelMaterial 128 4 128 4 1 0 {
        shininess 0 4 0 4 1 0 {}
    }
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

struct Material
{
    float shininess;
};

layout(attributes) uniform modelInformations_Type
{
    mat4 modelMatrix;
    mat4 inverseModelMatrix;
    Material modelMaterial;
} modelInformations;

layout(constants) uniform spk_CameraConstants_Type
{
    mat4 view;
    mat4 projection;
} spk_CameraConstants;

uniform sampler2D Texture_diffuseTexture;

uniform sampler2D Texture_specularTexture;

void main()
{
    vec4 worldPosition = modelInformations.modelMatrix * (vec4(modelPosition, 1.0));
    vec4 viewPosition = spk_CameraConstants.view * worldPosition;
    gl_Position = (spk_CameraConstants.projection * viewPosition);
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

struct Material
{
    float shininess;
};

layout(attributes) uniform modelInformations_Type
{
    mat4 modelMatrix;
    mat4 inverseModelMatrix;
    Material modelMaterial;
} modelInformations;

uniform sampler2D Texture_diffuseTexture;

uniform sampler2D Texture_specularTexture;

void main()
{
    vec3 normalizedNormal = normalize(fragmentNormal);
    vec3 lightDirection = normalize(vec3(0.0, -1.0, -1.0));
    vec4 lightColor = vec4(1.0, 1.0, 1.0, 1.0);
    float ambientStrength = 0.1;
    vec4 ambient = ambientStrength * lightColor;
    float diff = max(dot(normalizedNormal, -lightDirection), 0.0);
    vec4 diffuse = diff * lightColor;
    vec3 viewDirection = normalize(vec3(0.0, 0.0, 1.0));
    vec3 reflectDirection = reflect(lightDirection, normalizedNormal);
    vec4 specularMap = texture(Texture_specularTexture, fragmentUVs);
    float specIntensity = specularMap.r;
    float spec = pow(max(dot(viewDirection, reflectDirection), 0.0), modelInformations.modelMaterial.shininess);
    vec4 specular = (specIntensity * spec) * lightColor;
    vec4 finalColor = (ambient + diffuse) + specular;
    vec4 textureColor = texture(Texture_diffuseTexture, fragmentUVs);
    finalColor *= (vec4(textureColor.rgb, 1.0));
    pixelColor = (vec4(finalColor.rgb, textureColor.a));
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

		_object.render(1);
	}

	void MeshRenderer::awake()
	{
		_onTransformEditionContract = owner()->transform().addOnEditionCallback([&](){
			_modelMatrixElement = owner()->transform().model();
			_modelInverseMatrixElement = owner()->transform().inverseModel();
		});
	}

	void MeshRenderer::sleep()
	{
		_onTransformEditionContract.resign();
	}

	MeshRenderer::MeshRenderer(const std::wstring& p_name) :
		spk::Component(p_name),
		_object(_pipeline.createObject()),
		_modelInformations(_object.attribute(L"modelInformations")),
		_modelMatrixElement(_modelInformations[L"modelMatrix"]),
		_modelInverseMatrixElement(_modelInformations[L"inverseModelMatrix"]),
		_modelMaterialElement(_modelInformations[L"modelMaterial"]),
		_modelDiffuseTexture(_object.sampler2D(L"diffuseTexture")),
		_modelSpecularTexture(_object.sampler2D(L"specularTexture"))
	{
		
	}

	void MeshRenderer::setMesh(spk::SafePointer<spk::Mesh> p_mesh)
	{
		_mesh = p_mesh;
		_needMeshUpload = true;
	}

	void MeshRenderer::setMaterial(spk::SafePointer<spk::Material> p_material)
	{
		_material = p_material;

		_modelDiffuseTexture.bind(_material->diffuse());
		_modelSpecularTexture.bind(_material->specular());

		_modelMaterialElement[L"shininess"] = _material->shininess();
		_modelInformations.validate();
	}

	spk::SafePointer<spk::Mesh> MeshRenderer::mesh() const
	{
		return (_mesh);
	}
	
	spk::SafePointer<spk::Material> MeshRenderer::material() const
	{
		return (_material);
	}
}