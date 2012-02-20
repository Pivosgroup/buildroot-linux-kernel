#ifndef __AML_AUDIO_HW_H__
#define __AML_AUDIO_HW_H__

#if defined (CONFIG_ARCH_MESON) || defined (CONFIG_ARCH_MESON2)

/* assumming PLL source is 24M */

#define AUDIO_384FS_PLL_192K        0x507d  /* 36.864M */
#define AUDIO_384FS_PLL_192K_MUX    12
#define AUDIO_384FS_CLK_192K        0x5eb

#define AUDIO_384FS_PLL_176K        0x0e7c  /* 33.8688M */
#define AUDIO_384FS_PLL_176K_MUX    25
#define AUDIO_384FS_CLK_176K        0x5eb

#define AUDIO_384FS_PLL_96K         0x507d  /* 36.864M */
#define AUDIO_384FS_PLL_96K_MUX     12
#define AUDIO_384FS_CLK_96K         0x5ef

#define AUDIO_384FS_PLL_88K         0x0e7c  /* 33.8688M */
#define AUDIO_384FS_PLL_88K_MUX     25
#define AUDIO_384FS_CLK_88K         0x5ef

#define AUDIO_384FS_PLL_48K         0x487d  /* 18.432M */
#define AUDIO_384FS_PLL_48K_MUX     12
#define AUDIO_384FS_CLK_48K_AC3     0x5ed
#define AUDIO_384FS_CLK_48K         0x5ef

#define AUDIO_384FS_PLL_44K         0x0aa3  /* 16.9344M */
#define AUDIO_384FS_PLL_44K_MUX     23
#define AUDIO_384FS_CLK_44K         0x5ef

#define AUDIO_384FS_PLL_32K         0x1480  /* 12.288M */
#define AUDIO_384FS_PLL_32K_MUX     24
#define AUDIO_384FS_CLK_32K         0x5ef

#define AUDIO_384FS_DAC_CFG         0x6

#define AUDIO_256FS_PLL_192K        0x0a53  /* 24.576M */
#define AUDIO_256FS_PLL_192K_MUX    17
#define AUDIO_256FS_CLK_192K        0x5c7

#define AUDIO_256FS_PLL_176K        0x0eba  /* 22.5792M */
#define AUDIO_256FS_PLL_176K_MUX    25
#define AUDIO_256FS_CLK_176K        0x5c7

#define AUDIO_256FS_PLL_96K         0x0a53  /* 24.576M */
#define AUDIO_256FS_PLL_96K_MUX     17
#define AUDIO_256FS_CLK_96K         0x5db

#define AUDIO_256FS_PLL_88K         0x0eba  /* 22.5792M */
#define AUDIO_256FS_PLL_88K_MUX     25
#define AUDIO_256FS_CLK_88K         0x5db

#define AUDIO_256FS_PLL_48K         0x08d3  /* 12.288M */
#define AUDIO_256FS_PLL_48K_MUX     27
#define AUDIO_256FS_CLK_48K_AC3     0x5d9
#define AUDIO_256FS_CLK_48K         0x5db

#define AUDIO_256FS_PLL_44K         0x06b9  /* 11.2896M */
#define AUDIO_256FS_PLL_44K_MUX     29
#define AUDIO_256FS_CLK_44K         0x5db

#define AUDIO_256FS_PLL_32K         0x4252  /* 8.192M */
#define AUDIO_256FS_PLL_32K_MUX     14
#define AUDIO_256FS_CLK_32K         0x5db
#define AUDIO_256FS_DAC_CFG         0x7

#endif

typedef struct {
    unsigned short pll;
    unsigned short mux;
    unsigned short devisor;
} _aiu_clk_setting_t;

typedef struct {
    unsigned short chstat0_l;
    unsigned short chstat1_l;
    unsigned short chstat0_r;
    unsigned short chstat1_r;
} _aiu_958_channel_status_t;

typedef struct {
    /* audio clock */
    unsigned short clock;
    /* analog output */
    unsigned short i2s_mode;
    unsigned short i2s_dac_mode;
    unsigned short i2s_preemphsis;
    /* digital output */
    unsigned short i958_buf_start_addr;
    unsigned short i958_buf_blksize;
    unsigned short i958_int_flag;
    unsigned short i958_mode;
    unsigned short i958_sync_mode;
    unsigned short i958_preemphsis;
    unsigned short i958_copyright;
    unsigned short bpf;
    unsigned short brst;
    unsigned short length;
    unsigned short paddsize;
    _aiu_958_channel_status_t chan_status;
} audio_output_config_t;

typedef struct {
    unsigned short int_flag;
    unsigned short bpf;
    unsigned short brst;
    unsigned short length;
    unsigned short paddsize;
    _aiu_958_channel_status_t *chan_stat;
} _aiu_958_raw_setting_t;

#define AUDIO_CLK_256FS             0
#define AUDIO_CLK_384FS             1

#define AUDIO_CLK_FREQ_192  0
#define AUDIO_CLK_FREQ_1764 1
#define AUDIO_CLK_FREQ_96   2
#define AUDIO_CLK_FREQ_882  3
#define AUDIO_CLK_FREQ_48   4
#define AUDIO_CLK_FREQ_441  5
#define AUDIO_CLK_FREQ_32   6

#define AUDIO_CLK_FREQ_8		7
#define AUDIO_CLK_FREQ_11		8
#define AUDIO_CLK_FREQ_12		9
#define AUDIO_CLK_FREQ_16		10
#define AUDIO_CLK_FREQ_22		11
#define AUDIO_CLK_FREQ_24		12


#define AIU_958_MODE_RAW    0
#define AIU_958_MODE_PCM16  1
#define AIU_958_MODE_PCM24  2
#define AIU_958_MODE_PCM32  3
#define AIU_958_MODE_PCM_RAW  4

#define AIU_I2S_MODE_PCM16   0
#define AIU_I2S_MODE_PCM24   2
#define AIU_I2S_MODE_PCM32   3

#define AUDIO_ALGOUT_DAC_FORMAT_DSP             0
#define AUDIO_ALGOUT_DAC_FORMAT_LEFT_JUSTIFY    1

extern unsigned ENABLE_IEC958;
extern unsigned IEC958_MODE;
extern unsigned I2S_MODE;

void audio_set_aiubuf(u32 addr, u32 size);
void audio_set_958outbuf(u32 addr, u32 size, int flag);
void audio_in_i2s_set_buf(u32 addr, u32 size);
void audio_in_spdif_set_buf(u32 addr, u32 size);
void audio_in_i2s_enable(int flag);
void audio_in_spdif_enable(int flag);
unsigned int audio_in_i2s_rd_ptr(void);
unsigned int audio_in_i2s_wr_ptr(void);
void audio_set_i2s_mode(u32 mode);
void audio_set_clk(unsigned freq, unsigned fs_config);
void audio_enable_ouput(int flag);
unsigned int read_i2s_rd_ptr(void);
void audio_i2s_unmute(void);
void audio_i2s_mute(void);
void audio_util_set_dac_format(unsigned format);
void audio_set_958_mode(unsigned mode, _aiu_958_raw_setting_t * set);
unsigned int read_i2s_mute_swap_reg(void);
void audio_i2s_swap_left_right(unsigned int flag);
int audio_dac_set(unsigned freq);
int if_audio_out_enable(void);
int if_audio_in_i2s_enable(void);
void audio_hw_958_enable(unsigned flag);

#define APB_ADAC_RESET                		(0x5000+0x00*4)
#define APB_ADAC_LATCH                		(0x5000+0x01*4)
#define APB_ADAC_CLOCK                		(0x5000+0x02*4)
#define APB_ADAC_I2S_CONFIG_REG1      		(0x5000+0x0c*4)
#define APB_ADAC_I2S_CONFIG_REG2      		(0x5000+0x0d*4)
#define APB_ADAC_POWER_CTRL_REG1      		(0x5000+0x10*4)
#define APB_ADAC_POWER_CTRL_REG2      		(0x5000+0x11*4)
#define APB_ADAC_POWER_CTRL_REG3      		(0x5000+0x12*4)
#define APB_ADAC_MUTE_CTRL_REG1       		(0x5000+0x18*4)
#define APB_ADAC_DAC_ADC_MIXER        		(0x5000+0x1a*4)
#define APB_ADAC_PLAYBACK_VOL_CTRL_LSB              (0x5000+0x20*4)
#define APB_ADAC_PLAYBACK_VOL_CTRL_MSB              (0x5000+0x21*4)
#define APB_ADAC_STEREO_HS_VOL_CTRL_LSB             (0x5000+0x22*4)
#define APB_ADAC_STEREO_HS_VOL_CTRL_MSB             (0x5000+0x23*4)



#endif
