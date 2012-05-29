#ifndef PPMGR_DEV_INCLUDE_H
#define PPMGR_DEV_INCLUDE_H

#define PPMGR_VIDEO_2_VLAYER 0
#define PPMGR_VIDEO_2_V4L2 1
typedef  struct {
	struct class 		*cla;
	struct device		*dev;
	char  			name[20];
	unsigned int 		open_count;
	int	 			major;
	unsigned  int 		dbg_enable;
	char* buffer_start;
	unsigned int buffer_size;
	
	unsigned angle;
	unsigned orientation;
	unsigned videoangle; 

	int mirror_mode;
	int bypass;
	int disp_width;
	int disp_height;
#ifdef CONFIG_MIX_FREE_SCALE
	int ppscaler_flag;
	int scale_h_start;
	int scale_h_end;
	int scale_v_start;
	int scale_v_end;
#endif
	const vinfo_t *vinfo;
	int left;
	int top;
	int width;
	int height;
	
	int video_out;
}ppmgr_device_t;

extern ppmgr_device_t  ppmgr_device;

#define PPMGR_MIRROR_MODE_DISABLE     0
#define PPMGR_MIRROR_MODE_X_DIR         1
#define PPMGR_MIRROR_MODE_Y_DIR         2
#define PPMGR_MIRROR_MODE_ALL             3
#define PPMGR_MIRROR_MODE_MAX            4

#endif /* PPMGR_DEV_INCLUDE_H. */
