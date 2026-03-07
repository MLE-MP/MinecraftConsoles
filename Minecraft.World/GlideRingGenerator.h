#pragma once

#include <vector>

class AABB;
class BlockPos;

class GlideRingGenerator
{
public:
	enum eGlideRingSize {};

	static void BuildRing(vector<BlockPos> &positions, const AABB *area, eGlideRingSize size);
	static void *getIcon(eGlideRingSize size);
};
