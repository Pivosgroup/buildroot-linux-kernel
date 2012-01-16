#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/soundcard.h>
#include <linux/timer.h>
#include <linux/debugfs.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/control.h>
#include <mach/am_regs.h>
#include <mach/pinmux.h>

#ifdef CONFIG_AMLOGIC_BOARD_APOLLO_H_LINUX
#include <mach/gpio.h>    
#endif

#include "aml_pcm.h"
#include "aml_audio_hw.h"
#include "aml_alsa_common.h"

#include "tone/tone_32k.h"
#include "tone/tone_44k.h"
#include "tone/tone_48k.h"

#define AOUT_EVENT_PREPARE  0x1
extern int aout_notifier_call_chain(unsigned long val, void *v);

audio_mixer_control_t audio_mixer_control;
audio_tone_control_t audio_tone_control;

static unsigned int rates[] = {
    32000, 44100, 48000,
};

static unsigned int capture_rates[] = {
	8000, 16000,
};

static struct snd_pcm_hw_constraint_list hw_playback_rates = {
    .count = ARRAY_SIZE(rates),
    .list = rates,
    .mask = 0,
};

static struct snd_pcm_hw_constraint_list hw_capture_rates = {
	.count = ARRAY_SIZE(capture_rates),
	.list = capture_rates,
	.mask = 0,
};

static char *id = NULL;
static struct platform_device *device;
struct timer_list timer;
static audio_output_config_t config;

int get_mixer_output_volume(void)
{
    int val;
    val = audio_mixer_control.output_volume;
    return val;
}

int set_mixer_output_volume(int volume)
{
    audio_mixer_control.output_volume = volume;
    return 0;
}

static int adapt_speed(int speed)
{

    if (speed >= (44100 + 48000) / 2) {
        speed = 48000;
    } else if (speed >= (32000 + 44100) / 2) {
        speed = 44100;
    } else {
        speed = 32000;
    }
    return speed;
}

static void normalize_speed_for_pcm(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime;
    aml_audio_t *chip;
    audio_stream_t *audio_stream;
    int stream_id;

    chip = snd_pcm_substream_chip(substream);
    stream_id = substream->pstr->stream;
    audio_stream = &chip->s[stream_id];
    runtime = substream->runtime;

    runtime->rate = adapt_speed(runtime->rate);

    switch (runtime->rate) {

    case 192000:
        audio_stream->sample_rate = AUDIO_CLK_FREQ_192;
        break;
    case 176400:
        audio_stream->sample_rate = AUDIO_CLK_FREQ_1764;
        break;
    case 96000:
        audio_stream->sample_rate = AUDIO_CLK_FREQ_96;
        break;
    case 88200:
        audio_stream->sample_rate = AUDIO_CLK_FREQ_882;
        break;
    case 48000:
        audio_stream->sample_rate = AUDIO_CLK_FREQ_48;
	 audio_tone_control.tone_data = (unsigned short *)ToneData_48;
	 audio_tone_control.tone_source= (unsigned short *)ToneData_48;
	 audio_tone_control.tone_data_len = ToneDataLen_48;
        break;
    case 44100:
        audio_stream->sample_rate = AUDIO_CLK_FREQ_441;
	 audio_tone_control.tone_data = (unsigned short *)ToneData_44;
	 audio_tone_control.tone_source= (unsigned short *)ToneData_44;
	 audio_tone_control.tone_data_len = ToneDataLen_44;
        break;
    case 32000:
        audio_stream->sample_rate = AUDIO_CLK_FREQ_32;
	 audio_tone_control.tone_data = (unsigned short *)ToneData_32;
	 audio_tone_control.tone_source= (unsigned short *)ToneData_32;
	 audio_tone_control.tone_data_len = ToneDataLen_32;
        break;
    default:
        audio_stream->sample_rate = AUDIO_CLK_FREQ_441;
    }
    dug_printk("audio_stream->sample_rate= %d", audio_stream->sample_rate);
}

static struct snd_pcm_hardware snd_aml_pcm_playback = {
    .info = (SNDRV_PCM_INFO_INTERLEAVED
             | SNDRV_PCM_INFO_BLOCK_TRANSFER
             | SNDRV_PCM_INFO_MMAP 
             | SNDRV_PCM_INFO_MMAP_VALID
             |SNDRV_PCM_INFO_PAUSE
//          | SNDRV_PCM_INFO_RESUME
        ),
    .formats = SNDRV_PCM_FMTBIT_S16_LE,
    .rates = (0
              | SNDRV_PCM_RATE_32000
              | SNDRV_PCM_RATE_44100 
              | SNDRV_PCM_RATE_48000
        ),
    .rate_min = 32000,
    .rate_max = 48000,
    .channels_min = 1,
    .channels_max = 2,
    .buffer_bytes_max = 32 * 1024,
    .period_bytes_min = 64,
    .period_bytes_max = 8 * 1024,
    .periods_min = 2,
    .periods_max = 255,
    .fifo_size = 0,
};

static struct snd_pcm_hardware snd_aml_pcm_capture = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED
		  | SNDRV_PCM_INFO_BLOCK_TRANSFER
		  | SNDRV_PCM_INFO_MMAP
		  | SNDRV_PCM_INFO_MMAP_VALID
	         | SNDRV_PCM_INFO_PAUSE
//             | SNDRV_PCM_INFO_RESUME
	),
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.rates = (0
	           | SNDRV_PCM_RATE_8000
	           | SNDRV_PCM_RATE_16000
	),
	.rate_min = 8000,
	.rate_max = 16000,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = 32 * 1024,
	.period_bytes_min = 64,
	.period_bytes_max = 8 * 1024,
	.periods_min = 2,
	.periods_max = 255,
	.fifo_size = 0,
};

static void aml_pcm_timer_callback(unsigned long data)
{
    struct snd_pcm_substream *substream = (struct snd_pcm_substream *)data;
    aml_audio_t *chip = snd_pcm_substream_chip(substream);
    struct snd_pcm_runtime *runtime = substream->runtime;
    audio_stream_t *s;
    int stream_id;
    unsigned int last_ptr, size;

    stream_id = substream->pstr->stream;
    s = &chip->s[stream_id];
	
    if(s->active == 1){
    spin_lock(&s->dma_lock);
    last_ptr = read_i2s_rd_ptr();
    if (last_ptr < s->last_ptr) {
        size = runtime->dma_bytes + last_ptr - (s->last_ptr);
    } else {
        size = last_ptr - (s->last_ptr);
    }

    s->last_ptr = last_ptr;
    s->size += bytes_to_frames(substream->runtime, size);
    if (s->size >= runtime->period_size) {
        s->size %= runtime->period_size;
        spin_unlock(&s->dma_lock);
        snd_pcm_period_elapsed(substream);
        spin_lock(&s->dma_lock);
    }
    mod_timer(&timer, jiffies + 1);
    spin_unlock(&s->dma_lock);
    }else {
    mod_timer(&timer, jiffies + 1);
    }
}

static int snd_card_aml_audio_playback_open(struct snd_pcm_substream
                                            *substream)
{
    aml_audio_t *chip = snd_pcm_substream_chip(substream);
    struct snd_pcm_runtime *runtime = substream->runtime;
    audio_stream_t *audio_stream;
    int stream_id = substream->pstr->stream;
    int err;

    audio_stream = &chip->s[stream_id];
    chip->s[stream_id].stream = substream;

    if (stream_id == SNDRV_PCM_STREAM_PLAYBACK)
        runtime->hw = snd_aml_pcm_playback;
    //else
    //       runtime->hw = snd_aml_pcm_capture;
    if ((err =
         snd_pcm_hw_constraint_integer(runtime,
                                       SNDRV_PCM_HW_PARAM_PERIODS)) < 0)
        return err;
    if ((err =
         snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
                                    &hw_playback_rates)) < 0)
        return err;

/*
pinmux JTAG-
data0
tck
tdo 
tdi
reg[8],8,9,10,11

*/
//	set_mio_mux(8,0xf<<8);

    timer.function = &aml_pcm_timer_callback;
    timer.data = (unsigned long)substream;
    init_timer(&timer);
    return 0;

}

static int snd_card_aml_audio_playback_close(struct snd_pcm_substream
                                             *substream)
{

    aml_audio_t *chip = snd_pcm_substream_chip(substream);
    del_timer_sync(&timer);
    chip->s[substream->pstr->stream].stream = NULL;
    return 0;
}

static int snd_aml_audio_hw_params(struct snd_pcm_substream *substream,
                                   struct snd_pcm_hw_params *hw_params)
{
    int ret;
    struct snd_pcm_runtime *runtime;

    runtime = substream->runtime;
    ret =
        snd_pcm_lib_malloc_pages(substream,
                                 params_buffer_bytes(hw_params));

    if (ret < 0)
        return ret;

    runtime->dma_addr = virt_to_phys(runtime->dma_area);
    runtime->dma_area = ioremap_nocache(runtime->dma_addr, runtime->dma_bytes);
    printk(" snd_aml_audio_hw_params runtime->dma_addr 0x(%x)\n",
               (unsigned int)runtime->dma_addr);
    printk(" snd_aml_audio_hw_params runtime->dma_area 0x(%x)\n",
               (unsigned int)runtime->dma_area);
    printk(" snd_aml_audio_hw_params runtime->dma_bytes 0x(%x)\n",
               (unsigned int)runtime->dma_bytes);
    return ret;
}

static int snd_aml_audio_hw_free(struct snd_pcm_substream *substream)
{
    if(substream->runtime->dma_area)
        iounmap(substream->runtime->dma_area);
    return snd_pcm_lib_free_pages(substream);
}

static int snd_aml_audio_playback_prepare(struct snd_pcm_substream
                                          *substream)
{
    aml_audio_t *chip = snd_pcm_substream_chip(substream);
    audio_stream_t *s = &chip->s[substream->pstr->stream];
    _aiu_958_raw_setting_t set;

    normalize_speed_for_pcm(substream);
    //audio_set_clk(s->sample_rate, AUDIO_CLK_256FS);
    //config.clock = s->sample_rate;
   	audio_set_clk(s->sample_rate, AUDIO_CLK_256FS);       // XD: div by n+1 by n+1
	audio_dac_set(s->sample_rate);
	//audio_set_i2s_mode(config.i2s_mode);
    
    audio_set_aiubuf(substream->runtime->dma_addr,
                     substream->runtime->dma_bytes);
    audio_set_958outbuf(substream->runtime->dma_addr,
                        substream->runtime->dma_bytes);
    memset((void*)substream->runtime->dma_area,0,substream->runtime->dma_bytes);
    s->I2S_addr = substream->runtime->dma_addr;

    dug_printk("music channel=%d\n", substream->runtime->channels);
   // audio_set_clk(s->sample_rate, AUDIO_CLK_256FS);

    config.clock = s->sample_rate;
	
    
    //audio_i2s_unmute();
    //audio_util_set_dac_format(config.i2s_dac_mode);

    memset(&set, 0, sizeof(set));
    set.chan_stat = &config.chan_status;

    switch (config.clock) {
    case AUDIO_CLK_FREQ_192:
    case AUDIO_CLK_FREQ_96:
    case AUDIO_CLK_FREQ_48:
        set.chan_stat->chstat1_l &= 0xf0ff;
        set.chan_stat->chstat1_r &= 0xf0ff;
        set.chan_stat->chstat1_l |= 0x0200;
        set.chan_stat->chstat1_r |= 0x0200;
        break;

    case AUDIO_CLK_FREQ_1764:
    case AUDIO_CLK_FREQ_882:
    case AUDIO_CLK_FREQ_441:
        set.chan_stat->chstat1_l &= 0xf0ff;
        set.chan_stat->chstat1_r &= 0xf0ff;
        break;

    case AUDIO_CLK_FREQ_32:
        set.chan_stat->chstat1_l &= 0xf0ff;
        set.chan_stat->chstat1_r &= 0xf0ff;
        set.chan_stat->chstat1_l |= 0x0300;
        set.chan_stat->chstat1_r |= 0x0300;
        break;

    default:
        break;
    }
    audio_hw_set_958_mode(config.i958_mode, &set);

    aout_notifier_call_chain(AOUT_EVENT_PREPARE, substream);
    return 0;
}

static snd_pcm_uframes_t snd_aml_audio_playback_pointer(struct
                                                        snd_pcm_substream
                                                        *substream)
{
    aml_audio_t *chip;
    audio_stream_t *s;
    unsigned int addr1;
    unsigned int ptr;

    chip = snd_pcm_substream_chip(substream);
    s = &chip->s[substream->pstr->stream];
    ptr = read_i2s_rd_ptr();
    addr1 = ptr - (s->I2S_addr);

    return bytes_to_frames(substream->runtime, addr1);

}

static int snd_aml_audio_playback_trigger(struct snd_pcm_substream
                                          *substream, int cmd)
{
    aml_audio_t *chip = snd_pcm_substream_chip(substream);
    int stream_id = substream->pstr->stream;
    audio_stream_t *s = &chip->s[stream_id];
    int err = 0;

    spin_lock(&s->dma_lock);
    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
        s->active = 1;
        del_timer_sync(&timer);
        timer.expires = jiffies + 1;
        del_timer(&timer);
        add_timer(&timer);
        //flush_dcache_all();
        dug_printk
            ("snd_aml_audio_playback_trigger: pcm trigger! SNDRV_PCM_TRIGGER_START\n");
        audio_enable_ouput(1);
        break;

    case SNDRV_PCM_TRIGGER_STOP:
        s->active = 0;
        dug_printk
            ("snd_aml_audio_playback_trigger: pcm trigger! SNDRV_PCM_TRIGGER_STOP\n");
        audio_enable_ouput(0);
        break;

    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        s->active = 0;
        WRITE_MPEG_REG_BITS(AIU_MEM_I2S_CONTROL, 0, 2, 1);
        dug_printk
            ("snd_aml_audio_playback_trigger: pcm trigger! SNDRV_PCM_TRIGGER_PAUSE\n");
        break;

     case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        s->active = 1;
        WRITE_MPEG_REG_BITS(AIU_MEM_I2S_CONTROL, 1, 2, 1);
        dug_printk
            ("snd_aml_audio_playback_trigger: pcm trigger! SNDRV_PCM_TRIGGER_PAUSE_RELEASE\n");
        break;

    default:
        err = -EINVAL;
        break;
    }
    spin_unlock(&s->dma_lock);

    return err;
}

//int snd_aml_audio_playback_ack(struct snd_pcm_substream *substream)
//{
//printk("snd_aml_audio_playback_ack\n");
//      return 0;
//}
#ifdef DEBUG_TIME_TEST
int debug_time_value=0;
int debug_time_loop=0;
#endif
#define VOL_CTL(s) ( unsigned short)((unsigned int)(((signed short)(s))*(vol))>>VOLUME_SHIFT)
//#define VOL_CTL(s) s

static int snd_aml_audio_playback_copy(struct snd_pcm_substream *substream,
                                       int channel, snd_pcm_uframes_t pos,
                                       void __user * buf,
                                       snd_pcm_uframes_t count)
{
    unsigned short *tfrom, *to, *left, *right;
    int res = 0;
    int n;
    int i = 0, j = 0;
#ifdef DEBUG_TIME_TEST
	int debug_temp_value=0;
#endif
    register unsigned  int vol =(audio_mixer_control.output_volume*(1<<VOLUME_SHIFT))/VOLUME_SCALE;
    struct snd_pcm_runtime *runtime = substream->runtime;
    char *hwbuf = runtime->dma_area + frames_to_bytes(runtime, pos);
    //snd_assert(runtime->dma_area, return -EFAULT);
    //      if (copy_from_user(hwbuf, buf, frames_to_bytes(runtime, frames)))
    //              return -EFAULT;
#ifdef DEBUG_TIME_TEST
    debug_time_loop++;
	if(debug_time_loop>500)
	{
		debug_temp_value=DSP_RD(DSP_TEST_TIME_VALUE);
		if(debug_temp_value!=debug_time_value)
		{	
			debug_time_value=debug_temp_value;
			printk("[DEBUG TIME TEST]:+++++++++++++++++debug_time_value=%d\n",debug_time_value);
		}
		debug_time_loop=0;
	}
#endif
    tfrom = (unsigned short *)buf;
    to = (unsigned short *)hwbuf;
    n = frames_to_bytes(runtime, count);

    if (access_ok(VERIFY_READ, buf, frames_to_bytes(runtime, count)))
    { 
	    left = to;
	    right = to + 16;
	    if (pos % 16) {
	        printk("audio data unligned\n");
	    }
	    //if(tfrom)
	    for (j = 0; j < n; j += 64) {
	        for (i = 0; i < 16; i++) {
			  if(audio_tone_control.tone_flag== 0){
	                    *left++ = VOL_CTL(*tfrom++) ;
	                    *right++ = VOL_CTL(*tfrom++);
			  }else{
			      *left = VOL_CTL(*tfrom++) + VOL_CTL(*audio_tone_control.tone_data++);
				if(*left > 65536) *left = 65536;
				*right = VOL_CTL(*tfrom++) + VOL_CTL(*audio_tone_control.tone_data++);
				if(*right > 65536) *right = 65536;
				left++;
				right++;
				audio_tone_control.tone_count +=4;
				if(audio_tone_control.tone_count >= audio_tone_control.tone_data_len){
					audio_tone_control.tone_flag = 0;
					audio_tone_control.tone_data = audio_tone_control.tone_source;
					audio_tone_control.tone_count = 0;
				}
			  }
	         }
	        left += 16;
	        right += 16;
	    }
	  // dma_cache_wback((unsigned long)hwbuf,n);
    	}
   else
   	{
   	res=-EFAULT;
   	}
   
    return res;
}

static int snd_aml_audio_playback_silence(struct snd_pcm_substream *substream,
						int channel, snd_pcm_uframes_t pos,
						snd_pcm_uframes_t count)
{
	char *ppos;
	int n;
	struct snd_pcm_runtime *runtime = substream->runtime;

	n = frames_to_bytes(runtime, count);
	ppos = runtime->dma_area + frames_to_bytes(runtime, pos);
	memset(ppos, 0, n);
	//snd_pcm_format_set_silence(runtime->format, ppos, count);
	
	return 0;
}

static int snd_card_aml_audio_capture_open(struct snd_pcm_substream *substream)
{
	aml_audio_t *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int stream_id = substream->pstr->stream;
	int err = -1;
	
	chip->s[stream_id].stream = substream;

	if (stream_id == SNDRV_PCM_STREAM_CAPTURE)
		runtime->hw = snd_aml_pcm_capture;
	else 
		return err;

	if ((err = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS)) < 0)
		return err;

	if ((err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE, &hw_capture_rates)) < 0)
		return err;

	return 0;
}

static int snd_card_aml_audio_capture_close(struct snd_pcm_substream *substream)
{
	aml_audio_t *chip = snd_pcm_substream_chip(substream);
	int stream_id = substream->pstr->stream;
	
	chip->s[stream_id].stream = NULL;
	return 0;
}

static int snd_aml_audio_capture_prepare(struct snd_pcm_substream *substream)
{
//	aml_audio_t *chip = snd_pcm_substream_chip(substream);
//	int stream_id = substream->pstr->stream;
//	audio_stream_t *s = &chip->s[stream_id];
	return 0;
}

static int snd_aml_audio_capture_trigger(struct snd_pcm_substream *substream, int cmd)
{
//	aml_audio_t *chip = snd_pcm_substream_chip(substream);
//	int stream_id = substream->pstr->stream;
//	audio_stream_t *s = &chip->s[stream_id];
	int err = 0;
	
	switch(cmd) {
		case SNDRV_PCM_TRIGGER_START:
			break;
			
		case SNDRV_PCM_TRIGGER_STOP:
			break;
			
		default:
			err = -EINVAL;
			break;
	}
	
	return err;
}

static snd_pcm_uframes_t snd_aml_audio_capture_pointer(struct snd_pcm_substream *substream)
{
//	aml_audio_t *chip = snd_pcm_substream_chip(substream);
//	int stream_id = substream->pstr->stream;
//	audio_stream_t *s = &chip->s[stream_id];
    return 0;
}

static struct snd_pcm_ops snd_card_aml_audio_playback_ops = {
    .open = snd_card_aml_audio_playback_open,
    .close = snd_card_aml_audio_playback_close,
    .ioctl = snd_pcm_lib_ioctl,
    .hw_params = snd_aml_audio_hw_params,
    .hw_free = snd_aml_audio_hw_free,
    .prepare = snd_aml_audio_playback_prepare,
    .trigger = snd_aml_audio_playback_trigger,
    .pointer = snd_aml_audio_playback_pointer,
    //.ack=snd_aml_audio_playback_ack,
    .copy = snd_aml_audio_playback_copy,
   // .silence = snd_aml_audio_playback_silence,
};

static struct snd_pcm_ops snd_card_aml_audio_capture_ops = {
    .open = snd_card_aml_audio_capture_open,
    .close = snd_card_aml_audio_capture_close,
    .ioctl = snd_pcm_lib_ioctl,
    .hw_params = snd_aml_audio_hw_params,
    .hw_free = snd_aml_audio_hw_free,
    .prepare = snd_aml_audio_capture_prepare,
    .trigger = snd_aml_audio_capture_trigger,
    .pointer = snd_aml_audio_capture_pointer,
};

static void aml_pcm_mixer_controls_init(aml_audio_t * aml_audio)
{
    audio_mixer_control_t *audio_control;

    audio_control = &audio_mixer_control;

    memset(audio_control, 0, sizeof(audio_mixer_control_t));

    audio_control->output_devide = SOUND_MASK_VOLUME | SOUND_MASK_PCM;
    audio_control->output_volume = 255;
}

static void aml_pcm_audio_init(aml_audio_t * aml_audio)
{

    aml_audio->s[SNDRV_PCM_STREAM_PLAYBACK].id = "Audio out";
    aml_audio->s[SNDRV_PCM_STREAM_PLAYBACK].stream_id =
        SNDRV_PCM_STREAM_PLAYBACK;
    aml_audio->s[SNDRV_PCM_STREAM_CAPTURE].id = "Audio in";
    aml_audio->s[SNDRV_PCM_STREAM_CAPTURE].stream_id =
	 SNDRV_PCM_STREAM_CAPTURE;

    config.clock = AUDIO_CLK_FREQ_48;
    config.i2s_mode = AIU_I2S_MODE_2x16;
    config.i2s_dac_mode = AUDIO_ALGOUT_DAC_FORMAT_DSP;

    config.i958_mode = AIU_958_MODE_PCM16;
    config.chan_status.chstat0_l = 0x1900;
    config.chan_status.chstat1_l = 0x0000;
    config.chan_status.chstat0_r = 0x1900;
    config.chan_status.chstat1_r = 0x0000;

}

static int __init snd_card_aml_audio_pcm(aml_audio_t * aml_audio,
                                         int device)
{

    struct snd_pcm *pcm;
    int err;

    if ((err =
         snd_pcm_new(aml_audio->card, "AML", device, 1, 1, &pcm)) < 0) {

        return err;
    }

    snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_CONTINUOUS,
                                          snd_dma_continuous_data
                                          (GFP_KERNEL), 32 * 1024,
                                          32 * 1024);

    snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
                    &snd_card_aml_audio_playback_ops);

    snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,  
                    &snd_card_aml_audio_capture_ops);

    pcm->private_data = aml_audio;
    pcm->info_flags = 0;
    strcpy(pcm->name, "AML PCM");
    aml_audio->pcm = pcm;
    aml_pcm_audio_init(aml_audio);

    aml_pcm_mixer_controls_init(aml_audio);
    memset(&audio_tone_control, 0, sizeof(audio_tone_control_t));

    return 0;
}

static int __init aml_alsa_audio_probe(struct platform_device *dev)
{
    int err;
    struct snd_card *card;
    aml_audio_t *chip;

   // card = snd_card_new(-1, id, THIS_MODULE, sizeof(aml_audio_t));
	err=snd_card_create(SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1,
			      THIS_MODULE,sizeof(aml_audio_t), &card);
    if (err <0) {
        return -ENOMEM;
    }

    chip = card->private_data;
    chip->card = card;

    if ((err = snd_card_aml_audio_pcm(chip, 0)) < 0)

        goto nodev;

    if (0 == aml_alsa_create_ctrl(card, &audio_mixer_control))
        printk("control ALSA component registered!\n");

    spin_lock_init(&(chip->s[0].dma_lock));

    strcpy(card->driver, "AML");
    strcpy(card->shortname, "PCM-audio");
    sprintf(card->longname, "AML with PCM");

    snd_card_set_dev(card, &dev->dev);

    audio_i2s_unmute();
    if ((err = snd_card_register(card)) == 0) {

        pr_debug(KERN_INFO "AML audio support initialized\n");
        platform_set_drvdata(dev, card);

        return 0;
    }

  nodev:
    snd_card_free(card);
    return err;
}

static int aml_alsa_audio_remove(struct platform_device *dev)
{
    snd_card_free(platform_get_drvdata(dev));
    platform_set_drvdata(dev, NULL);

    return 0;
}
#ifdef CONFIG_DEBUG_FS

static struct dentry *debugfs_root;
static struct dentry *debugfs_regs;

static int regs_open_file(struct inode *inode, struct file *file)
{
	return 0;
}

/**
 *	cat regs
 */
static ssize_t regs_read_file(struct file *file, char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	ssize_t ret;
	char *buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
		
	ret = sprintf(buf, "Usage: \n"
										 "	echo base reg val >regs\t(set the register)\n"
										 "	echo base reg >regs\t(show the register)\n"
										 "	base -> c(cbus), x(aix), p(apb), h(ahb) \n"
									);
		
	if (ret >= 0)
		ret = simple_read_from_buffer(user_buf, count, ppos, buf, ret);
	kfree(buf);	
	
	return ret;
}

static int read_regs(char base, int reg)
{
	int val = 0;
	switch(base){
		case 'c':
			val = READ_CBUS_REG(reg);
			break;
		case 'x':
			val = READ_AXI_REG(reg);
			break;
		case 'p':
			val = READ_APB_REG(reg);
			break;
		case 'h':
			val = READ_AHB_REG(reg);
			break;
		default:
			break;
	};
	printk("\tReg %x = %x\n", reg, val);
	return val;
}

static void write_regs(char base, int reg, int val)
{
	switch(base){
		case 'c':
			WRITE_CBUS_REG(reg, val);
			break;
		case 'x':
			WRITE_AXI_REG(reg, val);
			break;
		case 'p':
			WRITE_APB_REG(reg, val);
			break;
		case 'h':
			WRITE_AHB_REG(reg, val);
			break;
		default:
			break;
	};
	printk("Write reg:%x = %x\n", reg, val);
}
static ssize_t regs_write_file(struct file *file,
		const char __user *user_buf, size_t count, loff_t *ppos)
{
	char buf[32];
	int buf_size = 0;
	char *start = buf;
	unsigned long reg, value;
	int step = 1;
	char base;
	
	buf_size = min(count, (sizeof(buf)-1));
	
	if (copy_from_user(buf, user_buf, buf_size))
		return -EFAULT;
	buf[buf_size] = 0;
	while (*start == ' ')
		start++;
		
	base = *start;
	start ++;
	if(!(base =='c' || base == 'x' || base == 'p' || base == 'h')){
		return -EINVAL;
	}
	
	while (*start == ' ')
		start++;
		
	reg = simple_strtoul(start, &start, 16);
	
	while (*start == ' ')
		start++;
		
	if (strict_strtoul(start, 16, &value))
	{
			read_regs(base, reg);
			return -EINVAL;
	}
	
	write_regs(base, reg, value);
	
	return buf_size;
}

static const struct file_operations regs_fops = {
	.open = regs_open_file,
	.read = regs_read_file,
	.write = regs_write_file,
};

static void aml_pcm_init_debugfs()
{
		debugfs_root = debugfs_create_dir("aml_pcm_debug",NULL);
		if (IS_ERR(debugfs_root) || !debugfs_root) {
			printk("aml_pcm: Failed to create debugfs directory\n");
			debugfs_root = NULL;
		}
		
		debugfs_regs = debugfs_create_file("regs", 0644, debugfs_root, NULL, &regs_fops);
		if(!debugfs_regs){
			printk("aml_pcm: Failed to create debugfs file\n");
		}
}
static void aml_pcm_cleanup_debugfs()
{
	debugfs_remove_recursive(debugfs_root);
}
#else
static void aml_pcm_init_debugfs()
{
}
static void aml_pcm_cleanup_debugfs()
{
}
#endif


#define aml_ALSA "aml_ALSA"

static struct platform_driver aml_alsa_audio_driver = {

    .probe = aml_alsa_audio_probe,
    .remove = aml_alsa_audio_remove,
#ifdef CONFIG_PM
    //.suspend = snd_aml_audio_suspend,
    //.resume = snd_aml_audio_resume,
#endif
    .driver = {
               .name = "aml_ALSA",
               },
};

static int __init aml_alsa_audio_init(void)
{
    int err;
	
	aml_pcm_init_debugfs();
		
    if ((err = platform_driver_register(&aml_alsa_audio_driver)) < 0)
        return err;

    device = platform_device_register_simple(aml_ALSA, -1, NULL, 0);

    if (!IS_ERR(device)) {
        if (platform_get_drvdata(device))
            return 0;

        platform_device_unregister(device);
        platform_driver_unregister(&aml_alsa_audio_driver);

        err = -ENODEV;
    } else {
        err = PTR_ERR(device);

        platform_driver_unregister(&aml_alsa_audio_driver);
    }

    return err;
}

static void __exit aml_alsa_audio_exit(void)
{
    platform_driver_unregister(&aml_alsa_audio_driver);
    platform_device_unregister(device);

}

/*The sound driver must later than the 
sound system it self
*/
late_initcall(aml_alsa_audio_init);
module_exit(aml_alsa_audio_exit);

MODULE_AUTHOR("AMLogic, Inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("AML driver for ALSA");
MODULE_SUPPORTED_DEVICE("{{AML APOLLO}}");

module_param(id, charp, 0444);
MODULE_PARM_DESC(id, "ID string for AML soundcard.");
