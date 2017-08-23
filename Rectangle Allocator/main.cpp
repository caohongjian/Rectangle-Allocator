#include"allocator.h"
int main()
{
	TRectangleAllocator<2, 2048> allocator = {};
	sizeof(allocator);
	auto rect1 = allocator.Allocate(1024, 1024);
	auto rect2 = allocator.Allocate(1024, 1024);
	auto rect3 = allocator.Allocate(1024, 1024);
	auto rect4 = allocator.Allocate(1024, 1024);
	allocator.Free(rect1);
	allocator.Free(rect2);
	allocator.Free(rect3);
	allocator.Free(rect4);
}