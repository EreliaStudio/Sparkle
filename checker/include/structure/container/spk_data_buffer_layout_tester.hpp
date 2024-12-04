#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

#include "structure/container/spk_data_buffer.hpp"
#include "structure/container/spk_data_buffer_layout.hpp"

class DataBufferLayoutTest : public ::testing::Test
{
public:
	struct Vertex
	{
		std::array<float, 3> position;
		uint32_t colors[2];
	};

protected:
	
	spk::DataBuffer buffer;
	spk::DataBufferLayout layout;

	DataBufferLayoutTest() :
		buffer(sizeof(Vertex)),
		layout(L"Vertex", &buffer)
	{
	}

	void SetUp() override
	{
		layout.setBuffer(&buffer);
		layout.updateRootSize();
	}
};
