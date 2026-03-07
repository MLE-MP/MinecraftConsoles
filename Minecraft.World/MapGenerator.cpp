#include "stdafx.h"
#include "MapGenerator.h"

MapGenerator::MapGenerator(Random random) : m_random(random)
{
}

MapGenerator::~MapGenerator()
{
}

int MapGenerator::GetBlockCountOnLayer(unsigned int layer)
{
	return 0;
}
