#pragma once

#include <Engine/Graphics/Vertex.h>
#include <Engine/Graphics/Particle.h>
#include <cstddef>
#include <vector>
#include <fstream>
#include <optional>
#include <array>
#include <glm/glm.hpp>

static bool ParseOBJ(const std::string& filename, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices,glm::vec3 color ,bool flipAxisAndWinding = true)
{
	std::ifstream file(filename);
	if (!file)
		return false;

	std::vector<glm::vec3> positions{};
	std::vector<glm::vec3> normals{};
	std::vector<glm::vec2> UVs{};

	vertices.clear();
	indices.clear();

	std::string sCommand;
	while (!file.eof())
	{
		file >> sCommand;

		if (sCommand == "#")
		{
			// Ignore Comment
		}
		else if (sCommand == "v")
		{

			float x, y, z;
			file >> x >> y >> z;

			positions.emplace_back(x, y, z);
		}
		else if (sCommand == "vt")
		{
			// Vertex TexCoord
			float u, v;
			file >> u >> v;
			UVs.emplace_back(u, 1 - v);
		}
		else if (sCommand == "vn")
		{
			// Vertex Normal
			float x, y, z;
			file >> x >> y >> z;

			normals.emplace_back(x, y, z);
		}
		else if (sCommand == "f")
		{
			Vertex vertex{};
			size_t iPosition, iTexCoord, iNormal;

			uint16_t tempIndices[3];
			for (size_t iFace = 0; iFace < 3; iFace++)
			{
				file >> iPosition;
	
				vertex.pos = positions[iPosition - 1];

				if ('/' == file.peek())
				{
					file.ignore();

					if ('/' != file.peek())
					{
						file >> iTexCoord;
					}

					if ('/' == file.peek())
					{
						file.ignore();

						file >> iNormal;
						vertex.normal = normals[iNormal - 1];
					}
				}

				vertex.color = color;
				vertices.push_back(vertex);
				tempIndices[iFace] = uint16_t(vertices.size()) - 1;
			}

			indices.push_back(tempIndices[0]);
			if (flipAxisAndWinding)
			{
				indices.push_back(tempIndices[2]);
				indices.push_back(tempIndices[1]);
			}
			else
			{
				indices.push_back(tempIndices[1]);
				indices.push_back(tempIndices[2]);
			}
		}
		file.ignore(1000, '\n');
	}
}