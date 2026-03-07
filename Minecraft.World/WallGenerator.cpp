#include "stdafx.h"
#include "WallGenerator.h"
#include "AABB.h"

WallGenerator::Wall::Wall(AABB *boundaries, bool unk1, bool unk2)
{
	m_boundaries = boundaries;
	m_unk8 = unk1;
	m_unk9 = unk2;
}

WallGenerator::Wall::~Wall()
{
}

WallGenerator::WallGenerator(Random random, vector<AABB *> *walls, vector<AABB *> *areas) : MapGenerator(random)
{
	m_unkAabb = NULL;
}

WallGenerator::~WallGenerator()
{
	for(auto it = m_walls.begin(); it != m_walls.end(); it++)
	{
		delete *it;
	}
}

void WallGenerator::StartGeneration(Level *level)
{
}

const AABB *WallGenerator::GetSpawnArea() const
{
	if(m_wallsAreas.size() == 0)
		return NULL;

	return *m_wallsAreas.begin();
}

void WallGenerator::CleanUp(Level *level)
{
}

const AABB *WallGenerator::GetLayerBB(unsigned int layer)
{
	return NULL;
}

unsigned int WallGenerator::GetNumberOfLayers()
{
	return 0;
}

void WallGenerator::GetLayerExtents(unsigned int layer, float *min, float *max)
{
}
