#ifndef _CODEC_MESSAGE_HEADERS
#define _CODEC_MESSAGE_HEADERS


#define SUB_FMT_VALID 			(1<<1)
#define CHANNEL_VALID 			(1<<2)	
#define SAMPLE_RATE_VALID     	(1<<3)	
#define DATA_WIDTH_VALID     	(1<<4)	
struct frame_fmt
{

    int valid;
    int sub_fmt;
    int channel_num;
    int sample_rate;
    int data_width;
    int reversed[3];/*for cache aligned 32 bytes*/
    int format;
    unsigned int total_byte_parsed;
    unsigned int total_sample_decoded;
    unsigned int bps;
};


struct frame_info
{

    int len;
    unsigned long  offset;/*steam start to here*/
    unsigned long  buffered_len;/*data buffer in  dsp,pcm datalen*/
    int reversed[1];/*for cache aligned 32 bytes*/
};

struct dsp_working_info
{
	int status;
	int sp;
	int pc;
	int ilink1;
	int ilink2;
	int blink;
	int jeffies;
	int out_wp;
	int out_rp;
	int buffered_len;//pcm buffered at the dsp side
	int es_offset;//stream read offset since start decoder
	int reserved[5];
};
#endif

