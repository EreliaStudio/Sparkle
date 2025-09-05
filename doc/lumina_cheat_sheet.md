# Lumina Language Cheat Sheet

## General Overview
Lumina is a wrapper around GLSL behavior, designed to simplify the creation of shader programs. It introduces a structured syntax for defining inputs, outputs, structures, uniform blocks, and shader stages.

## Syntax Elements

### Import
- Import predefined section of code or user-provided files using the precompile instruction "#include".
```cpp
#include "<predefinedInclude>"
#include "shader/userProvidedInclude.lum"
```

### Native types
- Scalar types :
    - int : equivalent of the int32_t of C++.
    - uint : equivalent of the uint32_t of C++.
    - float : equivalent of the float32_t of C++.
    - bool : a boolean.

- Vectors :
    - Vector2, Vector2Int and Vector2UInt : representation of a 2D point using float, int and uint.
    - Vector3, Vector3Int and Vector3UInt : representation of a 3D point using float, int and uint.
    - Vector4, Vector4Int and Vector4UInt : representation of a 4D point using float, int and uint.

- Matrix :
    - Matrix2x2 : a matrix of 4 float.
    - Matrix3x3 : a matrix of 9 float.
    - Matrix4x4 : a matrix of 16 float.

- Lumina types :
    - Color : a 4 float representation of a color.
    - Texture : a representation of a 2D texture, with a method "getPixel(Vector2 UV)" to retreive a Color.

### Pipeline Flow
- Define data flow between shader stages.
- Available pipeline flow :
    - Input -> VertexPass
    - VertexPass -> FragmentPass
    - FragmentPass -> Output
- Data passed thought flow must remain native data, such as Vector2, Vector2Int, float, int, etc
```cpp
Input -> VertexPass: float variableNameA;
VertexPass -> FragmentPass: Vector3 variableNameB;
FragmentPass -> Ouput: Vector4 variableNameC;
```


### Functions
- Define functions with return types and input parameters.
```cpp
returnType functionName(type inputVariableName)
{
    returnType result;
    return result;
}
```
- Example:
```cpp
Vector4 myFunction(float inputVariable)
{
    Vector4 result = Vector4(inputVariable);
    return result;
}
```

### Exception management
- Define a specific function `raiseException` that will allow the shader to indicate errors to the CPU
- Exemple:
```cpp
raiseException("Invalid UV value : %v", myUV);
```
- List of possible format :
	- %d : int
	- %u : uint
	- %d : float
	- %v2 : Vector2
	- %v3 : Vector3
	- %v4 : Vector4
	- %c : Color
	- %m2 : Matrix2x2
	- %m3 : Matrix3x3
	- %m4 : Matrix4x4

### Structures
- Define custom structures to group variables.
- Much like in C++, structures can contain methods, constructors and operators.
- Inside those methods, constructors and operators, you can use the "this" keyword to refer to the current instance, but the compiler will be able to deduce its presence if you didn't place it (Like in C++)
```cpp
struct StructName
{
    float variableA;
    Vector3 variableB;
    Vector4 variableC;

	Type method(ParamTypeA valueA, ParamTypeB& valueBAsReference)
    {
        variableA += valueA;
        variableB += this.variableB;
    }

    // Operator definition inside a structure
    Type operator + (TypeB other)
    {
        
    }
};
```

### Attribute Blocks
- Define uniform blocks for information shared by every triangle in a single call.
- Attributes are shared by both the Vertex and the Fragment passes.
```cpp
AttributeBlock attributeBlockName
{
    int variableName;
};
```
- AttributeBlock share the same principle as structures for defining methods and operator overloads.

### Constant Blocks
- Define uniform blocks for information shared across all calls of all lumina pipeline.
- Constants are shared by both the Vertex and the Fragment passes.
```cpp
ConstantBlock constantBlockName
{
    float variableName;
};
```
- ConstantBlock share the same principle as structures for defining methods and operator overloads.

### Texture variable
- Define 2D texture object
- Textures can't be placed inside a structure block, nor in an Attribute or Constant
```cpp
Texture myTexture;
```
- You can then access the pixel stored inside the texture using the method `getPixel(Vector2)`
```cpp
FragmentPass()
{
    pixelColor = getPixel(myTexture, UV);
}
```

### Namespaces
- Create namespaces to scope functions, structures, constants block, attributes block and texture.
```cpp
namespace NamespaceName
{
    struct NestedStruct
    {
        float value;
    };

    Vector4 namespaceFunction(float input)
    {
        Vector4 result = Vector4(input);
        return result;
    }

    namespace NestedNamespace
    {
        struct InnerStruct
        {
            int innerValue;
        };

        int nestedFunction(int input)
        {
            return input * 2;
        }
    }
}
```
- Usage: 
  - `NamespaceName::NestedStruct`
  - `NamespaceName::namespaceFunction`
  - `NamespaceName::NestedNamespace::InnerStruct`
  - `NamespaceName::NestedNamespace::nestedFunction`
  
### Commenting

Lumina supports both single-line and multi-line comments for documenting your code, explaining complex logic, or temporarily disabling code.

#### Single-Line Comments
- Use `//` for single-line comments. Everything following `//` on the same line is considered a comment.

```cpp
// This is a single-line comment
Vector3 color = Vector3(1.0, 0.0, 0.0); // This comment is on the same line as code
```

#### Multi-Line Comments
- Use `/*` to start a multi-line comment and `*/` to end it. This allows you to comment out multiple lines of code or write longer explanations.

```cpp
/*
This is a multi-line comment.
It spans multiple lines.
Useful for detailed explanations or
temporarily disabling large blocks of code.
*/
Vector3 color = Vector3(1.0, 0.0, 0.0); /* Multi-line comments can also be 
                                    used to comment on parts of code */
```

### Shader Stages

#### Vertex Pass
- Define the vertex shader stage.
```cpp
VertexPass()
{
    Vector4 finalPixelPosition = Vector4(0, 0, 0, 1);
    variableNameB = Vector3(10);

    if (variableNameB.x == 5.0)
    {
        raiseException("Custom pipeline failed with variable B == %d!", variableNameB);
    }

    pixelPosition = finalPixelPosition;
}
```

#### Fragment Pass
- Define the fragment shader stage.
```cpp
FragmentPass()
{
    Vector4 finalPixelColor = Vector4(0, 1, 0, 1);
    pixelColor = finalPixelColor;
}
```

## Example Code

Here is an example of a complete Lumina pipeline code:

```cpp
// Include predefined and user-provided files
#include <screenConstants>
#include "shader/customInclude.lum"

// Define the pipeline flow
Input -> VertexPass: Vector3 vertexPosition;
Input -> VertexPass: Vector3 vertexNormal;
Input -> VertexPass: Vector2 vertexUV;
VertexPass -> FragmentPass: Vector3 fragPosition;
VertexPass -> FragmentPass: Vector3 fragNormal;
VertexPass -> FragmentPass: Vector2 fragUV;

// Define custom structures
struct Material
{
    Vector3 diffuseColor;
    Vector3 specularColor;
    float shininess;
};

// Define a texture
Texture diffuseTexture;

// Define attribute blocks
AttributeBlock modelAttributes
{
    Matrix4x4 modelMatrix;
    Matrix4x4 normalMatrix;
};

// Define constant blocks
ConstantBlock lightingConstants
{
    Vector3 lightPosition;
    Vector3 lightColor;
    float ambientIntensity;
};

// Create namespaces
namespace Lighting
{
    Vector3 calculateDiffuse(Vector3 normal, Vector3 lightDir, Vector3 lightColor)
    {
        float diff = max(dot(normal, lightDir), 0.0);
        return diff * lightColor;
    }

    Vector3 calculateSpecular(Vector3 normal, Vector3 lightDir, Vector3 viewDir, float shininess, Vector3 lightColor)
    {
        Vector3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        return spec * lightColor;
    }
}

/*
Pipeline Description:
This pipeline is designed to render a 3D object by processing its vertex data and applying lighting calculations.

Vertex Shader (VertexPass):
- Transforms the input vertex positions using a model matrix to position the object correctly in the world space.
- Transforms the normals using the normal matrix for accurate lighting calculations.
- Passes UV coordinates, transformed positions, and normals to the fragment shader.

Fragment Shader (FragmentPass):
- Performs lighting calculations using ambient, diffuse, and specular components.
- Combines these lighting components with a texture sampled from the UV coordinates.
- Discards fragments where the texture's alpha value is zero.
- Sets the final pixel color based on the combined lighting and texture information.

The vertex and fragment shaders work together to accurately render the 3D object with realistic lighting and texturing.
*/

// Define the vertex shader stage
VertexPass()
{
    Vector4 worldPosition = modelAttributes.modelMatrix * Vector4(vertexPosition, 1.0);
    fragPosition = worldPosition.xyz;
    fragNormal = mat3(modelAttributes.normalMatrix) * vertexNormal;
    fragUV = vertexUV;

    Vector4 clipSpacePosition = worldPosition;
    pixelPosition = clipSpacePosition;
}

// Define the fragment shader stage
FragmentPass()
{
    // Error handling example
    if (lightingConstants.ambientIntensity < 0.0 || lightingConstants.ambientIntensity > 1.0)
    {
        raiseException("Ambient intensity must be between 0 and 1. Current value: %d", lightingConstants.ambientIntensity);
    }

    Vector3 normal = normalize(fragNormal);
    Vector3 lightDir = normalize(lightingConstants.lightPosition - fragPosition);
    Vector3 viewDir = normalize(-fragPosition);

    Vector3 ambient = lightingConstants.ambientIntensity * lightingConstants.lightColor;

    Vector3 diffuse = Lighting::calculateDiffuse(normal, lightDir, lightingConstants.lightColor);

    Material material;
    Vector3 specular = Lighting::calculateSpecular(normal, lightDir, viewDir, material.shininess, lightingConstants.lightColor);

    Vector3 finalColor = ambient + diffuse * material.diffuseColor + specular * material.specularColor;

    Vector4 textureColor = getPixel(diffuseTexture, fragUV);
    finalColor *= textureColor.rgb;

    if (textureColor.a == 0)
    {
        discard;
    }

    pixelColor = Vector4(finalColor, textureColor.a);
}


```

This cheat sheet covers the essential elements of the Lumina language to help you create and manage shaders efficiently. Happy coding!