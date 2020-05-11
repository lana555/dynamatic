#include <stdlib.h>
#include "video_filter.h"

void video_filter(inout_int_t pixel_red[30][30], inout_int_t pixel_blue[30][30], inout_int_t pixel_green[30][30], in_int_t offset, in_int_t scale) {

	for (int row = 0; row < 30; row++) {

		for (int col = 0; col < 30; col++) {

			pixel_red[row][col] = ((pixel_red[row][col]-offset) * scale) >> 4;
			pixel_blue[row][col] = ((pixel_blue[row][col]-offset) * scale) >> 4;
			pixel_green[row][col] = ((pixel_green[row][col]- offset) *scale) >> 4;

		}
	}
}

#define AMOUNT_OF_TEST 1

int main(void){
	inout_int_t pixel_red[AMOUNT_OF_TEST][30][30];
	inout_int_t pixel_blue[AMOUNT_OF_TEST][30][30];
	inout_int_t pixel_green[AMOUNT_OF_TEST][30][30];
	in_int_t offset[AMOUNT_OF_TEST];
	in_int_t scale[AMOUNT_OF_TEST];

	srand(13);
	for(int i = 0; i < AMOUNT_OF_TEST; ++i){
		offset[i] = rand() % 100;
		scale[i] = rand() % 10;
		for(int y = 0; y < 30; ++y){
			for(int x = 0; x < 30; ++x){
				pixel_red[i][y][x] = rand() % 1000;
				pixel_blue[i][y][x] = rand() % 1000;
				pixel_green[i][y][x] = rand() % 1000;
			}
	    }
    }
    
	//for(int i = 0; i < AMOUNT_OF_TEST; ++i){
    int i = 0;
	video_filter(pixel_red[i], pixel_blue[i], pixel_green[i], offset[i], scale[i]);
	//}
}



