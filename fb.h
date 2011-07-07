#include <sys/fbio.h>
#include <sys/consio.h>

struct framebuffer {
    vid_info_t		con_info;
    struct video_info	old_info;
    struct video_info	info;
    void*		ptr;
};
typedef struct framebuffer framebuffer_t;

framebuffer_t* fb_open(int mode);

void fb_close(framebuffer_t *fb);
