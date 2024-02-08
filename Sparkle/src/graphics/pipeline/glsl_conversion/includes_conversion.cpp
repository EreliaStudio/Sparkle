#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk
{
    

	int extractVersion(const std::string &p_source)
	{
		std::regex versionRegex(R"(#version\s+(\d+))");
		std::smatch matches;
		if (std::regex_search(p_source, matches, versionRegex))
		{
			return std::stoi(matches[1].str());
		}
		return 0;
	}

	std::string getIncludeContent(const std::string& includeKey)
	{
		const std::unordered_map<std::string, std::string> sparkleNativeIncludes = {
			{"widgetAttribute", "AttributeBlock widgetAttribute { float depth; };"},
			{"screenConstants", "ConstantBlock screenConstants { mat4 canvasMVP; };"},
			{"transform", R"(struct Transform { vec3 translation; vec3 scale; vec3 rotation; };)"},
			{"transformUtils", R"(#include <transform>

				mat4 createRotationMatrix(vec3 angles)
				{
					float c1 = cos(radians(angles.x));
					float s1 = sin(radians(angles.x));
					float c2 = cos(radians(angles.y));
					float s2 = sin(radians(angles.y));
					float c3 = cos(radians(angles.z));
					float s3 = sin(radians(angles.z));

					mat4 rotationX = mat4(	1.0f, 0.0f, 0.0f, 0.0f,
											0.0f,   c1,   s1, 0.0f,
											0.0f,  -s1,   c1, 0.0f,
											0.0f, 0.0f, 0.0f, 1.0f);

					mat4 rotationY = mat4(    c2, 0.0f,  -s2, 0.0f,
											0.0f, 1.0f, 0.0f, 0.0f,
											s2, 0.0f,   c2, 0.0f,
											0.0f, 0.0f, 0.0f, 1.0f);

					mat4 rotationZ = mat4(    c3,   s3, 0.0f, 0.0f,
											-s3,   c3, 0.0f, 0.0f,
											0.0f, 0.0f, 1.0f, 0.0f,
											0.0f, 0.0f, 0.0f, 1.0f);

					return rotationZ * rotationY * rotationX;
				}

				vec3 applyTransform(vec3 p_position, Transform p_transform)
				{
					mat4 rotationMatrix = createRotationMatrix(p_transform.rotation);
					vec3 rotatedPosition = (rotationMatrix * vec4(p_position * p_transform.scale, 1.0)).xyz;
					return (p_transform.translation + rotatedPosition);
				})"},
			{"cameraConstants", R"(ConstantBlock mainCamera { mat4 MVP; };)"},
			{"timeConstants", R"(ConstantBlock timeConstants { int epoch; };)"},
			{"spriteAnimation", R"(struct SpriteAnimation { int duration; ivec2 firstFrame; ivec2 frameOffset; int lenght; };)"}
		};

		auto found = sparkleNativeIncludes.find(includeKey);
		if (found != sparkleNativeIncludes.end())
		{
			return found->second;
		}
		else
		{
			try
			{
				return spk::readFileContent(includeKey);
			}
			catch (const std::exception& e)
			{
				throwException("Failed to load include file [" + includeKey + "]: " + e.what());
			}
		}
		return ("");
	}

	void applyRegex(std::vector<std::string>& p_includePath, std::regex& p_versionRegex, int& p_highestVersion, 
					const std::regex& p_includeRegex, std::string& p_source)
	{
		std::smatch matches;

		while (std::regex_search(p_source, matches, p_includeRegex))
		{
			std::string includeKey = matches[1].str();
			
			if (std::find(p_includePath.begin(), p_includePath.end(), includeKey) != p_includePath.end())
			{
				throwException("Circular include detected: " + includeKey);
			}

			std::string newSource = getIncludeContent(includeKey) + "\n";
			
			p_includePath.push_back(includeKey);

			int version = extractVersion(newSource);
			if (version > p_highestVersion)
			{
				p_highestVersion = version;
			}

			newSource = std::regex_replace(newSource, p_versionRegex, "");

			applyRegex(p_includePath, p_versionRegex, p_highestVersion, p_includeRegex, newSource);
			p_source.replace(matches.position(0), matches.length(0), newSource + "\n\n");

			p_includePath.pop_back();
		}
	}

	std::string executeIncludeLines(const std::string& p_source)
	{
		std::string code = p_source;

		std::vector<std::string> includePath;
		std::regex versionRegex(R"(#version\s+\d+\s*\n)");
		std::regex includeDoubleQuoteRegex(R"(#include\s*\"([^\"]+)\"(\s*[\n\0]|$))");
    	std::regex includeBracketRegex(R"(#include\s*<([^>]+)>(\s*[\n\0]|$))");
		int highestVersion = extractVersion(code);

		code = std::regex_replace(code, versionRegex, "");

		applyRegex(includePath, versionRegex, highestVersion, includeDoubleQuoteRegex, code);

		applyRegex(includePath, versionRegex, highestVersion, includeBracketRegex, code);

		return (std::move("#version " + std::to_string(highestVersion) + "\n" + code));
	}
}