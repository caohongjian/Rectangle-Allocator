#pragma once
#include<cstdint>


template<uint32_t LevelCount, uint32_t TotalExtent>
class TRectangleAllocator
{
public:
	template<uint32_t level>
	struct TCapacitiesCount
	{
		enum
		{
			count = 4 * TCapacitiesCount<level - 1>::count,
			size = TCapacitiesCount<level>::count + TCapacitiesCount<level - 1>::size,
		};
	};


	template<>
	struct TCapacitiesCount<1>
	{
		enum
		{
			count = 1,
			size = 1,
		};
	};


	struct KCapacity
	{
		uint16_t mask;
		void Init(uint32_t level)
		{
			mask = 0xffff;
			for (uint32_t i = 0; i < level; ++i)
			{
				mask = mask& ~(0x1 << i);
			}
		}
		bool HasCapacity(uint32_t level)const 
		{ 
			return (mask >> level) & 0x1; 
		}
		void Update(uint32_t level, const KCapacity children[4])
		{
			mask = 0;
			for (uint32_t i = 0; i < 4; ++i)
				mask |= children[i].mask;

			for (uint32_t i = 0; i < 4; ++i)
				if (!children[i].HasCapacity(level + 1))
				{
					return;
				}
			this->Enable(level);
		}
		void Enable(uint32_t level) { mask = mask | 0x1 << level; }
		void Allocate() { mask = 0x0; }
	};

	struct KRectangle
	{
		uint32_t left, top, right, bottom;
	};

	TRectangleAllocator()
	{
		Clear();
	}

	void Clear()
	{
		uint32_t depthCount = 1;
		uint32_t offset = 0;
		uint32_t extent = TotalExtent;
		for (uint32_t level = 0; level < LevelCount; ++level)
		{
			levelOffsets[level] = offset;
			offset += depthCount;

			levelExtents[level] = extent;
			extent = extent / 2;

			for (uint32_t index = 0; index < depthCount; ++index)
			{
				GetCapacity(level, index).Init(level);
			}

			depthCount *= 4;
		}
	}

	inline KRectangle Allocate(uint32_t width, uint32_t height)
	{
		uint32_t maxExtent = width > height ? width : height;
		uint32_t targetLevel = 0;
		for (targetLevel = 0; targetLevel < LevelCount && (levelExtents[targetLevel] >= maxExtent); ++targetLevel);

		targetLevel = targetLevel - 1;

		uint32_t absolutePath[LevelCount] = {};
		uint32_t relativePath[LevelCount] = {};
		for (uint32_t i = 0; i < LevelCount; ++i)absolutePath[i] = UINT32_MAX;
		absolutePath[0] = 0;
		relativePath[0] = 0;
		for (uint32_t level = 0; level < targetLevel; ++level)
		{
			for (uint32_t childIndex = 0; childIndex < 4; ++childIndex)
			{
				absolutePath[level + 1] = GetChildIndex(absolutePath[level], childIndex);
				if (GetCapacity(level + 1, absolutePath[level + 1]).HasCapacity(targetLevel))
				{
					relativePath[level + 1] = childIndex;
					break;
				}
			}
		}

		GetCapacity(targetLevel, absolutePath[targetLevel]).Allocate();
		for (uint32_t level = targetLevel; level > 0; --level)
		{
			uint32_t parent = GetParent(absolutePath[level]);
			GetCapacity(level - 1, parent).Update(level - 1, &GetCapacity(level, GetChildIndex(parent, 0)));
		}

		//根据路径计算矩形
		KRectangle rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = TotalExtent;
		rect.bottom = TotalExtent;

		for (uint32_t i = 1; i <= targetLevel; ++i)
		{
			switch (relativePath[i])
			{
			case 0:
				rect.right -= levelExtents[i];
				rect.bottom -= levelExtents[i];
				break;
			case 1:
				rect.left += levelExtents[i];
				rect.bottom -= levelExtents[i];
				break;
			case 2:
				rect.top += levelExtents[i];
				rect.right -= levelExtents[i];
				break;
			case 3:
				rect.top += levelExtents[i];
				rect.left += levelExtents[i];
				break;
			default:
				break;
			}
		}
		return rect;
	}

	void Free(const KRectangle&rectangle)
	{
		uint32_t paths[LevelCount];
		for (uint32_t i = 0; i < LevelCount; ++i)
			paths[i] = 0;

		uint32_t extent = rectangle.right - rectangle.left;
		uint32_t targetLevel = 0;

		for(uint32_t level=0;level<LevelCount;++level)
			if (extent == levelExtents[level])
			{
				targetLevel = level;
				break;
			}

		uint32_t x = 0, y = 0;
		for (uint32_t level = 0; level < targetLevel; level++)
		{
			if (rectangle.top == y)
			{
				if (rectangle.left == x)
				{
					paths[level] = 0;
				}
				else
				{
					paths[level] = 1;
					x += levelExtents[level + 1];
				}
			}
			else
			{
				if (rectangle.left == x)
				{
					paths[level] = 2;
					y += levelExtents[level + 1];
				}
				else
				{
					paths[level] = 3;
					x += levelExtents[level + 1];
					y += levelExtents[level + 1];
				}
			}
		}
		uint32_t parent = 0;
		uint32_t relativePath[LevelCount] = {};


		for (uint32_t level = 0; level < targetLevel; ++level)
		{
			relativePath[level + 1] = GetChildIndex(parent, paths[level]);
			parent = relativePath[level + 1];
		}

		GetCapacity(targetLevel, parent).Init(targetLevel);

		for (int level = targetLevel - 1; level >= 0; level--)
		{
			parent = GetParent(parent);
			GetCapacity(level, parent).Update(level, &GetCapacity(level + 1, 0));
		}
	}
	inline uint32_t GetChildIndex(uint32_t index, uint32_t child) 
	{ 
		return index * 4 + child; 
	}
	inline uint32_t GetParent(uint32_t index) 
	{ 
		return index / 4; 
	}
	inline KCapacity&GetCapacity(uint32_t level, uint32_t index) 
	{ 
		return levelCapacities[levelOffsets[level] + index]; 
	}
	uint32_t levelOffsets[LevelCount];
	uint32_t levelExtents[LevelCount];
	KCapacity levelCapacities[TCapacitiesCount<LevelCount>::size];
};
