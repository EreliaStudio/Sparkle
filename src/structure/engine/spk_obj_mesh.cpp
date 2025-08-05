#include "structure/engine/spk_obj_mesh.hpp"

#include <array>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace spk
{
	namespace
	{
		static Vertex parseVertex(const std::string &p_token,
								  const std::vector<spk::Vector3> &p_positions,
								  const std::vector<spk::Vector2> &p_uvs,
								  const std::vector<spk::Vector3> &p_normals)
		{
			Vertex result;
			std::array<std::string, 3> parts = {"", "", ""};
			size_t start = 0;
			size_t index = 0;
			for (size_t i = 0; i <= p_token.size() && index < parts.size(); ++i)
			{
				if (i == p_token.size() || p_token[i] == '/')
				{
					parts[index] = p_token.substr(start, i - start);
					++index;
					start = i + 1;
				}
			}

			int posIndex = parts[0].empty() == true ? 0 : std::stoi(parts[0]);
			int uvIndex = parts[1].empty() == true ? 0 : std::stoi(parts[1]);
			int normIndex = parts[2].empty() == true ? 0 : std::stoi(parts[2]);

			if (posIndex > 0 && posIndex <= static_cast<int>(p_positions.size()))
			{
				result.position = p_positions[static_cast<std::size_t>(posIndex - 1)];
			}
			if (uvIndex > 0 && uvIndex <= static_cast<int>(p_uvs.size()))
			{
				result.uv = p_uvs[static_cast<std::size_t>(uvIndex - 1)];
			}
			if (normIndex > 0 && normIndex <= static_cast<int>(p_normals.size()))
			{
				result.normal = p_normals[static_cast<std::size_t>(normIndex - 1)];
			}

			return result;
		}
	} // namespace

	void ObjMesh::loadFromString(const std::string &p_input)
	{
		clear();

		std::vector<spk::Vector3> positions;
		std::vector<spk::Vector2> uvs;
		std::vector<spk::Vector3> normals;

		std::istringstream stream(p_input);
		std::string line;
		while (true)
		{
			std::getline(stream, line);
			if (stream.fail() == true)
			{
				break;
			}

			std::istringstream lineStream(line);
			std::string prefix;
			lineStream >> prefix;
			if (lineStream.fail() == true)
			{
				continue;
			}

			if (prefix == "v")
			{
				float x = 0.0f;
				float y = 0.0f;
				float z = 0.0f;
				lineStream >> x >> y >> z;
				positions.emplace_back(x, y, z);
			}
			else if (prefix == "vt")
			{
				float u = 0.0f;
				float v = 0.0f;
				lineStream >> u >> v;
				uvs.emplace_back(u, v);
			}
			else if (prefix == "vn")
			{
				float x = 0.0f;
				float y = 0.0f;
				float z = 0.0f;
				lineStream >> x >> y >> z;
				normals.emplace_back(x, y, z);
			}
			else if (prefix == "f")
			{
				std::vector<Vertex> verts;
				std::string token;
				while (true)
				{
					lineStream >> token;
					if (lineStream.fail() == true)
					{
						break;
					}
					verts.push_back(parseVertex(token, positions, uvs, normals));
				}

				if (verts.size() == 3)
				{
					addShape(verts[0], verts[1], verts[2]);
				}
				else if (verts.size() == 4)
				{
					addShape(verts[0], verts[1], verts[2], verts[3]);
				}
			}
		}
	}

	void ObjMesh::loadFromFile(const std::filesystem::path &p_path)
	{
		std::ifstream file(p_path);
		if (file.is_open() == false)
		{
			return;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		loadFromString(buffer.str());
	}

	void ObjMesh::exportToFile(const std::filesystem::path &p_path) const
	{
		std::ofstream file(p_path);
		if (file.is_open() == false)
		{
			return;
		}

		std::vector<spk::Vector3> positions;
		std::vector<spk::Vector2> uvs;
		std::vector<spk::Vector3> normals;
		std::unordered_map<spk::Vector3, unsigned int> positionMap;
		std::unordered_map<spk::Vector2, unsigned int> uvMap;
		std::unordered_map<spk::Vector3, unsigned int> normalMap;

		struct Index
		{
			unsigned int position = 0;
			unsigned int uv = 0;
			unsigned int normal = 0;
		};
		std::vector<std::vector<Index>> faces;

		const auto &shapeList = shapes();
		for (const auto &shape : shapeList)
		{
			std::vector<Index> face;

			auto processVertex = [&](const Vertex &p_vertex) -> Index
			{
				Index idx;
				auto posIt = positionMap.find(p_vertex.position);
				if (posIt == positionMap.end())
				{
					idx.position = static_cast<unsigned int>(positions.size());
					positions.push_back(p_vertex.position);
					positionMap[p_vertex.position] = idx.position;
				}
				else
				{
					idx.position = posIt->second;
				}

				auto uvIt = uvMap.find(p_vertex.uv);
				if (uvIt == uvMap.end())
				{
					idx.uv = static_cast<unsigned int>(uvs.size());
					uvs.push_back(p_vertex.uv);
					uvMap[p_vertex.uv] = idx.uv;
				}
				else
				{
					idx.uv = uvIt->second;
				}

				auto normIt = normalMap.find(p_vertex.normal);
				if (normIt == normalMap.end())
				{
					idx.normal = static_cast<unsigned int>(normals.size());
					normals.push_back(p_vertex.normal);
					normalMap[p_vertex.normal] = idx.normal;
				}
				else
				{
					idx.normal = normIt->second;
				}

				return idx;
			};

			if (std::holds_alternative<Triangle>(shape) == true)
			{
				const auto &t = std::get<Triangle>(shape);
				face.push_back(processVertex(t.a));
				face.push_back(processVertex(t.b));
				face.push_back(processVertex(t.c));
			}
			else
			{
				const auto &q = std::get<Quad>(shape);
				face.push_back(processVertex(q.a));
				face.push_back(processVertex(q.b));
				face.push_back(processVertex(q.c));
				face.push_back(processVertex(q.d));
			}

			faces.push_back(face);
		}

		for (const auto &p : positions)
		{
			file << "v " << p.x << " " << p.y << " " << p.z << "\n";
		}
		for (const auto &t : uvs)
		{
			file << "vt " << t.x << " " << t.y << "\n";
		}
		for (const auto &n : normals)
		{
			file << "vn " << n.x << " " << n.y << " " << n.z << "\n";
		}
		for (const auto &face : faces)
		{
			file << "f";
			for (const auto &idx : face)
			{
				file << " " << idx.position + 1 << "/" << idx.uv + 1 << "/" << idx.normal + 1;
			}
			file << "\n";
		}
	}
}