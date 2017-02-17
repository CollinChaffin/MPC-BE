/*
 * Provide registration of all codecs, parsers and bitstream filters for libavcodec.
 * Copyright (c) 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * Provide registration of all codecs, parsers and bitstream filters for libavcodec.
 */

#include "config.h"
#include "avcodec.h"
#include "version.h"

#define REGISTER_HWACCEL(X, x)                                          \
    {                                                                   \
        extern AVHWAccel ff_##x##_hwaccel;                              \
        if (CONFIG_##X##_HWACCEL)                                       \
            av_register_hwaccel(&ff_##x##_hwaccel);                     \
    }

#define REGISTER_ENCODER(X, x)                                          \
    {                                                                   \
        extern AVCodec ff_##x##_encoder;                                \
        if (CONFIG_##X##_ENCODER)                                       \
            avcodec_register(&ff_##x##_encoder);                        \
    }

#define REGISTER_DECODER(X, x)                                          \
    {                                                                   \
        extern AVCodec ff_##x##_decoder;                                \
        if (CONFIG_##X##_DECODER)                                       \
            avcodec_register(&ff_##x##_decoder);                        \
    }

#define REGISTER_ENCDEC(X, x) REGISTER_ENCODER(X, x); REGISTER_DECODER(X, x)

#define REGISTER_PARSER(X, x)                                           \
    {                                                                   \
        extern AVCodecParser ff_##x##_parser;                           \
        if (CONFIG_##X##_PARSER)                                        \
            av_register_codec_parser(&ff_##x##_parser);                 \
    }

#define REGISTER_BSF(X, x)                                              \
    {                                                                   \
        extern AVBitStreamFilter ff_##x##_bsf;                          \
        if (CONFIG_##X##_BSF)                                           \
            av_register_bitstream_filter(&ff_##x##_bsf);                \
    }

void avcodec_register_all(void)
{
    static int initialized;

    if (initialized)
        return;
    initialized = 1;

    /* video codecs */
    REGISTER_DECODER (8BPS, eightbps);
    REGISTER_DECODER (AIC, aic);
    REGISTER_DECODER (AMV, amv);
    REGISTER_DECODER (BINK, bink);
    REGISTER_DECODER (CFHD, cfhd);
    REGISTER_DECODER (CINEPAK, cinepak);
    REGISTER_DECODER (CLLC, cllc);
    REGISTER_DECODER (CSCD, cscd);
    REGISTER_DECODER (DIRAC, dirac);
    REGISTER_DECODER (DNXHD, dnxhd);
    REGISTER_DECODER (DSD_LSBF, dsd_lsbf);
    REGISTER_DECODER (DSD_MSBF, dsd_msbf);
    REGISTER_DECODER (DSD_LSBF_PLANAR, dsd_lsbf_planar);
    REGISTER_DECODER (DSD_MSBF_PLANAR, dsd_msbf_planar);
    REGISTER_DECODER (DST, dst);
    REGISTER_DECODER (DVVIDEO, dvvideo);
    REGISTER_DECODER (FFV1, ffv1);
    REGISTER_DECODER (FFVHUFF, ffvhuff);
    REGISTER_DECODER (FLASHSV, flashsv);
    REGISTER_DECODER (FLASHSV2, flashsv2);
    REGISTER_DECODER (FLV, flv);
    REGISTER_DECODER (FRAPS, fraps);
    REGISTER_DECODER (G2M, g2m);
    REGISTER_DECODER (H263, h263);
    REGISTER_DECODER (H264, h264);
    REGISTER_DECODER (HQ_HQA, hq_hqa);
    REGISTER_DECODER (HQX, hqx);
    REGISTER_DECODER (HEVC, hevc);
    REGISTER_DECODER (HUFFYUV, huffyuv);
    REGISTER_DECODER (INDEO3, indeo3);
    REGISTER_DECODER (INDEO4, indeo4);
    REGISTER_DECODER (INDEO5, indeo5);
    REGISTER_DECODER (LAGARITH, lagarith);
    REGISTER_DECODER (MAGICYUV, magicyuv);
    REGISTER_DECODER (MJPEG, mjpeg);
    REGISTER_DECODER (MJPEGB, mjpegb);
    REGISTER_DECODER (MPEG1VIDEO, mpeg1video);
    REGISTER_DECODER (MPEG2VIDEO, mpeg2video);
    REGISTER_DECODER (MPEG4, mpeg4);
    REGISTER_DECODER (MSA1, msa1);
    REGISTER_DECODER (MSMPEG4V1, msmpeg4v1);
    REGISTER_DECODER (MSMPEG4V2, msmpeg4v2);
    REGISTER_DECODER (MSMPEG4V3, msmpeg4v3);
    REGISTER_DECODER (MSS1, mss1);
    REGISTER_DECODER (MSS2, mss2);
    REGISTER_DECODER (MSVIDEO1, msvideo1);
    REGISTER_DECODER (MTS2, mts2);
    REGISTER_DECODER (QTRLE, qtrle);
    REGISTER_DECODER (PNG, png);
    REGISTER_DECODER (PRORES, prores);
    REGISTER_DECODER (R10K, r10k);
    REGISTER_DECODER (R210, r210);
    REGISTER_DECODER (RPZA, rpza);
    REGISTER_DECODER (RV10, rv10);
    REGISTER_DECODER (RV20, rv20);
    REGISTER_DECODER (RV30, rv30);
    REGISTER_DECODER (RV40, rv40);
    REGISTER_DECODER (SVQ1, svq1);
    REGISTER_DECODER (SVQ3, svq3);
    REGISTER_DECODER (THEORA, theora);
    REGISTER_DECODER (TSCC, tscc);
    REGISTER_DECODER (TSCC2, tscc2);
    REGISTER_DECODER (VC1, vc1);
    REGISTER_DECODER (VC1IMAGE, vc1image);
    REGISTER_DECODER (VMNC, vmnc);
    REGISTER_DECODER (VP3, vp3);
    REGISTER_DECODER (VP5, vp5);
    REGISTER_DECODER (VP6, vp6);
    REGISTER_DECODER (VP6A, vp6a);
    REGISTER_DECODER (VP6F, vp6f);
    REGISTER_DECODER (VP7, vp7);
    REGISTER_DECODER (VP8, vp8);
    REGISTER_DECODER (VP9, vp9);
    REGISTER_DECODER (WMV1, wmv1);
    REGISTER_DECODER (WMV2, wmv2);
    REGISTER_DECODER (WMV3, wmv3);
    REGISTER_DECODER (WMV3IMAGE, wmv3image);
    REGISTER_DECODER (UTVIDEO, utvideo);
    REGISTER_DECODER (V210, v210);
    REGISTER_DECODER (V410, v410);
    REGISTER_DECODER (RAWVIDEO, rawvideo);

    /* audio codecs */
    REGISTER_DECODER (AAC, aac);
    REGISTER_DECODER (AAC_LATM, aac_latm);
    REGISTER_DECODER (ALAC, alac);
    REGISTER_DECODER (ALS, als);
    REGISTER_DECODER (APE, ape);
    REGISTER_DECODER (ATRAC3, atrac3);
    REGISTER_DECODER (ATRAC3P, atrac3p);
    REGISTER_DECODER (BINKAUDIO_DCT, binkaudio_dct);
    REGISTER_DECODER (BINKAUDIO_RDFT, binkaudio_rdft);
    REGISTER_DECODER (COOK, cook);
    REGISTER_DECODER (DCA, dca);
    REGISTER_DECODER (FLAC, flac);
    REGISTER_DECODER (IAC, iac);
    REGISTER_DECODER (METASOUND, metasound);
    REGISTER_DECODER (MPC7, mpc7);
    REGISTER_DECODER (MPC8, mpc8);
    REGISTER_DECODER (MLP, mlp);
    REGISTER_DECODER (MP1FLOAT, mp1float);
    REGISTER_DECODER (MP2FLOAT, mp2float);
    REGISTER_DECODER (MP3FLOAT, mp3float);
    REGISTER_DECODER (NELLYMOSER, nellymoser);
    REGISTER_DECODER (OPUS, opus);
    REGISTER_DECODER (RA_144, ra_144);
    REGISTER_DECODER (RA_288, ra_288);
    REGISTER_DECODER (RALF, ralf);
    REGISTER_DECODER (SHORTEN, shorten);
    REGISTER_DECODER (SIPR, sipr);
    REGISTER_DECODER (TRUEHD, truehd);
    REGISTER_DECODER (TRUESPEECH, truespeech);
    REGISTER_DECODER (VORBIS, vorbis);
    REGISTER_DECODER (AMRNB, amrnb);
    REGISTER_DECODER (AMRWB, amrwb);
    REGISTER_DECODER (QDM2, qdm2);
    REGISTER_DECODER (QDMC, qdmc);
    REGISTER_DECODER (WAVPACK, wavpack);
    REGISTER_DECODER (WMALOSSLESS, wmalossless);
    REGISTER_DECODER (WMAPRO, wmapro);
    REGISTER_DECODER (WMAV1, wmav1);
    REGISTER_DECODER (WMAV2, wmav2);
    REGISTER_DECODER (WMAVOICE, wmavoice);
    REGISTER_DECODER (TTA, tta);
    REGISTER_DECODER (TAK, tak);

    REGISTER_ENCDEC  (AC3, ac3);
    REGISTER_ENCDEC  (EAC3, eac3);

    /* ADPCM codecs */
    REGISTER_DECODER (ADPCM_ADX, adpcm_adx);
    REGISTER_DECODER (ADPCM_4XM, adpcm_4xm);
    REGISTER_DECODER (ADPCM_CT, adpcm_ct);
    REGISTER_DECODER (ADPCM_EA, adpcm_ea);
    REGISTER_DECODER (ADPCM_IMA_AMV, adpcm_ima_amv);
    REGISTER_DECODER (ADPCM_IMA_DK3, adpcm_ima_dk3);
    REGISTER_DECODER (ADPCM_IMA_DK4, adpcm_ima_dk4);
    REGISTER_DECODER (ADPCM_IMA_QT, adpcm_ima_qt);
    REGISTER_DECODER (ADPCM_IMA_SMJPEG, adpcm_ima_smjpeg);
    REGISTER_DECODER (ADPCM_IMA_WAV, adpcm_ima_wav);
    REGISTER_DECODER (ADPCM_IMA_WS, adpcm_ima_ws);
    REGISTER_DECODER (ADPCM_MS, adpcm_ms);
    REGISTER_DECODER (ADPCM_SBPRO_2, adpcm_sbpro_2);
    REGISTER_DECODER (ADPCM_SBPRO_3, adpcm_sbpro_3);
    REGISTER_DECODER (ADPCM_SBPRO_4, adpcm_sbpro_4);
    REGISTER_DECODER (ADPCM_SWF, adpcm_swf);
    REGISTER_DECODER (ADPCM_XA, adpcm_xa);
    REGISTER_DECODER (ADPCM_YAMAHA, adpcm_yamaha);

    /* parsers */
    REGISTER_PARSER (AAC_LATM, aac_latm);
    REGISTER_PARSER (AC3, ac3);
    REGISTER_PARSER (ADX, adx);
    REGISTER_PARSER (DCA, dca);
    REGISTER_PARSER (DNXHD, dnxhd);
    REGISTER_PARSER (H263, h263);
    REGISTER_PARSER (H264, h264);
    REGISTER_PARSER (HEVC, hevc);
    REGISTER_PARSER (MJPEG, mjpeg);
    REGISTER_PARSER (MLP, mlp);
    REGISTER_PARSER (MPEGAUDIO, mpegaudio);
    REGISTER_PARSER (MPEGVIDEO, mpegvideo);
    REGISTER_PARSER (OPUS, opus);
    REGISTER_PARSER (TAK, tak);
    REGISTER_PARSER (VORBIS, vorbis);
    REGISTER_PARSER (VP3, vp3);
    REGISTER_PARSER (VP8, vp8);
    REGISTER_PARSER (VP9, vp9);

    /* external libraries */
    REGISTER_DECODER (LIBOPENJPEG, libopenjpeg);
    //REGISTER_DECODER (LIBOPUS, libopus);
    REGISTER_DECODER (LIBSPEEX, libspeex);
    //REGISTER_DECODER (LIBVPX_VP9, libvpx_vp9);
}
