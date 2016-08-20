/*
 * Copyright (C) 2013-2015 Freescale Semiconductor, Inc.
 *
 * Based on imx-sgtl5000.c
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
 * Copyright (C) 2012 Linaro Ltd.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <asm/mach-types.h>

#include "../codecs/tlv320aic23.h"
#include "imx-ssi.h"
#include "fsl_ssi.h"
#include "imx-audmux.h"

#define CODEC_CLOCK 12000000


static int imx_tlv320aic23_hw_params(struct snd_pcm_substream *substream,
								struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime * rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	ret = snd_soc_dai_set_sysclk(codec_dai,0,CODEC_CLOCK,SND_SOC_CLOCK_OUT);

	if(ret){
		dev_err(cpu_dai->dev,"Failed to set the codec sysclk.\n");
		return ret;
	}

	snd_soc_dai_set_tdm_slot(cpu_dai, 0x3, 0x3, 2, 0);

	ret = snd_soc_dai_set_sysclk(cpu_dai,IMX_SSP_SYS_CLK,0,SND_SOC_CLOCK_IN);

	/* fsl_ssi lacks the set sysclk ops */
	if(ret && ret != -EINVAL){
		dev_err(cpu_dai->dev,"Can't set the IMX_SSP_SYS_CLK CPU system clock.\n");
		return ret;
	}
	return 0;
}

static struct snd_soc_ops imx_tlv320aic23_snd_ops = {
	.hw_params = imx_tlv320aic23_hw_params,
};

static struct snd_soc_dai_link imx_tlv320aic23_dai = {
	.name = "tlv320aic23",
	.stream_name = "TLV320AIC23",
	.codec_dai_name = "tlv320aic23-hifi",
	.dai_fmt  = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_IF |
				SND_SOC_DAIFMT_CBM_CFM,
	.ops     = &imx_tlv320aic23_snd_ops,
};

static struct snd_soc_card imx_tlv320aic23 = {
	.owner   = THIS_MODULE,
	.dai_link = &imx_tlv320aic23_dai,
	.num_links =1,
};

static int imx_tlv320aic23_probe(struct platform_device *pdev)
{
	int ret;
	int int_port = 0, ext_port;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *ssi_np = NULL, *codec_np = NULL;

	imx_tlv320aic23.dev = &pdev->dev;
	if(np){
		ret = snd_soc_of_parse_card_name(&imx_tlv320aic23,"imx,model");
		if (ret) {
			dev_err(&pdev->dev,"imx,model node missing or invalid.\n");
			goto err;
		}	

		ssi_np = of_parse_phandle(pdev->dev.of_node,"ssi-controller",0);
		if(!ssi_np){
			dev_err(&pdev->dev,"ssi-controller missing or invalid.\r");
			ret = -ENODEV;
			goto err;
		}

		codec_np = of_parse_phandle(ssi_np, "codec-handle", 0);
		if(codec_np)
			imx_tlv320aic23_dai.codec_of_node = codec_np;
		else
			dev_err(&pdev->dev,"codec-handle node missing or invalid.\n");


		ret = of_property_read_u32(np,"fsl,mux-int-port",&int_port);
		if(ret){
			dev_err(&pdev->dev,"fsl,mux-int-port node missing or invalid.\n");
			return ret;
		}
		ret = of_property_read_u32(np, "fsl,mux-ext-port", &ext_port);
		if(ret){
			dev_err(&pdev->dev,"fsl,mux-ext-port node missing or invalid.\n");
			return ret;
		}

		/*
		 * The port numbering in the hardware manual starts at 1, while
		 * the audmux API expects it starts at 0.
		 */
		int_port--;
		ext_port--;

		imx_tlv320aic23_dai.cpu_of_node = ssi_np;
		imx_tlv320aic23_dai.platform_of_node = ssi_np;		
	}else{
		imx_tlv320aic23_dai.cpu_dai_name = "imx-ssi.0";
		imx_tlv320aic23_dai.platform_name = "imx-ssi.0";
		imx_tlv320aic23_dai.codec_name = "tlv320aic23-codec.0-001a";
		imx_tlv320aic23.name = "cpuimx-audio";
	}

    if(of_find_compatible_node(NULL, NULL, "fsl,imx6q-audmux")) {
		imx_audmux_v2_configure_port(int_port,
			IMX_AUDMUX_V2_PTCR_SYN |
			IMX_AUDMUX_V2_PTCR_TFSDIR |
			IMX_AUDMUX_V2_PTCR_TFSEL(ext_port) |
			IMX_AUDMUX_V2_PTCR_TCLKDIR |
			IMX_AUDMUX_V2_PTCR_TCSEL(ext_port),
			IMX_AUDMUX_V2_PDCR_RXDSEL(ext_port)
		);
		imx_audmux_v2_configure_port(ext_port,
			IMX_AUDMUX_V2_PTCR_SYN,
			IMX_AUDMUX_V2_PDCR_RXDSEL(int_port)
		);
	} else {
		if (np) {
			/* The imx,asoc-tlv320 driver was explicitely
			 * requested (through the device tree).
			 */
			dev_err(&pdev->dev,
				"Missing or invalid audmux DT node.\n");
			return -ENODEV;
		} else {
			/* Return happy.
			 * We might run on a totally different machine.
			 */
			return 0;
		}
	}

	ret = snd_soc_register_card(&imx_tlv320aic23);
	
err:
	if(ret)
		dev_err(&pdev->dev,"snd_soc_register_card failed (%d)\n", ret);
	of_node_put(ssi_np);

	return ret;	
}

static int imx_tlv320aic23_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&imx_tlv320aic23);
	return 0;
}

static const struct of_device_id imx_tlv320aic23_dt_ids[] = {
		{ .compatible = "imx,asoc-tlv320aic23",},
};

MODULE_DEVICE_TABLE(of,imx_tlv320aic23_dt_ids);
static struct platform_driver imx_tlv320aic23_driver = {
	.driver = {
		.name = "imx_tlv320aic23",
		.of_match_table = imx_tlv320aic23_dt_ids,
	},
	.probe = imx_tlv320aic23_probe,
	.remove = imx_tlv320aic23_remove,
};


module_platform_driver(imx_tlv320aic23_driver);

MODULE_ALIAS("hehaibo:hehaibo@huania.com");
MODULE_DESCRIPTION("CPUIMX ALSA ASoC driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:imx-tlv320aic23");
