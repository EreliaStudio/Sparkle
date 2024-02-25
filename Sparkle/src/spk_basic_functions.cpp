#include "spk_basic_functions.hpp"
#include <chrono>
#include <fstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace spk
{
    std::string readFileContent(const std::filesystem::path& p_filePath)
    {
        std::ifstream fileStream(p_filePath);
        if (fileStream.is_open() == false)
            throw std::runtime_error("Failed to open file: " + p_filePath.string());

        std::stringstream buffer;
        buffer << fileStream.rdbuf();
        return buffer.str();
    }

    float degreeToRadian(float p_degrees)
    {
        return static_cast<float>(p_degrees * (M_PI / 180.0f));
    }

    float radianToDegree(float p_radians)
    {
        return static_cast<float>(p_radians * (180.0f / M_PI));
    }

    std::string to_string(const GLenum& p_type)
    {
        switch (p_type)
        {
        case GL_NO_ERROR:
            return ("No error");
        case GL_INVALID_ENUM:
            return ("Invalid enum");
        case GL_INVALID_VALUE:
            return ("Invalid value");
        case GL_INVALID_OPERATION:
            return ("Invalid operation");
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return ("Invalid framebuffer operation");
        case GL_OUT_OF_MEMORY:
            return ("Out of memory");
        case GL_STACK_UNDERFLOW:
            return ("Stack underflow");
        case GL_STACK_OVERFLOW:
            return ("Stack overflow");

        case GL_BYTE:
            return ("GL_BYTE");
        case GL_UNSIGNED_BYTE:
            return ("GL_UNSIGNED_BYTE");
        case GL_UNSIGNED_SHORT:
            return ("GL_UNSIGNED_SHORT");
        case GL_2_BYTES:
            return ("GL_2_BYTES");
        case GL_3_BYTES:
            return ("GL_3_BYTES");
        case GL_4_BYTES:
            return ("GL_4_BYTES");

        case GL_FLOAT:
            return ("float");
        case GL_FLOAT_VEC2:
            return ("vec2");
        case GL_FLOAT_VEC3:
            return ("vec3");
        case GL_FLOAT_VEC4:
            return ("vec4");
        case GL_DOUBLE:
            return ("double");
        case GL_DOUBLE_VEC2:
            return ("dvec2");
        case GL_DOUBLE_VEC3:
            return ("dvec3");
        case GL_DOUBLE_VEC4:
            return ("dvec4");
        case GL_INT:
            return ("int");
        case GL_INT_VEC2:
            return ("ivec2");
        case GL_INT_VEC3:
            return ("ivec3");
        case GL_INT_VEC4:
            return ("ivec4");
        case GL_UNSIGNED_INT:
            return ("unsigned int");
        case GL_UNSIGNED_INT_VEC2:
            return ("uvec2");
        case GL_UNSIGNED_INT_VEC3:
            return ("uvec3");
        case GL_UNSIGNED_INT_VEC4:
            return ("uvec4");
        case GL_BOOL:
            return ("bool");
        case GL_BOOL_VEC2:
            return ("bvec2");
        case GL_BOOL_VEC3:
            return ("bvec3");
        case GL_BOOL_VEC4:
            return ("bvec4");
        case GL_FLOAT_MAT2:
            return ("mat2");
        case GL_FLOAT_MAT3:
            return ("mat3");
        case GL_FLOAT_MAT4:
            return ("mat4");
        case GL_FLOAT_MAT2x3:
            return ("mat2x3");
        case GL_FLOAT_MAT2x4:
            return ("mat2x4");
        case GL_FLOAT_MAT3x2:
            return ("mat3x2");
        case GL_FLOAT_MAT3x4:
            return ("mat3x4");
        case GL_FLOAT_MAT4x2:
            return ("mat4x2");
        case GL_FLOAT_MAT4x3:
            return ("mat4x3");
        case GL_DOUBLE_MAT2:
            return ("dmat2");
        case GL_DOUBLE_MAT3:
            return ("dmat3");
        case GL_DOUBLE_MAT4:
            return ("dmat4");
        case GL_DOUBLE_MAT2x3:
            return ("dmat2x3");
        case GL_DOUBLE_MAT2x4:
            return ("dmat2x4");
        case GL_DOUBLE_MAT3x2:
            return ("dmat3x2");
        case GL_DOUBLE_MAT3x4:
            return ("dmat3x4");
        case GL_DOUBLE_MAT4x2:
            return ("dmat4x2");
        case GL_DOUBLE_MAT4x3:
            return ("dmat4x3");
        case GL_SAMPLER_1D:
            return ("sampler1D");
        case GL_SAMPLER_2D:
            return ("sampler2D");
        case GL_SAMPLER_3D:
            return ("sampler3D");
        case GL_SAMPLER_CUBE:
            return ("samplerCube");
        case GL_SAMPLER_1D_SHADOW:
            return ("sampler1DShadow");
        case GL_SAMPLER_2D_SHADOW:
            return ("sampler2DShadow");
        case GL_SAMPLER_1D_ARRAY:
            return ("sampler1DArray");
        case GL_SAMPLER_2D_ARRAY:
            return ("sampler2DArray");
        case GL_SAMPLER_1D_ARRAY_SHADOW:
            return ("sampler1DArrayShadow");
        case GL_SAMPLER_2D_ARRAY_SHADOW:
            return ("sampler2DArrayShadow");
        case GL_SAMPLER_2D_MULTISAMPLE:
            return ("sampler2DMS");
        case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
            return ("sampler2DMSArray");
        case GL_SAMPLER_CUBE_SHADOW:
            return ("samplerCubeShadow");
        case GL_SAMPLER_BUFFER:
            return ("samplerBuffer");
        case GL_SAMPLER_2D_RECT:
            return ("sampler2DRect");
        case GL_SAMPLER_2D_RECT_SHADOW:
            return ("sampler2DRectShadow");
        case GL_INT_SAMPLER_1D:
            return ("isampler1D");
        case GL_INT_SAMPLER_2D:
            return ("isampler2D");
        case GL_INT_SAMPLER_3D:
            return ("isampler3D");
        case GL_INT_SAMPLER_CUBE:
            return ("isamplerCube");
        case GL_INT_SAMPLER_1D_ARRAY:
            return ("isampler1DArray");
        case GL_INT_SAMPLER_2D_ARRAY:
            return ("isampler2DArray");
        case GL_INT_SAMPLER_2D_MULTISAMPLE:
            return ("isampler2DMS");
        case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
            return ("isampler2DMSArray");
        case GL_INT_SAMPLER_BUFFER:
            return ("isamplerBuffer");
        case GL_INT_SAMPLER_2D_RECT:
            return ("isampler2DRect");
        case GL_UNSIGNED_INT_SAMPLER_1D:
            return ("usampler1D");
        case GL_UNSIGNED_INT_SAMPLER_2D:
            return ("usampler2D");
        case GL_UNSIGNED_INT_SAMPLER_3D:
            return ("usampler3D");
        case GL_UNSIGNED_INT_SAMPLER_CUBE:
            return ("usamplerCube");
        case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
            return ("usampler2DArray");
        case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
            return ("usampler2DArray");
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
            return ("usampler2DMS");
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
            return ("usampler2DMSArray");
        case GL_UNSIGNED_INT_SAMPLER_BUFFER:
            return ("usamplerBuffer");
        case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
            return ("usampler2DRect");
        case GL_IMAGE_1D:
            return ("image1D");
        case GL_IMAGE_2D:
            return ("image2D");
        case GL_IMAGE_3D:
            return ("image3D");
        case GL_IMAGE_2D_RECT:
            return ("image2DRect");
        case GL_IMAGE_CUBE:
            return ("imageCube");
        case GL_IMAGE_BUFFER:
            return ("imageBuffer");
        case GL_IMAGE_1D_ARRAY:
            return ("image1DArray");
        case GL_IMAGE_2D_ARRAY:
            return ("image2DArray");
        case GL_IMAGE_2D_MULTISAMPLE:
            return ("image2DMS");
        case GL_IMAGE_2D_MULTISAMPLE_ARRAY:
            return ("image2DMSArray");
        case GL_INT_IMAGE_1D:
            return ("iimage1D");
        case GL_INT_IMAGE_2D:
            return ("iimage2D");
        case GL_INT_IMAGE_3D:
            return ("iimage3D");
        case GL_INT_IMAGE_2D_RECT:
            return ("iimage2DRect");
        case GL_INT_IMAGE_CUBE:
            return ("iimageCube");
        case GL_INT_IMAGE_BUFFER:
            return ("iimageBuffer");
        case GL_INT_IMAGE_1D_ARRAY:
            return ("iimage1DArray");
        case GL_INT_IMAGE_2D_ARRAY:
            return ("iimage2DArray");
        case GL_INT_IMAGE_2D_MULTISAMPLE:
            return ("iimage2DMS");
        case GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
            return ("iimage2DMSArray");
        case GL_UNSIGNED_INT_IMAGE_1D:
            return ("uimage1D");
        case GL_UNSIGNED_INT_IMAGE_2D:
            return ("uimage2D");
        case GL_UNSIGNED_INT_IMAGE_3D:
            return ("uimage3D");
        case GL_UNSIGNED_INT_IMAGE_2D_RECT:
            return ("uimage2DRect");
        case GL_UNSIGNED_INT_IMAGE_CUBE:
            return ("uimageCube");
        case GL_UNSIGNED_INT_IMAGE_BUFFER:
            return ("uimageBuffer");
        case GL_UNSIGNED_INT_IMAGE_1D_ARRAY:
            return ("uimage1DArray");
        case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:
            return ("uimage2DArray");
        case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE:
            return ("uimage2DMS");
        case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
            return ("uimage2DMSArray");
        case GL_UNSIGNED_INT_ATOMIC_COUNTER:
            return ("atomic_uint");

        case GL_ARRAY_BUFFER:
            return ("Vertex attributes");
        case GL_ATOMIC_COUNTER_BUFFER:
            return ("Atomic counter storage");
        case GL_COPY_READ_BUFFER:
            return ("Buffer copy source");
        case GL_COPY_WRITE_BUFFER:
            return ("Buffer copy destination");
        case GL_DISPATCH_INDIRECT_BUFFER:
            return ("Indirect compute dispatch commands");
        case GL_DRAW_INDIRECT_BUFFER:
            return ("Indirect command arguments");
        case GL_ELEMENT_ARRAY_BUFFER:
            return ("Vertex array indices");
        case GL_PIXEL_PACK_BUFFER:
            return ("Pixel read target");
        case GL_PIXEL_UNPACK_BUFFER:
            return ("Texture data source");
        case GL_QUERY_BUFFER:
            return ("Query result buffer");
        case GL_SHADER_STORAGE_BUFFER:
            return ("Read-write storage for shaders");
        case GL_TEXTURE_BUFFER:
            return ("Texture data buffer");
        case GL_TRANSFORM_FEEDBACK_BUFFER:
            return ("Transform feedback buffer");
        case GL_UNIFORM_BUFFER:
            return ("Uniform block storage");

        case GL_TEXTURE_2D:
            return ("GL_TEXTURE_2D");
        case GL_RGB:
            return ("GL_RGB");
        case GL_RGBA:
            return ("GL_RGBA");
        case GL_BGR:
            return ("GL_BGR");
        case GL_BGRA:
            return ("GL_BGRA");
        case GL_RED:
            return ("GL_RED");

        default:
            return ("not a GLenum type");
        }
    }

    void checkOpengl(const std::string& msg)
    {
        GLenum err = glGetError();
        if (err != GL_NO_ERROR)
        {
            std::cout << msg << " : Error type [" << to_string(err) << "]" << std::endl;
            exit(1);
        }
        else
        {
            std::cout << msg << std::endl;
        }
    }

    void throwException(const std::string& p_errorMessage)
    {
        std::cout << "Error thrown : " << p_errorMessage << std::endl;
        throw std::runtime_error(p_errorMessage);
    }

    long long getTime()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
}