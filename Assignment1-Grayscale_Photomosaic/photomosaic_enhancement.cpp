#include "stdio.h"
#include <iostream>
#include <vector>
#include <string>
#include "malloc.h"
#include "memory.h"
#include "math.h"
#include "bmp.h"		//	Simple .bmp library
#include "list_files.h" //	Simple list file library

#define SAFE_FREE(p)  { if(p){ free(p);  (p)=NULL;} }
using namespace std;

struct Coordinate{
	double x;
	unsigned char r,g,b;
};


void processTile(Bitmap& tile, int cell_w, int cell_h, Bitmap& processed_tile);
int getClosestDistanceIndex(int num_of_tiles, vector<vector<int>> tiles_hist, vector<int> region_hist);
vector<Color> splineCalculation(vector<Coordinate> points, double delta, int length);
void toGrayscale(Bitmap& bitmap);
vector<int> calculateHistogram(Bitmap& bitmap);
double histogramIntersection(vector<int>& hist1, vector<int>& hist2);
double histogramIntersectionDistance(vector<int>& hist1, vector<int>& hist2);
vector<int> getRegionHistogram(int cell_h, int cell_w, int leftx, int lefty, Bitmap& bmp);
void blend(Bitmap& b1, Bitmap& b2);

int main(int argc, char** argv){
    // Parse output and cell shape specified in argv[3]
	int output_w, output_h, cell_w, cell_h;
	sscanf(argv[3], "%d,%d,%d,%d", &output_w, &output_h, &cell_w, &cell_h);

	// Read source bitmap from argv[1]
	Bitmap source_bitmap(argv[1]);
	int source_w = source_bitmap.getWidth();
	int source_h = source_bitmap.getHeight();

	// List .bmp files in argv[2] and do preprocessing
	vector<string> list_of_paths;
	list_files(argv[2], ".bmp", list_of_paths, false);

    //create output bitmap
	Bitmap transformed_bitmap(output_w, output_h);
    //resize and convert original bmp into grayscale
    processTile(source_bitmap, output_w, output_h, transformed_bitmap);
    // Bitmap transformed_bitmap_color = transformed_bitmap;
    toGrayscale(transformed_bitmap);

    int num_of_tiles = list_of_paths.size();
    vector<Bitmap> tiles, processed_tiles;
	vector<double> tiles_brightness;
    vector<vector<int>> tiles_hist;
    cout << "Resizing..." << endl;
    //resize all tiles and convert them to grayscale
    for (int i = 0; i < num_of_tiles; i++) {
        Bitmap bmp1,bmp2;
		bmp1.create(list_of_paths[i].c_str());//convert a string object to a char*
		bmp2.create(cell_w, cell_h);
		tiles.push_back(bmp1);
		processed_tiles.push_back(bmp2);
		processed_tiles[i].create(cell_w, cell_h);
        processTile(tiles[i], cell_w, cell_h, processed_tiles[i]);
        // processed_tiles_color.push_back(processed_tiles[i]);
        toGrayscale(processed_tiles[i]);

        tiles_hist.push_back(calculateHistogram(processed_tiles[i]));

    }

    cout << "Composing..." << endl;
    Bitmap output_bitmap(output_w, output_h);
    vector<int> used_tiles;
    vector<Bitmap> used_bitmaps;
    for (int y = 0; y < output_h; y+=cell_h) {
		for (int x = 0; x < output_w; x+=cell_w) {
            vector<int> region_hist = getRegionHistogram(cell_h, cell_w, x, y, transformed_bitmap);//histogram for the region
            int most_similar = getClosestDistanceIndex(num_of_tiles,tiles_hist,region_hist);//most similar tile's index
            bool same_color = false;
            int used_tiles_size = used_tiles.size();
            Bitmap region(cell_w,cell_h);
            if(used_tiles_size){
                //check left and up
                if((x != 0 && used_tiles[used_tiles_size - 1] == most_similar) || (used_tiles_size >= output_w / cell_w && used_tiles[used_tiles_size - output_w / cell_w] == most_similar)){
                    same_color = true;
                    for (int i = 0; i < cell_h; i++) {
                        for (int j = 0; j < cell_w; j++) {
                            unsigned char r,g,b;
                            // transformed_bitmap_color.getColor(x + j , y + i, r, g, b);
                            transformed_bitmap.getColor(x + j , y + i, r, g, b);
                            region.setColor(j,i,r,g,b);
                        }
                    }
                    int idx = (x != 0 && used_tiles[used_tiles_size - 1] == most_similar)? used_tiles_size - 1 : used_tiles_size - output_w / cell_w;
                    blend(used_bitmaps[idx], region);
                    // toGrayscale(region);
                    // most_similar = -1;//mark as blended cell
                    // used_bitmaps.push_back(region);
                }
            }
            //loop through the tile and copy the tile to one of the regions of the output bmp
			for (int i = 0; i < cell_h; i++) {
				for (int j = 0; j < cell_w; j++) {
					Color RGB;
                    if(!same_color){
					    processed_tiles[most_similar].getColor(j, i, RGB.R, RGB.G, RGB.B);
                    }
                    else{
                        region.getColor(j, i, RGB.R, RGB.G, RGB.B);
                    }
					    output_bitmap.setColor(x + j , y + i, RGB.R, RGB.G, RGB.B);
				}
			}
            used_tiles.push_back(most_similar);
            // Bitmap temp = same_color?region:processed_tiles[most_similar];
            used_bitmaps.push_back(same_color?region:processed_tiles[most_similar]);
        }
    }
    cout << "Done!!";
	//save output bitmap to argv[4]
    output_bitmap.save(argv[4]);

    return 0;
}



vector<Color> splineCalculation(vector<Coordinate> points, double delta, int length){
    vector<double> resampled_points;
    vector<Coordinate> extrapolated_points;
    vector<Color> RGB;
    //resample all the data points
    for(int i = 0; i < length; i++){
        double x = (i + 0.5) * delta - 0.5;
        // double x = delta * i;
        resampled_points.push_back(x);
    }
    //extrapolate on both ends
    for(int i = 0; i < points.size(); i++){
        Coordinate new_point;
        new_point.x = (double)points[i].x;
        new_point.r = points[i].r;
        new_point.g = points[i].g;
        new_point.b = points[i].b;
        if(i == 0){
            new_point.x = (double)points[i].x - 2;
            extrapolated_points.push_back(new_point);
            new_point.x += (double)1;
            extrapolated_points.push_back(new_point);
            new_point.x += (double)1;
        }
        extrapolated_points.push_back(new_point);
        if(i == points.size()-1){
            new_point.x += (double)1;
            extrapolated_points.push_back(new_point);
            new_point.x += (double)1;
            extrapolated_points.push_back(new_point);
        }
    }

    vector<Color> output_row;
    int cur_point = 0;
    //loop through all splines
    for(int i = 1; i < extrapolated_points.size()-2; i++){
        Coordinate prev_point = extrapolated_points[i-1];
        Coordinate p1 = extrapolated_points[i];
        Coordinate p2 = extrapolated_points[i+1];
        Coordinate next_point = extrapolated_points[i+2];
        //check if the value exceeds the current upper point of the spline
        while(cur_point < length && resampled_points[cur_point] <= p2.x){
            //calculate f(x)
            double x = resampled_points[cur_point] - p1.x;
            double f0[3]={(double)p1.r,(double)p1.g,(double)p1.b};
            double f1[3]={(double)p2.r,(double)p2.g,(double)p2.b};
            double df0[3] = {
                (p2.r-prev_point.r)/(p2.x-prev_point.x),
                (p2.g-prev_point.g)/(p2.x-prev_point.x),
                (p2.b-prev_point.b)/(p2.x-prev_point.x)
            };
            double df1[3] = {
                (next_point.r - p1.r)/(next_point.x - p1.x),
                (next_point.g - p1.g)/(next_point.x - p1.x),
                (next_point.b - p1.b)/(next_point.x - p1.x)
            };
            double a[3],b[3],c[3],d[3],fx[3];
            Color RGB;
            for(int j = 0;j < 3; j++){
                a[j] = 2 * f0[j] - 2 * f1[j] + df0[j] + df1[j];
                b[j] = -3 * f0[j] + 3 * f1[j] - 2*df0[j] - df1[j];
                c[j] = df0[j];
                d[j] = f0[j];
                fx[j] = a[j]*x*x*x + b[j] *x*x + c[j]*x + d[j];
            }
            RGB.R=(unsigned char)fx[0];
            RGB.G=(unsigned char)fx[1];
            RGB.B=(unsigned char)fx[2];
            output_row.push_back(RGB);
            cur_point++;
            if(cur_point >= length) break;
        }
    }
    return output_row;//return RGB values for each pixel in a row/col
}

//convert a tile to grayscale and resize it to cell_w and cell_h
void processTile(Bitmap& tile, int cell_w, int cell_h, Bitmap& processed_tile){
    int tile_w = tile.getWidth();
    int tile_h = tile.getHeight();
    //process the row first therefore the height will be original height
    Bitmap processed_x_tile(cell_w,tile_h);
    processed_tile.create(cell_w, cell_h);

    double delta_x = (double)tile_w/cell_w;
    //loop through each row
    for (int y = 0; y < tile_h; y++) {
        vector<Coordinate> points;//a row of original points
		for (int x = 0; x < tile_w; x++) {
            Coordinate new_point;
            new_point.x = (double)x;
            tile.getColor(x,y,new_point.r,new_point.g,new_point.b);
            points.push_back(new_point);
        }
        vector<Color> row = splineCalculation(points, delta_x, cell_w);
        for(int x = 0; x < cell_w; x++){
            processed_x_tile.setColor(x,y,row[x].R,row[x].G,row[x].B);
        }
    }
    //loop through each column
    double delta_y = (double)tile_h/cell_h;
    for(int x = 0; x < cell_w; x++){
        vector<Coordinate> points;
        for (int y = 0; y < tile_h; y++){
            Coordinate new_point;
            new_point.x = (double)y;
            processed_x_tile.getColor(x,y,new_point.r,new_point.g,new_point.b);
            points.push_back(new_point);
        }
        vector<Color> col = splineCalculation(points, delta_y, cell_h);
        for (int y = 0; y < cell_h; y++){
            // unsigned char gray = col[y].R * 0.299 + col[y].G * 0.587 + col[y].B * 0.114;
            processed_tile.setColor(x,y,col[y].R,col[y].G,col[y].B);
        }
    }
}

int getClosestDistanceIndex(int num_of_tiles, vector<vector<int>> tiles_hist, vector<int> region_hist){
    int most_similar = 0;
    for (int i = 1; i < num_of_tiles; i++){
        if (histogramIntersectionDistance(tiles_hist[most_similar],region_hist) > 
        histogramIntersectionDistance(tiles_hist[i],region_hist))
            most_similar = i;
    }
    return most_similar;
}

void toGrayscale(Bitmap& bitmap){
    int width = bitmap.getWidth();
    int height = bitmap.getHeight();

    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            Color c;
            bitmap.getColor(j , i, c.R, c.G, c.B);
            unsigned char gray = c.R * 0.299 + c.G * 0.587 + c.B * 0.114;
            bitmap.setColor(j,i,gray,gray,gray);
        }
    }
    
}

//calculate histogram for a bitmap
vector<int> calculateHistogram(Bitmap& bitmap){
    int num_of_bins = 256;
    int width = bitmap.getWidth();
    int height = bitmap.getHeight();
    vector<int> hist(num_of_bins);
     for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            unsigned char r,g,b;
            bitmap.getColor(i, j, r, g, b);
            int bin_idx = ((r+g+b)/3) % num_of_bins;
            ++hist[bin_idx];
        }
     }
    return hist;
}

//computes the intersection of two histograms
double histogramIntersection(vector<int>& hist1, vector<int>& hist2) {
    double intersection = 0.0;
    for (int i = 0; i < hist1.size(); i++) {
        intersection += min(hist1[i], hist2[i]);
    }
    return intersection;
}

//computes the distance between two histograms using Intersection
double histogramIntersectionDistance(vector<int>& hist1, vector<int>& hist2) {
    double intersection = histogramIntersection(hist1, hist2);
    double total_bins = hist1.size();
    double distance = 1 - (intersection / total_bins);
    return distance;
}

vector<int> getRegionHistogram(int cell_h, int cell_w, int leftx, int lefty, Bitmap& bmp){
    double region_brightness = 0.0;
    Bitmap region(cell_w,cell_h);
    for (int i = 0; i < cell_h; i++) {
        for (int j = 0; j < cell_w; j++) {
            unsigned char R, G, B;
            bmp.getColor(j + leftx, i + lefty, R, G, B);
            region.setColor(j,i,R,G,B);
        }
    }
    return calculateHistogram(region);
}

void blend(Bitmap& b1, Bitmap& b2){
    int width = b2.getWidth();
    int height = b2.getHeight();
    float weight = 0.5;
    for(int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            unsigned char r[2],g[2],b[2];
            b1.getColor(x,y,r[0],g[0],b[0]);
            b2.getColor(x,y,r[1],g[1],b[1]);
            unsigned char blended_r = weight * r[0] + (1 - weight) * r[1];
            unsigned char blended_g = weight * g[0] + (1 - weight) * g[1];
            unsigned char blended_b = weight * b[0] + (1 - weight) * b[1];

            b2.setColor(x,y,blended_r,blended_g,blended_b);
        }
    }
}