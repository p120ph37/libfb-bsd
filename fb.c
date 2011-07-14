#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include "fb.h"
#include <stdio.h>
#include <errno.h>

/* 
 * The fbio.h interface does not provide a method to accutately
 * determine the VRAM size.  Since we will be mapping the physical
 * VRAM buffer, which will require root access anyway, we will
 * use the kvm.h interface to access the kernel's internal symbols
 * in order to get this information.
 * 
 * ...would be nice if this stuff was preseted via the fbio API,
 * wouldn't it?  But that would be too easy!
 */
#include "ksym.h"


/***
 ***  Private functions
 ***/

// set console to desired mode
static int set_mode(int mode) {

    if(mode >= M_VESA_BASE) {
	if(ioctl(0, _IO('V', mode - M_VESA_BASE), NULL) == -1) return -1;
    } else {
 	if(ioctl(0, _IO('S', mode), NULL) == -1) return -1;
    }

    return 0;
}

// switch to the desired VTY
// (0 to use current vty, -1 to select first unused)
static int switch_vty(int vty) {

    // < -1 is an error
    if(vty < -1) { errno = EINVAL; return -1; }

    // 0 is a no-op
    if(vty == 0) return 0;

    // -1 auto-selects first unused
    if(vty == -1) {
	if(ioctl(0, VT_OPENQRY, &vty) == -1) return -1;
    }

    // now activate the VTY
    if(ioctl(0, VT_ACTIVATE, &vty) == -1) return -1;	
    ioctl(0, VT_WAITACTIVE, &vty); // non-critical
    
    return 0;
}


/***
 ***  Public functions
 ***/

// resets the vty and deallocates the fb struct
void fb_close(framebuffer_t* fb) {
    int i;
    
    set_mode(fb->old_mode);
    i = 0; ioctl(0, VT_LOCKSWITCH, &i);
    switch_vty(fb->old_vty);
    if(fb->mapped_vram) {
	munmap(fb->mapped_vram, fb->vesa.v_memsize * 64 * 1024);
    }
    free(fb);
    
    return;
}

// flips video pages and buffer pointers (no-op if not double-buffered)
int fb_flip(framebuffer_t* fb) {
    video_display_start_t vds;
    void* temp;
    
    // no-op if using single buffer
    if(fb->show_buffer == fb->work_buffer) return 0;

    // switch video pages
    ioctl(0, FBIO_GETDISPSTART, &vds);
    vds.x = 0; vds.y ^= fb->info.vi_height;
    if(ioctl(0, FBIO_SETDISPSTART, &vds) == -1) return -1;

    // swap the pointers
    temp = fb->work_buffer; fb->work_buffer = fb->show_buffer; fb->show_buffer = temp;
    
    return 0;
}

// Puts the VTY into the requested mode and returns a pointer to the framebuffer struct.
// For a list of available modes, try: vidcontrol -i mode
// If you need to find the list programmatically, see the vidcontrol.c source.
// Returns a NULL pointer if there was an error (check errno for details)
// Remember to call fb_close later to free the resources and reset the hardware.
// db = 1 for double-buffering enabled (will fall back on disabled if insufficient memory)
// db = 0 for double-buffering disabled
// vty = console vty to switch to when in graphics mode (0 to use current vty, -1 to use first unused)
framebuffer_t* fb_open(int mode, int db, int vty) {
    framebuffer_t* fb;
    int fd;
    int i;

    if((fb = calloc(sizeof(framebuffer_t), 1)) == NULL)
      { return NULL; }
    
    // Gather info
    fb->info.vi_mode = mode;
    if(
       ioctl(0, VT_GETACTIVE, &fb->old_vty)
       ||
       ioctl(0, FBIO_GETMODE, &fb->old_mode)
       ||
       ioctl(0, FBIO_MODEINFO, &fb->info)
       ||
       get_ksym("vesa_adp_info", &fb->vesa, sizeof(fb->vesa))
      ) {
	free(fb);
	return NULL;
    }

    // Start changing settings
    if(
       switch_vty(vty)
       ||
       set_mode(fb->info.vi_mode)
      ) {
	fb_close(fb);
	return NULL;
    }
    
    // this really ought to be VT_SETMODE(VT_PROCESS)
    i = 1; ioctl(0, VT_LOCKSWITCH, &i);
    
    // access via the /dev/mem path.  NB this requires root.
    if((fd = open("/dev/mem", O_RDWR, 0)) == -1) {
	fb_close(fb);
	return NULL;
    }

    // map it based on the physical (not virtual) address
    fb->mapped_vram = mmap(0,
			   fb->vesa.v_memsize * 64 * 1024,
			   PROT_READ|PROT_WRITE,
			   MAP_NOCORE|MAP_SHARED,
			   fd,
			   fb->info.vi_buffer);
    close(fd);
    if(fb->mapped_vram == MAP_FAILED) {
	fb->mapped_vram = NULL;
	fb_close(fb);
	return NULL;
    }

    // set pointers to the buffers in the mmapped VRAM
    fb->show_buffer = fb->mapped_vram;
    if(db == 0 || fb->vesa.v_memsize * 64 * 1024 < fb->info.vi_buffer_size * 2) {
	fb->work_buffer = fb->show_buffer; // single buffer
    } else {
	fb->work_buffer = fb->show_buffer + fb->info.vi_buffer_size; // double buffer
    }

    return fb;
}
