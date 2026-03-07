#include "stdafx.h"
#include "TumbleGenerator.h"

TumbleGenerator::TumbleGenerator(Random random, vector<LayerGenerator *> &layers) : MapGenerator(random)
{
}

TumbleGenerator::~TumbleGenerator()
{
}

void TumbleGenerator::StartGeneration(Level *level)
{
}

const AABB *TumbleGenerator::GetSpawnArea() const
{
	return NULL;
}

void TumbleGenerator::CleanUp(Level *level)
{
}

const AABB *TumbleGenerator::GetLayerBB(unsigned int layer)
{
	return NULL;
}

unsigned int TumbleGenerator::GetNumberOfLayers()
{
	return 0;
}

void TumbleGenerator::GetLayerExtents(unsigned int layer, float *min, float *max)
{
}

int TumbleGenerator::GetBlockCountOnLayer(unsigned int layer)
{
	return 0;
}
