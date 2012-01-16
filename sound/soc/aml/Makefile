# AML Platform Support
snd-soc-aml-pcm-objs := aml_pcm.o
snd-soc-aml-dai-objs := aml_dai.o
snd-soc-aml-hw-objs  := aml_audio_hw.o

obj-$(CONFIG_SND_AML_SOC) += snd-soc-aml-pcm.o
obj-$(CONFIG_SND_AML_SOC) += snd-soc-aml-dai.o
obj-$(CONFIG_SND_AML_SOC) += snd-soc-aml-hw.o
obj-$(CONFIG_SND_AML_SOC) += aml_notify.o

#AML M1 Machine support

snd-soc-aml-m1-objs := aml_m1.o
snd-soc-aml-codec-objs := aml_m1_codec.o

snd-soc-aml-m1-mid-wm8900-objs := aml_m1_mid_wm8900.o

obj-$(CONFIG_SND_AML_M1) += snd-soc-aml-m1.o
obj-$(CONFIG_SND_AML_M1) += snd-soc-aml-codec.o


obj-$(CONFIG_SND_AML_M1_MID_WM8900) += snd-soc-aml-m1-mid-wm8900.o


