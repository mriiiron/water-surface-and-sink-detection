#include <iostream>
#include <GDAL\gdal_priv.h>

#include "WDUtility.h"

using std::cout;
using std::endl;

int pWaterPixelCountThreshold = 5;
double pEqualTolerance = 0.1;
double pOutletRateThreshold = 0.05;

void WD::PrintDatasetInfo(GDALDataset* dataset) {
	double adfGeoTransform[6];
	cout << "Driver: " << dataset->GetDriver()->GetDescription() << " " << dataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME) << endl;
	cout << "Size: " << dataset->GetRasterXSize() << " * " << dataset->GetRasterYSize() << " * "  << dataset->GetRasterCount() << endl;
	if (dataset->GetProjectionRef() != nullptr) {
		cout << "Projection: " << dataset->GetProjectionRef() << endl;
	}
	if (dataset->GetGeoTransform(adfGeoTransform) == CE_None) {
		cout << "Origin: " << adfGeoTransform[0] << ", " << adfGeoTransform[3] << endl;
		cout << "Pixel Size: " << adfGeoTransform[1] << ", " << adfGeoTransform[5] << endl;
	}
}

void WD::PrintBandInfo(GDALRasterBand* band) {
	int nBlockXSize, nBlockYSize;
	band->GetBlockSize(&nBlockXSize, &nBlockYSize);
	cout << "Block: " << nBlockXSize << " x " << nBlockYSize << endl;
	cout << "Type: " << GDALGetDataTypeName(band->GetRasterDataType()) << endl;
	cout << "Color Interpretation: " << GDALGetColorInterpretationName(band->GetColorInterpretation()) << endl;
	double adfMinMax[2];
	int bGotMin, bGotMax;
	adfMinMax[0] = band->GetMinimum(&bGotMin);
	adfMinMax[1] = band->GetMaximum(&bGotMax);
	if (!(bGotMin && bGotMax)) {
		GDALComputeRasterMinMax((GDALRasterBandH)band, true, adfMinMax);
	}
	cout << "Min: " << adfMinMax[0] << endl;
	cout << "Max: " << adfMinMax[1] << endl;
	if (band->GetOverviewCount() > 0) {
		cout << "Band has " << band->GetOverviewCount() << " overviews." << endl;
	}
	if (band->GetColorTable() != nullptr) {
		cout << "Band has a color table with " << band->GetColorTable()->GetColorEntryCount() << " entries.\n" << endl;
	}
}

void WD::GridFill(float* grid, int width, int height, float value) {
	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			grid[row * width + col] = value;
		}
	}
}

void WD::GridFill(bool* grid, int width, int height, bool value) {
	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			grid[row * width + col] = value;
		}
	}
}

void WD::GridFill(int* grid, int width, int height, int value) {
	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			grid[row * width + col] = value;
		}
	}
}

bool WD::IsTolerableEqual(float valueA, float valueB) {
	return fabs(valueA - valueB) <= pEqualTolerance ? true : false;
}

bool WD::IsTolerableLessThan(float valueA, float valueB) {
	return valueB - valueA > pEqualTolerance ? true: false;
}

bool WD::IsTolerableMoreThan(float valueA, float valueB) {
	return valueA - valueB > pEqualTolerance ? true: false;
}

// 5 6 7
// 4   0
// 3 2 1
WDNeighborCheck WD::CheckNeighbor(float* grid, int row, int col, int width, int height) {
	int cell = row * width + col;
	int relLoc[8] = {1, 1 + width, width, -1 + width, -1, -1 - width, -width, 1 - width};
	bool isSink = true, isFlat = true;
	for (int i = 0; i < 8; i++) {
		switch (i) {
		case 0:
			if (col == width - 1) { isFlat = false;  continue; }
			break;
		case 1:
			if (col == width - 1 || row == height - 1) { isFlat = false;  continue; }
			break;
		case 2:
			if (row == height - 1) { isFlat = false;  continue; }
			break;
		case 3:
			if (col == 0 || row == height - 1) { isFlat = false;  continue; }
			break;
		case 4:
			if (col == 0) { isFlat = false;  continue; }
			break;
		case 5:
			if (col == 0 || row == 0) { isFlat = false;  continue; }
			break;
		case 6:
			if (row == 0) { isFlat = false;  continue; }
			break;
		case 7:
			if (col == width - 1 || row == 0) { isFlat = false;  continue; }
			break;
		}
		int neighbor = cell + relLoc[i];
		if (IsTolerableEqual(grid[cell], grid[neighbor])) {
			isSink = false;
		}
		else if (grid[cell] > grid[neighbor]) {
			return WDNEIGHBOR_CAN_FLOW_DOWN;
		}
		else {
			isFlat = false;
		}
	}
	if (isSink) {
		if (isFlat) {
			return WDNEIGHBOR_UNKNOWN;
		}
		else {
			return WDNEIGHBOR_SINK;
		}
	}
	else if (isFlat) {
		return WDNEIGHBOR_FLAT;
	}
	else {
		return WDNEIGHBOR_HALF_SINK;
	}
}