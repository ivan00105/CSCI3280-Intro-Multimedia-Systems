#include "bmp.h"		//	Simple .bmp library
#include<iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

#define Baseline 30.0
#define Focal_Length 100
#define Image_Width 35.0
#define Image_Height 35.0
#define Resolution_Row 512
#define Resolution_Col 512
#define View_Grid_Row 9
#define View_Grid_Col 9

struct Point3d
{
	double x;
	double y;
	double z;
	Point3d(double x_, double y_, double z_) :x(x_), y(y_), z(z_) {}
};

struct Point2d
{
	double x;
	double y;
	Point2d(double x_, double y_) :x(x_), y(y_) {}
};


void equalizeHistogram(Bitmap& image) {
	int width = image.getWidth();
	int height = image.getHeight();
	int numPixels = width * height;

	vector<int> histogram(256, 0);
	vector<int> cdf(256, 0);

	// calculate the histogram
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			unsigned char red, green, blue;
			image.getColor(x, y, red, green, blue);
			histogram[red]++;
		}
	}

	// compute the cumulative distribution function (CDF)
	cdf[0] = histogram[0];
	for (int i = 1; i < 256; i++) {
		cdf[i] = cdf[i - 1] + histogram[i];
	}

	// equalize the histogram
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			unsigned char red, green, blue;
			image.getColor(x, y, red, green, blue);
			int newVal = (int)(round(255.0 * cdf[red] / numPixels));
			image.setColor(x, y, newVal, newVal, newVal);
		}
	}
}


int main(int argc, char** argv)
{
	if (argc < 5 || argc > 7)
	{
		cout << "Arguments prompt: viewSynthesis.exe <LF_dir> <X Y Z> OR: viewSynthesis.exe <LF_dir> <X Y Z> <focal_length>" << endl;
		return 0;
	}
	string LFDir = argv[1];
	double Vx = stod(argv[2]), Vy = stod(argv[3]), Vz = stod(argv[4]);
	double targetFocalLen = 100; // default focal length for "basic requirement" part
	if (argc >= 6)
	{
		targetFocalLen = stod(argv[5]);
	}


	vector<Bitmap> viewImageList;
	//! loading light field views
	for (int i = 0; i < View_Grid_Col * View_Grid_Row; i++)
	{
		char name[128];
		sprintf(name, "/cam%03d.bmp", i);
		string filePath = LFDir + name;
		Bitmap view_i(filePath.c_str());
		viewImageList.push_back(view_i);
	}

	Bitmap targetView(Resolution_Col, Resolution_Row);
	cout << "Synthesizing image from viewpoint: (" << Vx << "," << Vy << "," << Vz << ") with focal length: " << targetFocalLen << endl;
	//! resample pixels of the target view one by one
	for (int r = 0; r < Resolution_Row; r++)
	{
		for (int c = 0; c < Resolution_Col; c++)
		{
			Point3d rayRGB(0, 0, 0);
			//! resample the pixel value of this ray: TODO
			// focal length ratio between the target focal length and the source focal length (Focal_Length).
			double focalLengthRatio = targetFocalLen / Focal_Length;
			// scaled pixel coordinates (dx, dy) in the target image.
			double dx = (c - ((double)Resolution_Col) / 2) * focalLengthRatio ;
			double dy = (r - ((double)Resolution_Row) / 2) * focalLengthRatio ;
			// real-world coordinates (x, y) based on the scaled pixel coordinates
			double x = Vx + Vz / Focal_Length/ targetFocalLen * ((c - 255.5) / 256 * 17.5);
			double y = Vy + Vz / Focal_Length / targetFocalLen * ((r - 255.5) / 256 * 17.5);
			// pixel coordinates (col, row) in the image
			int col = ceil((c - Resolution_Col / 2.0) / targetFocalLen * Focal_Length + Resolution_Col / 2);
			int row = ceil((r - Resolution_Row / 2.0) / targetFocalLen * Focal_Length + Resolution_Row / 2);
			// half plane size
			const double HALF_PLANE = Baseline * 4; // view plane
			double x_start = -120;
			double y_start = 120;

			if ((abs(x) <= HALF_PLANE) && (abs(y) <= HALF_PLANE) 
				&&(col >= 0) && (col < Resolution_Col) && (row >= 0) && (row < Resolution_Row))
			{
				// neighbors of image plane
				int u1 = floor((HALF_PLANE + x) / Baseline);	// u
				int u2 = ceil((HALF_PLANE + x) / Baseline);		// ui+1
				int v1 = floor((HALF_PLANE - y) / Baseline);	// vi
				int v2 = ceil((HALF_PLANE - y) / Baseline);		// vi+1

				Point2d grid_point((HALF_PLANE + x) / Baseline, (HALF_PLANE - y) / Baseline);
				// scale down to the target grid point
				double scaled_x = x / Baseline, scaled_y = y / Baseline;
				double scaled_x_start = x_start / Baseline, scaled_y_start = y_start / Baseline;
				double a = scaled_x - (scaled_x_start + u1); // alpha
				double b = (scaled_y_start - v1) - scaled_y; // beta

				// target grid viewpoint index from image plane = 𝑡 ∗ 9 + 𝑠 => Vi * 9 + Ui
				Color vp1, vp2, vp3, vp4;

				viewImageList[v1 * 9 + u1].getColor(col, row, vp1.R, vp1.G, vp1.B);
				viewImageList[v1 * 9 + u2].getColor(col, row, vp2.R, vp2.G, vp2.B);
				viewImageList[v2 * 9 + u1].getColor(col, row, vp3.R, vp3.G, vp3.B);
				viewImageList[v2 * 9 + u2].getColor(col, row, vp4.R, vp4.G, vp4.B);

				//if (u1 >= 0 && u1 < 9 && u2 >= 0 && u2 < 9 && v1 >= 0 && v1 < 9 && v2 >= 0 && v2 < 9) {
					rayRGB.x = (1 - b) * ((1 - a) * vp1.R + a * vp2.R) + b * ((1 - a) * vp3.R + a * vp4.R);
					rayRGB.y = (1 - b) * ((1 - a) * vp1.G + a * vp2.G) + b * ((1 - a) * vp3.G + a * vp4.G);
					rayRGB.z = (1 - b) * ((1 - a) * vp1.B + a * vp2.B) + b * ((1 - a) * vp3.B + a * vp4.B);
				//}
			}
			//! record the resampled pixel value
			targetView.setColor(c, r, (unsigned char)rayRGB.x, (unsigned char)rayRGB.y, (unsigned char)rayRGB.z);
		}
	}
	if(argc == 7)
		equalizeHistogram(targetView);
	string savePath = "newView.bmp";
	targetView.save(savePath.c_str());
	cout << "Result saved!" << endl;
	return 0;
}