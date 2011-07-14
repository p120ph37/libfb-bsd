/*
 * ppm2fb-db
 * Simple program which displays a PPM bitmap using the framebuffer.
 * Note, these routines assume a 32-bit video mode.
 * Mostly intended as a test of the "fb.c" library.
 * This version uses double buffering and the fb_flip() function.
 */
#include "fb.h"
#include <stdio.h>
#include <sysexits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int w;
    int h;
    uint32_t pixel;
} img32_t;


img32_t* read_ppm(char* filename) {
    FILE* ppm;
    img32_t* img;
    int w, h, d, i;
    
    // open the file for input
    if(ppm = fopen(filename, "r")) {
	// only accept binary-format (with no comments)
	if(fscanf(ppm, "P6 %d %d %d", &w, &h, &d) == 3) {
	    // skip the final header newline
	    fseek(ppm, 1, SEEK_CUR);
	    // allocate a buffer to load into
	    if(img = malloc(sizeof(*img) + (w * h - 1) * sizeof(img->pixel))) {
		// load the data
		img->w = w; img->h = h;
		for(i = 0; i < w * h; i++) {
		    uint32_t r, g, b;
		    r = fgetc(ppm);
		    g = fgetc(ppm);
		    b = fgetc(ppm);
		    if(feof(ppm)) {
			free(img);
			fclose(ppm);
			return NULL;
		    }
		    (&img->pixel)[i] = (r << 16) + (g << 8) + b;
		}
		fclose(ppm);
		return img;
	    }
	}
	fclose(ppm);
    }
    return NULL;
}

// take off every blit for great justice
void blit(framebuffer_t* fb, img32_t* img, int x, int y) {
    int img_xs, img_xe, img_ys, img_ye;
    int fb_xs, fb_xe, fb_ys, fb_ye;
    int blit_y, blit_ye;
    img_xs = 0; img_ys = 0; img_xe = img->w; img_ye = img->h;
    fb_xs = x; fb_ys = y; fb_xe = fb_xs + img->w; fb_ye = fb_ys + img->h;
    // crop if outside viewable area
    if(fb_xs < 0) {
	img_xs -= fb_xs; fb_xs = 0;
	if(img_xs >= img_xe) return;
    }
    if(fb_ys < 0) {
	img_ys -= fb_ys; fb_ys = 0;
	if(img_ys >= img_ye) return;
    }
    if(fb_xe > fb->info.vi_width) {
	img_xe -= (fb_xe - fb->info.vi_width); fb_xe = fb->info.vi_width;
	if(img_xe < img_xs) return;
    }
    if(fb_ye > fb->info.vi_height) {
	img_ye -= (fb_ye - fb->info.vi_height); fb_ye = fb->info.vi_height;
	if(img_ye < img_ys) return;
    }
    blit_ye = img_ye - img_ys;
    for(blit_y = 0; blit_y < blit_ye; blit_y++) {
	memcpy(
	       fb->work_buffer + ((fb_ys + blit_y) * fb->info.vi_width + fb_xs) * sizeof(img->pixel),
	       (void*)&(img->pixel) + ((img_xs + blit_y) * img->w + img_xs) * sizeof(img->pixel),
	       (img_xe - img_xs) * sizeof(img->pixel)
	      );
    }
}

// clear the screen, using a particular color
void cls(framebuffer_t* fb, uint32_t color) {
    long i;
    long max;
    max = fb->info.vi_buffer_size / sizeof(color);
    for(i = 0; i < max; i++) {
	((uint32_t*)(fb->work_buffer))[i] = color;
    }
}


int main(int argc, char* argv[]) {
    framebuffer_t* fb;
    img32_t* img;
    
    // check for proper usage
    if(argc == 1) {
	fprintf(stderr, "Usage: %s lena.ppm\n", argv[0]);
	return EX_USAGE;
    }
    
    // initialize the framebuffer
    fprintf(stderr, "Starting up...\n");
    if((fb = fb_open(0x11e, 1, 0)) == NULL) {
	perror("fb_open");
	return EX_UNAVAILABLE;
    }

    // clear the screen in a shade of blue
    // (note, when double-buffering is active, this applies to the *work* buffer, not the show buffer)
    cls(fb, 0x00007F);

    // switch video pages (this should make the hidden blue screen appear)
    fb_flip(fb);

    // pause
    getc(stdin);

    // load the image
    img = read_ppm(argv[1]);
    
    // clear the screen in a shade of red (again, this applies to the hidden buffer)
    cls(fb, 0x7F0000);

    // display the image centered
    blit(fb, img, fb->info.vi_width / 2 - img->w / 2, fb->info.vi_height / 2 - img->h / 2);
    
    // free the image
    free(img);

    // switch video pages (this should make the hidden image on a red screen appear)
    fb_flip(fb);

    // pause for dramatic effect...
    getc(stdin);
    
    // tidy up
    fprintf(stderr, "Shutting down...\n");
    fb_close(fb);
    return EX_OK;
}
