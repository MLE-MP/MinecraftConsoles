#pragma once

#include "MapGenerator.h"
#include <vector>

class WallGenerator : public MapGenerator
{
public:
	class Wall
	{
	public:
		Wall(AABB *boundaries, bool unk1, bool unk2);
		~Wall();

		AABB *m_boundaries;
		bool m_unk8;
		bool m_unk9;
	};

	WallGenerator(Random random, vector<AABB *> *walls, vector<AABB *> *areas);

	~WallGenerator();
	void StartGeneration(Level *level);
	const AABB *GetSpawnArea() const;
	void CleanUp(Level *level);
	const AABB *GetLayerBB(unsigned int layer);
	unsigned int GetNumberOfLayers();
	void GetLayerExtents(unsigned int layer, float *min, float *max);

	vector<AABB *> m_wallsAreas;
	vector<Wall *> m_walls;
	AABB *m_unkAabb;
};
