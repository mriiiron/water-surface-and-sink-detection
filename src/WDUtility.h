#pragma once

#define WDRESULT_UNKNOWN			0
#define WDRESULT_LAND				1
#define WDRESULT_WATER				2
#define WDRESULT_SINK				3

enum WDNeighborCheck {
	WDNEIGHBOR_UNKNOWN, WDNEIGHBOR_CAN_FLOW_DOWN, WDNEIGHBOR_SINK, WDNEIGHBOR_HALF_SINK, WDNEIGHBOR_FLAT
};

namespace WD {

	void PrintDatasetInfo(GDALDataset* dataset);
	void PrintBandInfo(GDALRasterBand* band);

	void GridFill(float* grid, int width, int height, float value);
	void GridFill(bool* grid, int width, int height, bool value);
	void GridFill(int* grid, int width, int height, int value);

	bool IsTolerableEqual(float valueA, float valueB);
	bool IsTolerableLessThan(float valueA, float valueB);
	bool IsTolerableMoreThan(float valueA, float valueB);

	WDNeighborCheck CheckNeighbor(float* grid, int row, int col, int width, int height);

}