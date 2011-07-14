#include <sys/fbio.h>
#include <sys/consio.h>

struct vesa_info {
    /* mandatory fields */
    uint8_t            v_sig[4];       /* VESA */
    uint16_t           v_version;      /* ver in BCD */
    uint32_t           v_oemstr;       /* OEM string */
    uint32_t           v_flags;        /* flags */
#define V_DAC8          (1<<0)
#define V_NONVGA        (1<<1)
#define V_SNOW          (1<<2)
    uint32_t           v_modetable;    /* modes */
    uint16_t           v_memsize;      /* in 64K */
    /* 2.0 */
    uint16_t           v_revision;     /* software rev */
    uint32_t           v_venderstr;    /* vender */
    uint32_t           v_prodstr;      /* product name */
    uint32_t           v_revstr;       /* product rev */
    uint8_t            v_strach[222];
    uint8_t            v_oemdata[256];
} __packed;
typedef struct vesa_info vesa_info_t;

struct framebuffer {
    int			old_vty;
    int			old_mode;
    video_info_t	info;
    vesa_info_t 	vesa;
    void*		work_buffer;
    void*		show_buffer;
    void*		mapped_vram;
};
typedef struct framebuffer framebuffer_t;

void fb_close(framebuffer_t* fb);

int fb_flip(framebuffer_t* fb);

framebuffer_t* fb_open(int mode, int db, int vty);
