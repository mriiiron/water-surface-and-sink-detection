#include <iostream>
#include <string>
#include <windows.h> 
#include <GDAL\gdal_priv.h>
#include <GDAL\cpl_conv.h>

#include "WDUtility.h"
#include "WDFloodfill.h"

using std::cout;
using std::endl;
using std::string;

extern int pWaterPixelCountThreshold;
extern double pEqualTolerance;
extern double pOutletRateThreshold;

void exitWithMessage(const char* msg) {
	cout << msg << endl;
	exit(EXIT_FAILURE);
}

void PrintUsage() {
	cout << endl << "USAGE:" << endl << endl;
	cout << "Copy this application and gdal110.dll to the same folder with the data." << endl;
	cout << "WaterSurfaceDetect in_name out_name [waterpxcount] [equal] [outletrate]" << endl << endl;
	cout << "in_name         Input file name" << endl;
	cout << "out_name        Output file name without extension (GTiff format)" << endl;
	cout << "[waterpxcount]  Least count of pixels to be water (larger than 0)" << endl;
	cout << "[equal]         Equal tolerance (larger than 0.0)" << endl;
	cout << "[outletrate]    Least outlet rate to be water (0.0, 1.0]" << endl << endl;
}

int main(int argc, char* argv[]) {

	TCHAR cstr_path[MAX_PATH];
	DWORD dwRet = GetCurrentDirectory(MAX_PATH, cstr_path);
	string path(cstr_path);
	if (dwRet == 0) {
		exitWithMessage("Error: GetCurrentDirectory failed.");
	}
	// cout << path << endl;

	string in_name;
	string out_name;
	if (argc == 3 || argc == 6) {
		in_name = argv[1];
		out_name = argv[2];
		if (argc == 6) {
			pWaterPixelCountThreshold = atoi(argv[3]);
			if (pWaterPixelCountThreshold <= 0) {
				exitWithMessage("Error: Parameter thres_waterpxcount is invalid.");
			}
			pEqualTolerance = atof(argv[4]);
			if (pEqualTolerance <= 0.0) {
				exitWithMessage("Error: Parameter toler_equal is invalid.");
			}
			pOutletRateThreshold = atof(argv[5]);
			if (pOutletRateThreshold <= 0.0 || pOutletRateThreshold > 1.0) {
				exitWithMessage("Error: Parameter thres_outletrate is invalid.");
			}
		}
	}
	else {
		PrintUsage();
		return 0;
	}

	// Register GDAL drivers
	GDALAllRegister();

	// Get input file name
	string in_name_path = path + "\\" + in_name;
	cout << "Opening file: " << path << endl;

	// Open DEM
	GDALDataset* srcDataset = (GDALDataset*)GDALOpen(in_name_path.c_str(), GA_ReadOnly);
	if (srcDataset == nullptr) {
		cout << "Error: Open dataset failed." << endl;
		return 0;
	}
	GDALDriver* srcDriver = srcDataset->GetDriver();
	cout << "Driver: " << srcDriver->GetDescription() << " " << srcDriver->GetMetadataItem(GDAL_DMD_LONGNAME) << endl;
	if (srcDriver == nullptr) {
		cout << "Error: Open source driver failed." << endl;
		return 0;
	}

	// Get the only band (i.e. elevation data)
	GDALRasterBand* srcBand = srcDataset->GetRasterBand(1);
		if (srcBand == nullptr) {
		cout << "Error: Open band failed." << endl;
		return 0;
	}
	WD::PrintBandInfo(srcBand);

	// Read source data
	int width = srcBand->GetXSize();
	int height = srcBand->GetYSize();
	float* demGrid = (float*)CPLMalloc(sizeof(float) * width * height);
	srcBand->RasterIO(GF_Read, 0, 0, width, height, demGrid, width, height, GDT_Float32, 0, 0);

	// Assign initial data to result grid
	int* resultGrid = (int*)CPLMalloc(sizeof(int) * width * height);
	WD::GridFill(resultGrid, width, height, WDRESULT_UNKNOWN);

	// Start detecting
	WDQueueScanlineFloodFill floodfiller(width, height);
	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			int cell = row * width + col;
			if (resultGrid[cell] != WDRESULT_UNKNOWN) {
				continue;
			}
			WDNeighborCheck t = WD::CheckNeighbor(demGrid, row, col, width, height);
			switch (t) {
			case WDNEIGHBOR_CAN_FLOW_DOWN:
				resultGrid[cell] = WDRESULT_LAND;
				break;
			case WDNEIGHBOR_SINK:
				resultGrid[cell] = WDRESULT_SINK;
				break;
			case WDNEIGHBOR_HALF_SINK:
				cout << "Floodfilling on " << row << ", " << col << " (" << height << " Lines) ... ";
				floodfiller.FloodFill(demGrid, resultGrid, row, col, demGrid[cell]);
				cout << "Done." << endl;
				break;
			default:
				break;
			}
		}
	}

	// Create a new GTiff driver
    GDALDriver *dstDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	if (dstDriver == nullptr) {
		cout << "Error: Open destiny driver failed." << endl;
		return 0;
	}

	// Use driver to create output formatted with GTiff
	string out_name_path = path + "\\" + out_name + ".tif";
	char **papszOptions = nullptr;
	cout << endl << "All done. With parameters:" << endl;
	cout << "Water Pixel Count Threshold: " << pWaterPixelCountThreshold << endl;
	cout << "Equal Tolerance: " << pEqualTolerance << endl;
	cout << "Outlet Rate Threshold: " << pOutletRateThreshold << endl << endl;
	cout << "Output file as: " << out_name_path << endl;
	GDALDataset* dstDataset = dstDriver->Create(out_name_path.c_str(), width, height, 1, GDT_Int32, papszOptions);
	if (dstDataset == nullptr) {
		cout << "Error: Create dataset failed." << endl;
		return 0;
	}

	// Set needed metadata for the output dataset
	double adfGeoTransform[6];
	srcDataset->GetGeoTransform(adfGeoTransform);
	dstDataset->SetGeoTransform(adfGeoTransform);
	dstDataset->SetProjection(srcDataset->GetProjectionRef());
	GDALRasterBand* dstBand = dstDataset->GetRasterBand(1);

	// Write output buffer to the output dataset
	dstBand->RasterIO(GF_Write, 0, 0, width, height, resultGrid, width, height, GDT_Int32, 0, 0);
	
	// Cleaning
	CPLFree(demGrid);
	CPLFree(resultGrid);
	GDALClose(dstDataset);
	GDALClose(srcDataset);

	// Exiting program
	exit(EXIT_SUCCESS);

}