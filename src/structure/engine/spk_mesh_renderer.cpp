#include "structure/engine/spk_mesh_renderer.hpp"

#include "structure/engine/spk_entity.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	std::string MeshRenderer::_pipelineCode = R"(## LAYOUTS DEFINITION ##
0 Vector3 modelPosition
1 Vector2 modelUVs
2 Vector3 modelNormals


## FRAMEBUFFER DEFINITION ##
0 Color pixelColor


## CONSTANTS DEFINITION ##
spk_CameraConstants_Type spk::CameraConstants 140 144 {
    position 0 12 0 12 1 4 {
        x 0 4 0 4 1 0 {}
        y 4 4 4 4 1 0 {}
        z 8 4 8 4 1 0 {}
    }
    view 12 64 16 64 1 0 {}
    projection 76 64 80 64 1 0 {}
}
WorldConstants_Type WorldConstants 32 36 {
    directionalLight 0 32 0 36 1 12 {
        direction 0 12 0 12 1 4 {
            x 0 4 0 4 1 0 {}
            y 4 4 4 4 1 0 {}
            z 8 4 8 4 1 0 {}
        }
        color 12 16 16 16 1 0 {
            r 0 4 0 4 1 0 {}
            g 4 4 4 4 1 0 {}
            b 8 4 8 4 1 0 {}
            a 12 4 12 4 1 0 {}
        }
        ambiantPower 28 4 32 4 1 0 {}
    }
}


## ATTRIBUTES DEFINITION ##
modelInformations_Type modelInformations 140 140 {
    modelMatrix 0 64 0 64 1 0 {}
    inverseModelMatrix 64 64 64 64 1 0 {}
    modelMaterial 128 12 128 12 1 4 {
        hasDiffuseTexture 0 4 0 4 1 0 {}
        hasSpecularTexture 4 4 4 4 1 0 {}
        shininess 8 4 8 4 1 0 {}
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
layout (location = 1) out vec3 fragmentPosition;
layout (location = 2) out vec2 fragmentUVs;
layout (location = 3) out vec3 fragmentNormal;

struct spk_Material
{
    bool hasDiffuseTexture;
    bool hasSpecularTexture;
    float shininess;
};

struct spk_DirectionalLight
{
    vec3 direction;
    vec4 color;
    float ambiantPower;
};

layout(attributes) uniform modelInformations_Type
{
    mat4 modelMatrix;
    mat4 inverseModelMatrix;
    spk_Material modelMaterial;
} modelInformations;

layout(constants) uniform spk_CameraConstants_Type
{
    vec3 position;
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
    vec4 worldNormal = modelInformations.modelMatrix * (vec4(modelNormals, 0));
    fragmentPosition = worldPosition.xyz;
    fragmentUVs = modelUVs;
    fragmentNormal = worldNormal.xyz;
    out_instanceID = gl_InstanceID;
}

## FRAGMENT SHADER CODE ##
#version 450

layout (location = 0) in flat int instanceID;
layout (location = 1) in vec3 fragmentPosition;
layout (location = 2) in vec2 fragmentUVs;
layout (location = 3) in vec3 fragmentNormal;
layout (location = 0) out vec4 pixelColor;

struct spk_Material
{
    bool hasDiffuseTexture;
    bool hasSpecularTexture;
    float shininess;
};

struct spk_DirectionalLight
{
    vec3 direction;
    vec4 color;
    float ambiantPower;
};

layout(constants) uniform spk_CameraConstants_Type
{
    vec3 position;
    mat4 view;
    mat4 projection;
} spk_CameraConstants;

layout(attributes) uniform modelInformations_Type
{
    mat4 modelMatrix;
    mat4 inverseModelMatrix;
    spk_Material modelMaterial;
} modelInformations;

layout(constants) uniform WorldConstants_Type
{
    spk_DirectionalLight directionalLight;
} WorldConstants;

uniform sampler2D Texture_diffuseTexture;

uniform sampler2D Texture_specularTexture;

void main()
{
	// Normalize and invert the light direction so it points from the light towards the fragment
    vec3 lightDir = normalize(WorldConstants.directionalLight.direction);

    // Ambient component
    vec3 ambient = WorldConstants.directionalLight.color.rgb * WorldConstants.directionalLight.ambiantPower;

    // Diffuse component
    vec3 norm = normalize(fragmentNormal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * WorldConstants.directionalLight.color.rgb;

    // Specular component
	float specularStrength = 1.0f;
	if (modelInformations.modelMaterial.hasSpecularTexture == true)
	{
		specularStrength = texture(Texture_specularTexture, fragmentUVs).r;
	}
    vec3 viewDir = normalize(spk_CameraConstants.position - fragmentPosition);
    vec3 reflectDir = reflect(lightDir, norm);
    float specFactor = pow(max(dot(viewDir, reflectDir), 0.0), modelInformations.modelMaterial.shininess);
    vec3 specular = specularStrength * specFactor * WorldConstants.directionalLight.color.rgb * diff;

	//Merging color
    vec3 result = (ambient + specular + diffuse);
	if (modelInformations.modelMaterial.hasDiffuseTexture == true)
	{
		result *= texture(Texture_diffuseTexture, fragmentUVs).rgb;
	}

    pixelColor = vec4(result, 1.0);
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
			_needMeshUpload = true;
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

		_object.render();
	}

	void MeshRenderer::awake()
	{
		_onTransformEditionContract = owner()->transform().addOnEditionCallback([&](){
			_modelMatrixElement = owner()->transform().model();
			_modelInverseMatrixElement = owner()->transform().inverseModel();
			_modelInformations.validate();
		});
		_onTransformEditionContract.trigger();
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

		_modelMaterialElement[L"hasDiffuseTexture"] = static_cast<int>(_material->diffuse() != nullptr);
		_modelMaterialElement[L"hasSpecularTexture"] = static_cast<int>(_material->specular() != nullptr);
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