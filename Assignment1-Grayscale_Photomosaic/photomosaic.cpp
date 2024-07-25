/*

CSCI 3280, Introduction to Multimedia Systems
Spring 2023

Assignment 01 Skeleton

photomosaic.cpp

*/

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

double getTileBrightness(Bitmap& tile);
void processTile(Bitmap& tile, int cell_w, int cell_h, Bitmap& processed_tile);
double getRegionBrightness(int cell_h, int cell_w, int leftx, int lefty, Bitmap& bmp);
int getClosestDistanceIndex(int num_of_tiles, vector<double> tiles_brightness, double region_brightness);

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

    int num_of_tiles = list_of_paths.size();
    vector<Bitmap> tiles, processed_tiles;
	vector<double> tiles_brightness;

    //resize all tiles and convert them to grayscale
    for (int i = 0; i < num_of_tiles; i++) {
        Bitmap bmp1,bmp2;
		bmp1.create(list_of_paths[i].c_str());//convert a string object to a char*
		bmp2.create(cell_w, cell_h);
		tiles.push_back(bmp1);
		processed_tiles.push_back(bmp2);
		processed_tiles[i].create(cell_w, cell_h);
        processTile(tiles[i], cell_w, cell_h, processed_tiles[i]);

        //average the brightness
		tiles_brightness.push_back(getTileBrightness(processed_tiles[i]));

    }

    Bitmap output_bitmap(output_w, output_h);

    for (int y = 0; y < output_h; y+=cell_h) {
		for (int x = 0; x < output_w; x+=cell_w) {
            //calculate the region brightness of the transformed bitmap
            double region_brightness = getRegionBrightness(cell_h, cell_w, x, y, transformed_bitmap);
            int most_similar = getClosestDistanceIndex(num_of_tiles,tiles_brightness,region_brightness);
            //loop through the tile and copy the tile to one of the regions of the output bmp
			for (int i = 0; i < cell_h; i++) {
				for (int j = 0; j < cell_w; j++) {
					Color RGB;
					processed_tiles[most_similar].getColor(j, i, RGB.R, RGB.G, RGB.B);
					output_bitmap.setColor(x + j , y + i, RGB.R, RGB.G, RGB.B);
				}
			}
        }
    }

	//save output bitmap to argv[4]
    output_bitmap.save(argv[4]);

    return 0;
}


//calculate the brightness of a bitmap
double getTileBrightness(Bitmap& tile)
{
    int tile_w = tile.getWidth();
    int tile_h = tile.getHeight();
    double tile_brightness = 0.0;
    for (int y = 0; y < tile_h; y++) {
        for (int x = 0; x < tile_w; x++) {
            unsigned char R, G, B;
            tile.getColor(x, y, R, G, B);
            tile_brightness += 0.299 * R + 0.587 * G + 0.114 * B;
        }
    }

    return tile_brightness / (double)(tile_w * tile_h);
}

//convert a tile to grayscale and resize it to cell_w and cell_h
void processTile(Bitmap& tile, int cell_w, int cell_h, Bitmap& processed_tile){
    int tile_w = tile.getWidth();
    int tile_h = tile.getHeight();
    processed_tile.create(cell_w, cell_h);

    for (int y = 0; y < cell_h; y++) {
		for (int x = 0; x < cell_w; x++) {
            //mapping
			float fx = x * tile_w / cell_w;//f(x)
			float fy = y * tile_h / cell_h;//f(y)
            //find the four surrounding pixels
			int x1 = floor(fx);
			// int x2 = ceil(fx) > tile_w - 1 ? tile_w - 1 : ceil(fx);
            int x2 = ceil(fx);
			int y1 = floor(fy);
			// int y2 = ceil(fy) > tile_h - 1 ? tile_h - 1 : ceil(fy);
            int y2 = ceil(fy);

            //border case
            if (x2 == 0 && x1 == 0) x2 += 1;
            else if(x1 == x2) x1 -= 1;
            if (y2 == 0 && y1 == 0) y2 += 1;
            else if(y1 == y2) y1 -= 1;

            Color c1, c2, c3, c4;//BL, TL, BR, TR
            tile.getColor(x1, y1, c1.R, c1.G, c1.B);
            tile.getColor(x1, y2, c2.R, c2.G, c2.B);
            tile.getColor(x2, y1, c3.R, c3.G, c3.B);
            tile.getColor(x2, y2, c4.R, c4.G, c4.B);

            int horizontal_distance = abs(x2-x1);
            int vertical_distance = abs(y2-y1);
            unsigned char upper_weighted_R, upper_weighted_G, upper_weighted_B, lower_weighted_R, lower_weighted_G, lower_weighted_B;
            //upper weighted average
            upper_weighted_R = (c2.R * (x2-fx) + c4.R * (fx-x1))/horizontal_distance;
            upper_weighted_G = (c2.G * (x2-fx) + c4.G * (fx-x1))/horizontal_distance;
            upper_weighted_B = (c2.B * (x2-fx) + c4.B * (fx-x1))/horizontal_distance;
            //lower weighted average
            lower_weighted_R = (c3.R * (x2-fx) + c1.R * (fx-x1))/horizontal_distance;
            lower_weighted_G = (c3.G * (x2-fx) + c1.G * (fx-x1))/horizontal_distance;
            lower_weighted_B = (c3.B * (x2-fx) + c1.B * (fx-x1))/horizontal_distance;

            unsigned char vertical_weighted_R, vertical_weighted_G, vertical_weighted_B;

            //vertical weighted average
            vertical_weighted_R = (upper_weighted_R * (y2-fy) + lower_weighted_R * (fy-y1))/vertical_distance;
            vertical_weighted_G = (upper_weighted_G * (y2-fy) + lower_weighted_G * (fy-y1))/vertical_distance;
            vertical_weighted_B = (upper_weighted_B * (y2-fy) + lower_weighted_B * (fy-y1))/vertical_distance;

            unsigned char gray = vertical_weighted_R * 0.299 + vertical_weighted_G * 0.587 + vertical_weighted_B * 0.114;
            processed_tile.setColor(x,y,gray,gray,gray);

            // unsigned char avg_r = (c1.R + c2.R + c3.R + c4.R) / 4;
			// unsigned char avg_g = (c1.G + c2.G + c3.G + c4.G) / 4;
			// unsigned char avg_b = (c1.B + c2.B + c3.B + c4.B) / 4;
			// unsigned char gray = avg_r * 0.299 + avg_g * 0.587 + avg_b * 0.114;
            // processed_tile.setColor(x, y, gray, gray, gray);
        }
    }
}

double getRegionBrightness(int cell_h, int cell_w, int leftx, int lefty, Bitmap& bmp){
    double region_brightness = 0.0;
    for (int i = 0; i < cell_h; i++) {
        for (int j = 0; j < cell_w; j++) {
            unsigned char R, G, B;
            bmp.getColor(j + leftx, i + lefty, R, G, B);
            region_brightness += 0.299 * R
                + 0.587 * G
                + 0.114 * B;
        }
    }
    return region_brightness /= (cell_h * cell_w);
}

int getClosestDistanceIndex(int num_of_tiles, vector<double> tiles_brightness, double region_brightness){
    int most_similar = 0;
    for (int i = 1; i < num_of_tiles; i++){
        if (abs(tiles_brightness[i] - region_brightness) < abs(tiles_brightness[most_similar] - region_brightness))
            most_similar = i;
    }
    return most_similar;
}