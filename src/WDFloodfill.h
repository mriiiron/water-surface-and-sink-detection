#pragma once

#include <queue>


using std::queue;


struct WDFloodFillRange {
	int left, right, row;
};


class WDQueueScanlineFloodFill {

public:

	queue<WDFloodFillRange> rangeQueue;
	bool* isCellProcessed;
	int width, height;
	int floodCount;
	int outletCount;

	WDQueueScanlineFloodFill(int _width, int _height);
	~WDQueueScanlineFloodFill();

	void LineFill(float* dataGrid, int* resultGrid, int row, int col, float baseFloatValue);
	void FloodFill(float* dataGrid, int* resultGrid, int row, int col, float baseFloatValue);
	

};