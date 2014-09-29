#include <GDAL\gdal_priv.h>

#include "WDUtility.h"
#include "WDFloodfill.h"

extern int pWaterPixelCountThreshold;
extern double pOutletRateThreshold;

WDQueueScanlineFloodFill::WDQueueScanlineFloodFill(int _width, int _height) {
	width = _width;
	height = _height;
	isCellProcessed = new bool[width * height];
	floodCount = 0;
	outletCount = 0;
}

WDQueueScanlineFloodFill::~WDQueueScanlineFloodFill() {
	delete [] isCellProcessed;
}

void WDQueueScanlineFloodFill::LineFill(float* dataGrid, int* resultGrid, int row, int col, float baseFloatValue) {
	int lFillLoc = col;
	int cell = row * width + lFillLoc;
	while (lFillLoc >= 0 && !isCellProcessed[cell]) {
		if (!WD::IsTolerableEqual(baseFloatValue, dataGrid[cell])) {
			if (baseFloatValue > dataGrid[cell]) {
				outletCount++;
			}
			break;
		}
		if ((row > 0 && WD::IsTolerableMoreThan(baseFloatValue, dataGrid[cell - width])) || (row < height - 1 && WD::IsTolerableMoreThan(baseFloatValue, dataGrid[cell + width]))) {
			outletCount++;
		}
		resultGrid[cell] = WDRESULT_WATER;
		isCellProcessed[cell] = true;
		lFillLoc--;
		cell--;
		floodCount++;
	}
	lFillLoc++;
	int rFillLoc = col + 1;
	cell = row * width + rFillLoc;
	while (rFillLoc < width && !isCellProcessed[cell]) {
		if (!WD::IsTolerableEqual(baseFloatValue, dataGrid[cell])) {
			if (baseFloatValue > dataGrid[cell]) {
				outletCount++;
			}
			break;
		}
		if ((row > 0 && WD::IsTolerableMoreThan(baseFloatValue, dataGrid[cell - width])) || (row < height - 1 && WD::IsTolerableMoreThan(baseFloatValue, dataGrid[cell + width]))) {
			outletCount++;
		}
		resultGrid[cell] = WDRESULT_WATER;
		isCellProcessed[cell] = true;
		rFillLoc++;
		cell++;
		floodCount++;
	}
	rFillLoc--;
	WDFloodFillRange range = {lFillLoc, rFillLoc, row};
	rangeQueue.push(range);
}

void WDQueueScanlineFloodFill::FloodFill(float* dataGrid, int* resultGrid, int row, int col, float baseFloatValue) {
	floodCount = 0;
	outletCount = 0;
	WD::GridFill(isCellProcessed, width, height, false);
	LineFill(dataGrid, resultGrid, row, col, baseFloatValue);
	while (!rangeQueue.empty()) {
		WDFloodFillRange range = rangeQueue.front();
		rangeQueue.pop();
		for (int col_1 = range.left; col_1 <= range.right; col_1++) {
			int rowAbove = range.row - 1;
			int cellAbove = rowAbove * width + col_1;
			if (rowAbove >= 0 && (!isCellProcessed[cellAbove]) && WD::IsTolerableEqual(baseFloatValue, dataGrid[cellAbove])) {
				LineFill(dataGrid, resultGrid, rowAbove, col_1, baseFloatValue);
			}
			int rowBelow = range.row + 1;
			int cellBelow = rowBelow * width + col_1;
			if (rowBelow < height && (!isCellProcessed[cellBelow]) && WD::IsTolerableEqual(baseFloatValue, dataGrid[cellBelow])) {
				LineFill(dataGrid, resultGrid, rowBelow, col_1, baseFloatValue);
			}
		}
	}
	if ((float)outletCount / (float)floodCount > pOutletRateThreshold) {
		for (int row = 0; row < height; row++) {
			for (int col = 0; col < width; col++) {
				int cell = row * width + col;
				if (isCellProcessed[cell]) {
					resultGrid[cell] = WDRESULT_LAND;
				}
			}
		}
	}
	else if (floodCount < pWaterPixelCountThreshold) {
		for (int row = 0; row < height; row++) {
			for (int col = 0; col < width; col++) {
				int cell = row * width + col;
				if (isCellProcessed[cell]) {
					resultGrid[cell] = WDRESULT_SINK;
				}
			}
		}
	}
}