#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include "fb.h"

// set console to desired mode
static int set_mode(int mode) {
    if(mode >= M_VESA_BASE) {
	if(ioctl(0, _IO('V', mode - M_VESA_BASE), NULL) == -1) return -1;
    } else {
 	if(ioctl(0, _IO('S', mode), NULL) == -1) return -1;
    }
    return 0;
}

// populate a video_info structure based on the current mode
static int mode_info(video_info_t *info) {
    if(ioctl(0, CONS_GET, &info->vi_mode) == -1) return -1;
    if(ioctl(0, CONS_MODEINFO, info) == -1) return -1;
    return 0;
}

// Puts the VTY into the requested mode and returns a pointer to the framebuffer struct.
// For a list of available modes, try: vidcontrol -i mode
// If you need to find the list programmatically, see the vidcontrol.c source.
// Returns a NULL pointer if there was an error (check errno for details)
// Remember to call fb_close later to free the resources and reset the hardware.
framebuffer_t* fb_open(int mode) {
    framebuffer_t *fb;
    int fd;

    if((fb = malloc(sizeof(framebuffer_t))) != NULL) {
	fb->con_info.size = sizeof(fb->con_info);
	if(ioctl(0, CONS_GETINFO, &fb->con_info) == 0
	   &&
	   mode_info(&fb->old_info) == 0
	   &&
	   set_mode(mode) == 0
	) {
	    if(
	       mode_info(&fb->info) == 0
	       &&
	       (fd = open("/dev/mem", O_RDWR, 0))
	    ) {
		fb->ptr = mmap(0,
			       fb->info.vi_buffer_size,
			       PROT_READ|PROT_WRITE,
			       MAP_NOCORE|MAP_SHARED,
			       fd,
			       fb->info.vi_buffer);
		close(fd);
		if(fb->ptr != MAP_FAILED) return fb;
	    }
	    set_mode(fb->old_info.vi_mode);		
	}
	free(fb);
    }
    return NULL;
}

// free an existing framebuffer struct and reset the hardware
void fb_close(framebuffer_t *fb) {
    if(fb == NULL) return;
    munmap(fb->ptr, fb->info.vi_buffer_size);
    set_mode(fb->old_info.vi_mode);
    free(fb);
}
