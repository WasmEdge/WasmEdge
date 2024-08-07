// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
}

namespace WasmEdge {
namespace Host {
namespace WasmEdgeFFmpeg {
namespace FFmpegUtils {
class MediaType {
public:
  static AVMediaType intoMediaType(int32_t MediaTypeId) {
    switch (MediaTypeId) {
    case 0:
      return AVMEDIA_TYPE_VIDEO;
    case 1:
      return AVMEDIA_TYPE_AUDIO;
    case 2:
      return AVMEDIA_TYPE_DATA;
    case 3:
      return AVMEDIA_TYPE_SUBTITLE;
    case 4:
      return AVMEDIA_TYPE_ATTACHMENT;
    case 5:
      return AVMEDIA_TYPE_NB;
    default:
      return AVMEDIA_TYPE_UNKNOWN;
    }
  }

  static int32_t fromMediaType(AVMediaType MediaType) {
    switch (MediaType) {
    case AVMEDIA_TYPE_VIDEO:
      return 0;
    case AVMEDIA_TYPE_AUDIO:
      return 1;
    case AVMEDIA_TYPE_DATA:
      return 2;
    case AVMEDIA_TYPE_SUBTITLE:
      return 3;
    case 4:
      return AVMEDIA_TYPE_ATTACHMENT;
    case 5:
      return AVMEDIA_TYPE_NB;
    default:
      return AVMEDIA_TYPE_UNKNOWN;
    }
  }
};

class CodecID {
public:
  static AVCodecID intoAVCodecID(uint32_t AvCodecIndex) {
    switch (AvCodecIndex) {
    case 0:
      return AV_CODEC_ID_NONE;
    case 1:
      return AV_CODEC_ID_MPEG1VIDEO;
    case 2:
      return AV_CODEC_ID_MPEG2VIDEO;
    case 3:
      return AV_CODEC_ID_H261;
    case 4:
      return AV_CODEC_ID_H263;
    case 5:
      return AV_CODEC_ID_RV10;
    case 6:
      return AV_CODEC_ID_RV20;
    case 7:
      return AV_CODEC_ID_MJPEG;
    case 8:
      return AV_CODEC_ID_MJPEGB;
    case 9:
      return AV_CODEC_ID_LJPEG;
    case 10:
      return AV_CODEC_ID_SP5X;
    case 11:
      return AV_CODEC_ID_JPEGLS;
    case 12:
      return AV_CODEC_ID_MPEG4;
    case 13:
      return AV_CODEC_ID_RAWVIDEO;
    case 14:
      return AV_CODEC_ID_MSMPEG4V1;
    case 15:
      return AV_CODEC_ID_MSMPEG4V2;
    case 16:
      return AV_CODEC_ID_MSMPEG4V3;
    case 17:
      return AV_CODEC_ID_WMV1;
    case 18:
      return AV_CODEC_ID_WMV2;
    case 19:
      return AV_CODEC_ID_H263P;
    case 20:
      return AV_CODEC_ID_H263I;
    case 21:
      return AV_CODEC_ID_FLV1;
    case 22:
      return AV_CODEC_ID_SVQ1;
    case 23:
      return AV_CODEC_ID_SVQ3;
    case 24:
      return AV_CODEC_ID_DVVIDEO;
    case 25:
      return AV_CODEC_ID_HUFFYUV;
    case 26:
      return AV_CODEC_ID_CYUV;
    case 27:
      return AV_CODEC_ID_H264;
    case 28:
      return AV_CODEC_ID_INDEO3;
    case 29:
      return AV_CODEC_ID_VP3;
    case 30:
      return AV_CODEC_ID_THEORA;
    case 31:
      return AV_CODEC_ID_ASV1;
    case 32:
      return AV_CODEC_ID_ASV2;
    case 33:
      return AV_CODEC_ID_FFV1;
    case 34:
      return AV_CODEC_ID_4XM;
    case 35:
      return AV_CODEC_ID_VCR1;
    case 36:
      return AV_CODEC_ID_CLJR;
    case 37:
      return AV_CODEC_ID_MDEC;
    case 38:
      return AV_CODEC_ID_ROQ;
    case 39:
      return AV_CODEC_ID_INTERPLAY_VIDEO;
    case 40:
      return AV_CODEC_ID_XAN_WC3;
    case 41:
      return AV_CODEC_ID_XAN_WC4;
    case 42:
      return AV_CODEC_ID_RPZA;
    case 43:
      return AV_CODEC_ID_CINEPAK;
    case 44:
      return AV_CODEC_ID_WS_VQA;
    case 45:
      return AV_CODEC_ID_MSRLE;
    case 46:
      return AV_CODEC_ID_MSVIDEO1;
    case 47:
      return AV_CODEC_ID_IDCIN;
    case 48:
      return AV_CODEC_ID_8BPS;
    case 49:
      return AV_CODEC_ID_SMC;
    case 50:
      return AV_CODEC_ID_FLIC;
    case 51:
      return AV_CODEC_ID_TRUEMOTION1;
    case 52:
      return AV_CODEC_ID_VMDVIDEO;
    case 53:
      return AV_CODEC_ID_MSZH;
    case 54:
      return AV_CODEC_ID_ZLIB;
    case 55:
      return AV_CODEC_ID_QTRLE;
    case 56:
      return AV_CODEC_ID_TSCC;
    case 57:
      return AV_CODEC_ID_ULTI;
    case 58:
      return AV_CODEC_ID_QDRAW;
    case 59:
      return AV_CODEC_ID_VIXL;
    case 60:
      return AV_CODEC_ID_QPEG;
    case 61:
      return AV_CODEC_ID_PNG;
    case 62:
      return AV_CODEC_ID_PPM;
    case 63:
      return AV_CODEC_ID_PBM;
    case 64:
      return AV_CODEC_ID_PGM;
    case 65:
      return AV_CODEC_ID_PGMYUV;
    case 66:
      return AV_CODEC_ID_PAM;
    case 67:
      return AV_CODEC_ID_FFVHUFF;
    case 68:
      return AV_CODEC_ID_RV30;
    case 69:
      return AV_CODEC_ID_RV40;
    case 70:
      return AV_CODEC_ID_VC1;
    case 71:
      return AV_CODEC_ID_WMV3;
    case 72:
      return AV_CODEC_ID_LOCO;
    case 73:
      return AV_CODEC_ID_WNV1;
    case 74:
      return AV_CODEC_ID_AASC;
    case 75:
      return AV_CODEC_ID_INDEO2;
    case 76:
      return AV_CODEC_ID_FRAPS;
    case 77:
      return AV_CODEC_ID_TRUEMOTION2;
    case 78:
      return AV_CODEC_ID_BMP;
    case 79:
      return AV_CODEC_ID_CSCD;
    case 80:
      return AV_CODEC_ID_MMVIDEO;
    case 81:
      return AV_CODEC_ID_ZMBV;
    case 82:
      return AV_CODEC_ID_AVS;
    case 83:
      return AV_CODEC_ID_SMACKVIDEO;
    case 84:
      return AV_CODEC_ID_NUV;
    case 85:
      return AV_CODEC_ID_KMVC;
    case 86:
      return AV_CODEC_ID_FLASHSV;
    case 87:
      return AV_CODEC_ID_CAVS;
    case 88:
      return AV_CODEC_ID_JPEG2000;
    case 89:
      return AV_CODEC_ID_VMNC;
    case 90:
      return AV_CODEC_ID_VP5;
    case 91:
      return AV_CODEC_ID_VP6;
    case 92:
      return AV_CODEC_ID_VP6F;
    case 93:
      return AV_CODEC_ID_TARGA;
    case 94:
      return AV_CODEC_ID_DSICINVIDEO;
    case 95:
      return AV_CODEC_ID_TIERTEXSEQVIDEO;
    case 96:
      return AV_CODEC_ID_TIFF;
    case 97:
      return AV_CODEC_ID_GIF;
    case 98:
      return AV_CODEC_ID_DXA;
    case 99:
      return AV_CODEC_ID_DNXHD;
    case 100:
      return AV_CODEC_ID_THP;
    case 101:
      return AV_CODEC_ID_SGI;
    case 102:
      return AV_CODEC_ID_C93;
    case 103:
      return AV_CODEC_ID_BETHSOFTVID;
    case 104:
      return AV_CODEC_ID_PTX;
    case 105:
      return AV_CODEC_ID_TXD;
    case 106:
      return AV_CODEC_ID_VP6A;
    case 107:
      return AV_CODEC_ID_AMV;
    case 108:
      return AV_CODEC_ID_VB;
    case 109:
      return AV_CODEC_ID_PCX;
    case 110:
      return AV_CODEC_ID_SUNRAST;
    case 111:
      return AV_CODEC_ID_INDEO4;
    case 112:
      return AV_CODEC_ID_INDEO5;
    case 113:
      return AV_CODEC_ID_MIMIC;
    case 114:
      return AV_CODEC_ID_RL2;
    case 115:
      return AV_CODEC_ID_ESCAPE124;
    case 116:
      return AV_CODEC_ID_DIRAC;
    case 117:
      return AV_CODEC_ID_BFI;
    case 118:
      return AV_CODEC_ID_CMV;
    case 119:
      return AV_CODEC_ID_MOTIONPIXELS;
    case 120:
      return AV_CODEC_ID_TGV;
    case 121:
      return AV_CODEC_ID_TGQ;
    case 122:
      return AV_CODEC_ID_TQI;
    case 123:
      return AV_CODEC_ID_AURA;
    case 124:
      return AV_CODEC_ID_AURA2;
    case 125:
      return AV_CODEC_ID_V210X;
    case 126:
      return AV_CODEC_ID_TMV;
    case 127:
      return AV_CODEC_ID_V210;
    case 128:
      return AV_CODEC_ID_DPX;
    case 129:
      return AV_CODEC_ID_MAD;
    case 130:
      return AV_CODEC_ID_FRWU;
    case 131:
      return AV_CODEC_ID_FLASHSV2;
    case 132:
      return AV_CODEC_ID_CDGRAPHICS;
    case 133:
      return AV_CODEC_ID_R210;
    case 134:
      return AV_CODEC_ID_ANM;
    case 135:
      return AV_CODEC_ID_BINKVIDEO;
    case 136:
      return AV_CODEC_ID_IFF_ILBM;
    case 137:
      return AV_CODEC_ID_IFF_ILBM;
    case 138:
      return AV_CODEC_ID_KGV1;
    case 139:
      return AV_CODEC_ID_YOP;
    case 140:
      return AV_CODEC_ID_VP8;
    case 141:
      return AV_CODEC_ID_PICTOR;
    case 142:
      return AV_CODEC_ID_ANSI;
    case 143:
      return AV_CODEC_ID_A64_MULTI;
    case 144:
      return AV_CODEC_ID_A64_MULTI5;
    case 145:
      return AV_CODEC_ID_R10K;
    case 146:
      return AV_CODEC_ID_MXPEG;
    case 147:
      return AV_CODEC_ID_LAGARITH;
    case 148:
      return AV_CODEC_ID_PRORES;
    case 149:
      return AV_CODEC_ID_JV;
    case 150:
      return AV_CODEC_ID_DFA;
    case 151:
      return AV_CODEC_ID_WMV3IMAGE;
    case 152:
      return AV_CODEC_ID_VC1IMAGE;
    case 153:
      return AV_CODEC_ID_UTVIDEO;
    case 154:
      return AV_CODEC_ID_BMV_VIDEO;
    case 155:
      return AV_CODEC_ID_VBLE;
    case 156:
      return AV_CODEC_ID_DXTORY;
    case 157:
      return AV_CODEC_ID_V410;
    case 158:
      return AV_CODEC_ID_XWD;
    case 159:
      return AV_CODEC_ID_CDXL;
    case 160:
      return AV_CODEC_ID_XBM;
    case 161:
      return AV_CODEC_ID_ZEROCODEC;
    case 162:
      return AV_CODEC_ID_MSS1;
    case 163:
      return AV_CODEC_ID_MSA1;
    case 164:
      return AV_CODEC_ID_TSCC2;
    case 165:
      return AV_CODEC_ID_MTS2;
    case 166:
      return AV_CODEC_ID_CLLC;
    case 167:
      return AV_CODEC_ID_MSS2;
    case 168:
      return AV_CODEC_ID_VP9;
    case 169:
      return AV_CODEC_ID_AIC;
    case 170:
      return AV_CODEC_ID_ESCAPE130;
    case 171:
      return AV_CODEC_ID_G2M;
    case 172:
      return AV_CODEC_ID_WEBP;
    case 173:
      return AV_CODEC_ID_HNM4_VIDEO;
    case 174:
      return AV_CODEC_ID_HEVC;
    case 175:
      return AV_CODEC_ID_HEVC;
    case 176:
      return AV_CODEC_ID_FIC;
    case 177:
      return AV_CODEC_ID_ALIAS_PIX;
    case 178:
      return AV_CODEC_ID_BRENDER_PIX;
    case 179:
      return AV_CODEC_ID_PAF_VIDEO;
    case 180:
      return AV_CODEC_ID_EXR;
    case 181:
      return AV_CODEC_ID_VP7;
    case 182:
      return AV_CODEC_ID_SANM;
    case 183:
      return AV_CODEC_ID_SGIRLE;
    case 184:
      return AV_CODEC_ID_MVC1;
    case 185:
      return AV_CODEC_ID_MVC2;
    case 186:
      return AV_CODEC_ID_HQX;
    case 187:
      return AV_CODEC_ID_TDSC;
    case 188:
      return AV_CODEC_ID_HQ_HQA;
    case 189:
      return AV_CODEC_ID_HAP;
    case 190:
      return AV_CODEC_ID_DDS;
    case 191:
      return AV_CODEC_ID_DXV;
    case 192:
      return AV_CODEC_ID_SCREENPRESSO;
    case 193:
      return AV_CODEC_ID_RSCC;
      ///////////////////////////////
      //    case 194:
      //      return AV_CODEC_ID_Y41P;
      //    case 194:
      //      return AV_CODEC_ID_AVS2;
    case 194:
      return AV_CODEC_ID_Y41P;
    case 195:
      return AV_CODEC_ID_AVRP;
    case 196:
      return AV_CODEC_ID_012V;
    case 197:
      return AV_CODEC_ID_AVUI;
    case 198:
      return AV_CODEC_ID_AYUV;
    case 199:
      return AV_CODEC_ID_TARGA_Y216;
    case 200:
      return AV_CODEC_ID_V308;
    case 201:
      return AV_CODEC_ID_V408;
    case 202:
      return AV_CODEC_ID_YUV4;
    case 203:
      return AV_CODEC_ID_AVRN;
    case 204:
      return AV_CODEC_ID_CPIA;
    case 205:
      return AV_CODEC_ID_XFACE;
    case 206:
      return AV_CODEC_ID_SNOW;
    case 207:
      return AV_CODEC_ID_SMVJPEG;
    case 208:
      return AV_CODEC_ID_APNG;
    case 209:
      return AV_CODEC_ID_DAALA;
    case 210:
      return AV_CODEC_ID_CFHD;
    case 211:
      return AV_CODEC_ID_TRUEMOTION2RT;
    case 212:
      return AV_CODEC_ID_M101;
    case 213:
      return AV_CODEC_ID_MAGICYUV;
    case 214:
      return AV_CODEC_ID_SHEERVIDEO;
    case 215:
      return AV_CODEC_ID_YLC;
    case 216:
      return AV_CODEC_ID_PCM_S16LE;
    case 217:
      return AV_CODEC_ID_PCM_S16BE;
    case 218:
      return AV_CODEC_ID_PCM_U16LE;
    case 219:
      return AV_CODEC_ID_PCM_U16BE;
    case 220:
      return AV_CODEC_ID_PCM_S8;
    case 221:
      return AV_CODEC_ID_PCM_U8;
    case 222:
      return AV_CODEC_ID_PCM_MULAW;
    case 223:
      return AV_CODEC_ID_PCM_ALAW;
    case 224:
      return AV_CODEC_ID_PCM_S32LE;
    case 225:
      return AV_CODEC_ID_PCM_S32BE;
    case 226:
      return AV_CODEC_ID_PCM_U32LE;
    case 227:
      return AV_CODEC_ID_PCM_U32BE;
    case 228:
      return AV_CODEC_ID_PCM_S24LE;
    case 229:
      return AV_CODEC_ID_PCM_S24BE;
    case 230:
      return AV_CODEC_ID_PCM_U24LE;
    case 231:
      return AV_CODEC_ID_PCM_U24BE;
    case 232:
      return AV_CODEC_ID_PCM_S24DAUD;
    case 233:
      return AV_CODEC_ID_PCM_ZORK;
    case 234:
      return AV_CODEC_ID_PCM_S16LE_PLANAR;
    case 235:
      return AV_CODEC_ID_PCM_DVD;
    case 236:
      return AV_CODEC_ID_PCM_F32BE;
    case 237:
      return AV_CODEC_ID_PCM_F32LE;
    case 238:
      return AV_CODEC_ID_PCM_F64BE;
    case 239:
      return AV_CODEC_ID_PCM_F64LE;
    case 240:
      return AV_CODEC_ID_PCM_BLURAY;
    case 241:
      return AV_CODEC_ID_PCM_LXF;
    case 242:
      return AV_CODEC_ID_S302M;
    case 243:
      return AV_CODEC_ID_PCM_S8_PLANAR;
    case 244:
      return AV_CODEC_ID_PCM_S24LE_PLANAR;
    case 245:
      return AV_CODEC_ID_PCM_S32LE_PLANAR;
    case 246:
      return AV_CODEC_ID_PCM_S16BE_PLANAR;
    case 247:
      return AV_CODEC_ID_PCM_S64LE;
    case 248:
      return AV_CODEC_ID_PCM_S64BE;
    case 249:
      return AV_CODEC_ID_ADPCM_IMA_QT;
    case 250:
      return AV_CODEC_ID_ADPCM_IMA_WAV;
    case 251:
      return AV_CODEC_ID_ADPCM_IMA_DK3;
    case 252:
      return AV_CODEC_ID_ADPCM_IMA_DK4;
    case 253:
      return AV_CODEC_ID_ADPCM_IMA_WS;
    case 254:
      return AV_CODEC_ID_ADPCM_IMA_SMJPEG;
    case 255:
      return AV_CODEC_ID_ADPCM_MS;
    case 256:
      return AV_CODEC_ID_ADPCM_4XM;
    case 257:
      return AV_CODEC_ID_ADPCM_XA;
    case 258:
      return AV_CODEC_ID_ADPCM_ADX;
    case 259:
      return AV_CODEC_ID_ADPCM_EA;
    case 260:
      return AV_CODEC_ID_ADPCM_G726;
    case 261:
      return AV_CODEC_ID_ADPCM_CT;
    case 262:
      return AV_CODEC_ID_ADPCM_SWF;
    case 263:
      return AV_CODEC_ID_ADPCM_YAMAHA;
    case 264:
      return AV_CODEC_ID_ADPCM_SBPRO_4;
    case 265:
      return AV_CODEC_ID_ADPCM_SBPRO_3;
    case 266:
      return AV_CODEC_ID_ADPCM_SBPRO_2;
    case 267:
      return AV_CODEC_ID_ADPCM_THP;
    case 268:
      return AV_CODEC_ID_ADPCM_IMA_AMV;
    case 269:
      return AV_CODEC_ID_ADPCM_EA_R1;
    case 270:
      return AV_CODEC_ID_ADPCM_EA_R3;
    case 271:
      return AV_CODEC_ID_ADPCM_EA_R2;
    case 272:
      return AV_CODEC_ID_ADPCM_IMA_EA_SEAD;
    case 273:
      return AV_CODEC_ID_ADPCM_IMA_EA_EACS;
    case 274:
      return AV_CODEC_ID_ADPCM_EA_XAS;
    case 275:
      return AV_CODEC_ID_ADPCM_EA_MAXIS_XA;
    case 276:
      return AV_CODEC_ID_ADPCM_IMA_ISS;
    case 277:
      return AV_CODEC_ID_ADPCM_G722;
    case 278:
      return AV_CODEC_ID_ADPCM_IMA_APC;
    case 279:
      return AV_CODEC_ID_ADPCM_VIMA;
    case 280:
      return AV_CODEC_ID_ADPCM_AFC;
    case 281:
      return AV_CODEC_ID_ADPCM_IMA_OKI;
    case 282:
      return AV_CODEC_ID_ADPCM_DTK;
    case 283:
      return AV_CODEC_ID_ADPCM_IMA_RAD;
    case 284:
      return AV_CODEC_ID_ADPCM_G726LE;
    case 285:
      return AV_CODEC_ID_ADPCM_THP_LE;
    case 286:
      return AV_CODEC_ID_ADPCM_PSX;
    case 287:
      return AV_CODEC_ID_ADPCM_AICA;
    case 288:
      return AV_CODEC_ID_ADPCM_IMA_DAT4;
    case 289:
      return AV_CODEC_ID_ADPCM_MTAF;
    case 290:
      return AV_CODEC_ID_AMR_NB;
    case 291:
      return AV_CODEC_ID_AMR_WB;
    case 292:
      return AV_CODEC_ID_RA_144;
    case 293:
      return AV_CODEC_ID_RA_288;
    case 294:
      return AV_CODEC_ID_ROQ_DPCM;
    case 295:
      return AV_CODEC_ID_INTERPLAY_DPCM;
    case 296:
      return AV_CODEC_ID_XAN_DPCM;
    case 297:
      return AV_CODEC_ID_SOL_DPCM;
    case 298:
      return AV_CODEC_ID_SDX2_DPCM;
    case 299:
      return AV_CODEC_ID_MP2;
    case 300:
      return AV_CODEC_ID_MP3;
    case 301:
      return AV_CODEC_ID_AAC;
    case 302:
      return AV_CODEC_ID_AC3;
    case 303:
      return AV_CODEC_ID_DTS;
    case 304:
      return AV_CODEC_ID_VORBIS;
    case 305:
      return AV_CODEC_ID_DVAUDIO;
    case 306:
      return AV_CODEC_ID_WMAV1;
    case 307:
      return AV_CODEC_ID_WMAV2;
    case 308:
      return AV_CODEC_ID_MACE3;
    case 309:
      return AV_CODEC_ID_MACE6;
    case 310:
      return AV_CODEC_ID_VMDAUDIO;
    case 311:
      return AV_CODEC_ID_FLAC;
    case 312:
      return AV_CODEC_ID_MP3ADU;
    case 313:
      return AV_CODEC_ID_MP3ON4;
    case 314:
      return AV_CODEC_ID_SHORTEN;
    case 315:
      return AV_CODEC_ID_ALAC;
    case 316:
      return AV_CODEC_ID_WESTWOOD_SND1;
    case 317:
      return AV_CODEC_ID_GSM;
    case 318:
      return AV_CODEC_ID_QDM2;
    case 319:
      return AV_CODEC_ID_COOK;
    case 320:
      return AV_CODEC_ID_TRUESPEECH;
    case 321:
      return AV_CODEC_ID_TTA;
    case 322:
      return AV_CODEC_ID_SMACKAUDIO;
    case 323:
      return AV_CODEC_ID_QCELP;
    case 324:
      return AV_CODEC_ID_WAVPACK;
    case 325:
      return AV_CODEC_ID_DSICINAUDIO;
    case 326:
      return AV_CODEC_ID_IMC;
    case 327:
      return AV_CODEC_ID_MUSEPACK7;
    case 328:
      return AV_CODEC_ID_MLP;
    case 329:
      return AV_CODEC_ID_GSM_MS;
    case 330:
      return AV_CODEC_ID_ATRAC3;
      // #[cfg(feature = "ff_api_voxware")]
      //    case 331:
      //      return AV_CODEC_ID_VOXWARE;
    case 332:
      return AV_CODEC_ID_APE;
    case 333:
      return AV_CODEC_ID_NELLYMOSER;
    case 334:
      return AV_CODEC_ID_MUSEPACK8;
    case 335:
      return AV_CODEC_ID_SPEEX;
    case 336:
      return AV_CODEC_ID_WMAVOICE;
    case 337:
      return AV_CODEC_ID_WMAPRO;
    case 338:
      return AV_CODEC_ID_WMALOSSLESS;
    case 339:
      return AV_CODEC_ID_ATRAC3P;
    case 340:
      return AV_CODEC_ID_EAC3;
    case 341:
      return AV_CODEC_ID_SIPR;
    case 342:
      return AV_CODEC_ID_MP1;
    case 343:
      return AV_CODEC_ID_TWINVQ;
    case 344:
      return AV_CODEC_ID_TRUEHD;
    case 345:
      return AV_CODEC_ID_MP4ALS;
    case 346:
      return AV_CODEC_ID_ATRAC1;
    case 347:
      return AV_CODEC_ID_BINKAUDIO_RDFT;
    case 348:
      return AV_CODEC_ID_BINKAUDIO_DCT;
    case 349:
      return AV_CODEC_ID_AAC_LATM;
    case 350:
      return AV_CODEC_ID_QDMC;
    case 351:
      return AV_CODEC_ID_CELT;
    case 352:
      return AV_CODEC_ID_G723_1;
    case 353:
      return AV_CODEC_ID_G729;
    case 354:
      return AV_CODEC_ID_8SVX_EXP;
    case 355:
      return AV_CODEC_ID_8SVX_FIB;
    case 356:
      return AV_CODEC_ID_BMV_AUDIO;
    case 357:
      return AV_CODEC_ID_RALF;
    case 358:
      return AV_CODEC_ID_IAC;
    case 359:
      return AV_CODEC_ID_ILBC;
    case 360:
      return AV_CODEC_ID_OPUS;
    case 361:
      return AV_CODEC_ID_COMFORT_NOISE;
    case 362:
      return AV_CODEC_ID_TAK;
    case 363:
      return AV_CODEC_ID_METASOUND;
    case 364:
      return AV_CODEC_ID_PAF_AUDIO;
    case 365:
      return AV_CODEC_ID_ON2AVC;
    case 366:
      return AV_CODEC_ID_DSS_SP;
    case 367:
      return AV_CODEC_ID_CODEC2;
    case 368:
      return AV_CODEC_ID_FFWAVESYNTH;
    case 369:
      return AV_CODEC_ID_SONIC;
    case 370:
      return AV_CODEC_ID_SONIC_LS;
    case 371:
      return AV_CODEC_ID_EVRC;
    case 372:
      return AV_CODEC_ID_SMV;
    case 373:
      return AV_CODEC_ID_DSD_LSBF;
    case 374:
      return AV_CODEC_ID_DSD_MSBF;
    case 375:
      return AV_CODEC_ID_DSD_LSBF_PLANAR;
    case 376:
      return AV_CODEC_ID_DSD_MSBF_PLANAR;
    case 377:
      return AV_CODEC_ID_4GV;
    case 378:
      return AV_CODEC_ID_INTERPLAY_ACM;
    case 379:
      return AV_CODEC_ID_XMA1;
    case 380:
      return AV_CODEC_ID_XMA2;
    case 381:
      return AV_CODEC_ID_DST;
      /////////////
      /////////////
      /////////////
    case 382:
      return AV_CODEC_ID_DVD_SUBTITLE;
    case 383:
      return AV_CODEC_ID_DVB_SUBTITLE;
    case 384:
      return AV_CODEC_ID_TEXT;
    case 385:
      return AV_CODEC_ID_XSUB;
    case 386:
      return AV_CODEC_ID_SSA;
    case 387:
      return AV_CODEC_ID_MOV_TEXT;
    case 388:
      return AV_CODEC_ID_HDMV_PGS_SUBTITLE;
    case 389:
      return AV_CODEC_ID_DVB_TELETEXT;
    case 390:
      return AV_CODEC_ID_SRT;
    case 391:
      return AV_CODEC_ID_MICRODVD;
    case 392:
      return AV_CODEC_ID_EIA_608;
    case 393:
      return AV_CODEC_ID_JACOSUB;
    case 394:
      return AV_CODEC_ID_SAMI;
    case 395:
      return AV_CODEC_ID_REALTEXT;
    case 396:
      return AV_CODEC_ID_STL;
    case 397:
      return AV_CODEC_ID_SUBVIEWER1;
    case 398:
      return AV_CODEC_ID_SUBVIEWER;
    case 399:
      return AV_CODEC_ID_SUBRIP;
    case 400:
      return AV_CODEC_ID_WEBVTT;
    case 401:
      return AV_CODEC_ID_MPL2;
    case 402:
      return AV_CODEC_ID_VPLAYER;
    case 403:
      return AV_CODEC_ID_PJS;
    case 404:
      return AV_CODEC_ID_ASS;
    case 405:
      return AV_CODEC_ID_HDMV_TEXT_SUBTITLE;
    case 406:
      return AV_CODEC_ID_TTF;
    case 407:
      return AV_CODEC_ID_SCTE_35;
    case 408:
      return AV_CODEC_ID_BINTEXT;
    case 409:
      return AV_CODEC_ID_XBIN;
    case 410:
      return AV_CODEC_ID_IDF;
    case 411:
      return AV_CODEC_ID_OTF;
    case 412:
      return AV_CODEC_ID_SMPTE_KLV;
    case 413:
      return AV_CODEC_ID_DVD_NAV;
    case 414:
      return AV_CODEC_ID_TIMED_ID3;
    case 415:
      return AV_CODEC_ID_BIN_DATA;
    case 416:
      return AV_CODEC_ID_PROBE;
    case 417:
      return AV_CODEC_ID_MPEG2TS;
    case 418:
      return AV_CODEC_ID_MPEG4SYSTEMS;
    case 419:
      return AV_CODEC_ID_FFMETADATA;
    case 420:
      return AV_CODEC_ID_WRAPPED_AVFRAME;
    case 421:
      return AV_CODEC_ID_PSD;
    case 422:
      return AV_CODEC_ID_PIXLET;
    case 423:
      return AV_CODEC_ID_SPEEDHQ;
    case 424:
      return AV_CODEC_ID_CLEARVIDEO;
    case 425:
      return AV_CODEC_ID_FMVC;
    case 426:
      return AV_CODEC_ID_SCPR;
    case 427:
      return AV_CODEC_ID_XPM;
    case 428:
      return AV_CODEC_ID_AV1;
    case 429:
      return AV_CODEC_ID_PCM_F16LE;
    case 430:
      return AV_CODEC_ID_PCM_F24LE;
      ////////////
    case 431:
      return AV_CODEC_ID_ATRAC3AL;
    case 432:
      return AV_CODEC_ID_ATRAC3PAL;
    case 433:
      return AV_CODEC_ID_BITPACKED;
    case 434:
      return AV_CODEC_ID_MSCC;
    case 435:
      return AV_CODEC_ID_SRGC;
    case 436:
      return AV_CODEC_ID_SVG;
    case 437:
      return AV_CODEC_ID_GDV;
    case 438:
      return AV_CODEC_ID_FITS;
    case 439:
      return AV_CODEC_ID_GREMLIN_DPCM;
    case 440:
      return AV_CODEC_ID_DOLBY_E;
    case 441:
      return AV_CODEC_ID_APTX;
    case 442:
      return AV_CODEC_ID_APTX_HD;
    case 443:
      return AV_CODEC_ID_SBC;
    case 444:
      return AV_CODEC_ID_AVS2;
    case 445:
      return AV_CODEC_ID_IMM4;
    case 446:
      return AV_CODEC_ID_PROSUMER;
    case 447:
      return AV_CODEC_ID_MWSC;
    case 448:
      return AV_CODEC_ID_WCMV;
    case 449:
      return AV_CODEC_ID_RASC;
    case 450:
      return AV_CODEC_ID_PCM_VIDC;
    case 451:
      return AV_CODEC_ID_ATRAC9;
    case 452:
      return AV_CODEC_ID_TTML;
    case 453:
      return AV_CODEC_ID_HYMT;
    case 454:
      return AV_CODEC_ID_ARBC;
    case 455:
      return AV_CODEC_ID_AGM;
    case 456:
      return AV_CODEC_ID_LSCR;
    case 457:
      return AV_CODEC_ID_VP4;
    case 458:
      return AV_CODEC_ID_ADPCM_AGM;
    case 459:
      return AV_CODEC_ID_HCOM;
    case 460:
      return AV_CODEC_ID_ARIB_CAPTION;
    case 461:
      return AV_CODEC_ID_IMM5;
    case 462:
      return AV_CODEC_ID_MVDV;
    case 463:
      return AV_CODEC_ID_MVHA;
    case 464:
      return AV_CODEC_ID_CDTOONS;
    case 465:
      return AV_CODEC_ID_MV30;
    case 466:
      return AV_CODEC_ID_NOTCHLC;
    case 467:
      return AV_CODEC_ID_PFM;
    case 468:
      return AV_CODEC_ID_ARGO;
    case 469:
      return AV_CODEC_ID_ADPCM_IMA_SSI;
    case 470:
      return AV_CODEC_ID_ADPCM_ZORK;
    case 471:
      return AV_CODEC_ID_ADPCM_IMA_APM;
    case 472:
      return AV_CODEC_ID_ADPCM_IMA_ALP;
    case 473:
      return AV_CODEC_ID_ADPCM_IMA_MTF;
    case 474:
      return AV_CODEC_ID_ADPCM_IMA_CUNNING;
    case 475:
      return AV_CODEC_ID_DERF_DPCM;
    case 476:
      return AV_CODEC_ID_ACELP_KELVIN;
    case 477:
      return AV_CODEC_ID_MPEGH_3D_AUDIO;
    case 478:
      return AV_CODEC_ID_SIREN;
    case 479:
      return AV_CODEC_ID_HCA;
    case 480:
      return AV_CODEC_ID_EPG;
    case 481:
      return AV_CODEC_ID_AVS3;
    case 482:
      return AV_CODEC_ID_PGX;
    case 483:
      return AV_CODEC_ID_MSP2;
    case 484:
      return AV_CODEC_ID_VVC;
    case 485:
      return AV_CODEC_ID_MOBICLIP;
    case 486:
      return AV_CODEC_ID_PHOTOCD;
    case 487:
      return AV_CODEC_ID_ADPCM_ARGO;
    case 488:
      return AV_CODEC_ID_CRI;
    case 489:
      return AV_CODEC_ID_IPU;
    case 490:
      return AV_CODEC_ID_SIMBIOSIS_IMX;
    case 491:
      return AV_CODEC_ID_SGA_VIDEO;
    case 492:
      return AV_CODEC_ID_PCM_SGA;
    case 493:
      return AV_CODEC_ID_ADPCM_IMA_MOFLEX;
    case 494:
      return AV_CODEC_ID_FASTAUDIO;
    case 495:
      return AV_CODEC_ID_GEM;
    case 496:
      return AV_CODEC_ID_ADPCM_IMA_ACORN;
    case 497:
      return AV_CODEC_ID_MSNSIREN;
    case 498:
      return AV_CODEC_ID_VBN;
    case 499:
      return AV_CODEC_ID_JPEGXL;
    case 500:
      return AV_CODEC_ID_QOI;
    case 501:
      return AV_CODEC_ID_PHM;
    case 502:
      return AV_CODEC_ID_DFPWM;
    case 503:
      return AV_CODEC_ID_RADIANCE_HDR;
    case 504:
      return AV_CODEC_ID_WBMP;
    case 505:
      return AV_CODEC_ID_MEDIA100;
    case 506:
      return AV_CODEC_ID_VQC;
    case 507:
      return AV_CODEC_ID_ADPCM_XMD;
    case 508:
      return AV_CODEC_ID_WADY_DPCM;
    case 509:
      return AV_CODEC_ID_CBD2_DPCM;
    case 510:
      return AV_CODEC_ID_BONK;
    case 511:
      return AV_CODEC_ID_MISC4;
    case 512:
      return AV_CODEC_ID_APAC;
    case 513:
      return AV_CODEC_ID_FTR;
    case 514:
      return AV_CODEC_ID_WAVARC;
    case 515:
      return AV_CODEC_ID_RKA;
    case 516:
      return AV_CODEC_ID_VNULL;
    case 517:
      return AV_CODEC_ID_ANULL;
      //    case 518:
      //      return AV_CODEC_ID_MPEG2VIDEO_XVMC;
    default:
      return AV_CODEC_ID_NONE;
    };
  }

  // Convert AVCodecID to uint32_t for rust SDK.
  static uint32_t fromAVCodecID(AVCodecID AvCodecId) {
    switch (AvCodecId) {
    case AV_CODEC_ID_NONE:
      return 0;
    case AV_CODEC_ID_MPEG1VIDEO:
      return 1;
    case AV_CODEC_ID_MPEG2VIDEO:
      return 2;
    case AV_CODEC_ID_H261:
      return 3;
    case AV_CODEC_ID_H263:
      return 4;
    case AV_CODEC_ID_RV10:
      return 5;
    case AV_CODEC_ID_RV20:
      return 6;
    case AV_CODEC_ID_MJPEG:
      return 7;
    case AV_CODEC_ID_MJPEGB:
      return 8;
    case AV_CODEC_ID_LJPEG:
      return 9;
    case AV_CODEC_ID_SP5X:
      return 10;
    case AV_CODEC_ID_JPEGLS:
      return 11;
    case AV_CODEC_ID_MPEG4:
      return 12;
    case AV_CODEC_ID_RAWVIDEO:
      return 13;
    case AV_CODEC_ID_MSMPEG4V1:
      return 14;
    case AV_CODEC_ID_MSMPEG4V2:
      return 15;
    case AV_CODEC_ID_MSMPEG4V3:
      return 16;
    case AV_CODEC_ID_WMV1:
      return 17;
    case AV_CODEC_ID_WMV2:
      return 18;
    case AV_CODEC_ID_H263P:
      return 19;
    case AV_CODEC_ID_H263I:
      return 20;
    case AV_CODEC_ID_FLV1:
      return 21;
    case AV_CODEC_ID_SVQ1:
      return 22;
    case AV_CODEC_ID_SVQ3:
      return 23;
    case AV_CODEC_ID_DVVIDEO:
      return 24;
    case AV_CODEC_ID_HUFFYUV:
      return 25;
    case AV_CODEC_ID_CYUV:
      return 26;
    case AV_CODEC_ID_H264:
      return 27;
    case AV_CODEC_ID_INDEO3:
      return 28;
    case AV_CODEC_ID_VP3:
      return 29;
    case AV_CODEC_ID_THEORA:
      return 30;
    case AV_CODEC_ID_ASV1:
      return 31;
    case AV_CODEC_ID_ASV2:
      return 32;
    case AV_CODEC_ID_FFV1:
      return 33;
    case AV_CODEC_ID_4XM:
      return 34;
    case AV_CODEC_ID_VCR1:
      return 35;
    case AV_CODEC_ID_CLJR:
      return 36;
    case AV_CODEC_ID_MDEC:
      return 37;
    case AV_CODEC_ID_ROQ:
      return 38;
    case AV_CODEC_ID_INTERPLAY_VIDEO:
      return 39;
    case AV_CODEC_ID_XAN_WC3:
      return 40;
    case AV_CODEC_ID_XAN_WC4:
      return 41;
    case AV_CODEC_ID_RPZA:
      return 42;
    case AV_CODEC_ID_CINEPAK:
      return 43;
    case AV_CODEC_ID_WS_VQA:
      return 44;
    case AV_CODEC_ID_MSRLE:
      return 45;
    case AV_CODEC_ID_MSVIDEO1:
      return 46;
    case AV_CODEC_ID_IDCIN:
      return 47;
    case AV_CODEC_ID_8BPS:
      return 48;
    case AV_CODEC_ID_SMC:
      return 49;
    case AV_CODEC_ID_FLIC:
      return 50;
    case AV_CODEC_ID_TRUEMOTION1:
      return 51;
    case AV_CODEC_ID_VMDVIDEO:
      return 52;
    case AV_CODEC_ID_MSZH:
      return 53;
    case AV_CODEC_ID_ZLIB:
      return 54;
    case AV_CODEC_ID_QTRLE:
      return 55;
    case AV_CODEC_ID_TSCC:
      return 56;
    case AV_CODEC_ID_ULTI:
      return 57;
    case AV_CODEC_ID_QDRAW:
      return 58;
    case AV_CODEC_ID_VIXL:
      return 59;
    case AV_CODEC_ID_QPEG:
      return 60;
    case AV_CODEC_ID_PNG:
      return 61;
    case AV_CODEC_ID_PPM:
      return 62;
    case AV_CODEC_ID_PBM:
      return 63;
    case AV_CODEC_ID_PGM:
      return 64;
    case AV_CODEC_ID_PGMYUV:
      return 65;
    case AV_CODEC_ID_PAM:
      return 66;
    case AV_CODEC_ID_FFVHUFF:
      return 67;
    case AV_CODEC_ID_RV30:
      return 68;
    case AV_CODEC_ID_RV40:
      return 69;
    case AV_CODEC_ID_VC1:
      return 70;
    case AV_CODEC_ID_WMV3:
      return 71;
    case AV_CODEC_ID_LOCO:
      return 72;
    case AV_CODEC_ID_WNV1:
      return 73;
    case AV_CODEC_ID_AASC:
      return 74;
    case AV_CODEC_ID_INDEO2:
      return 75;
    case AV_CODEC_ID_FRAPS:
      return 76;
    case AV_CODEC_ID_TRUEMOTION2:
      return 77;
    case AV_CODEC_ID_BMP:
      return 78;
    case AV_CODEC_ID_CSCD:
      return 79;
    case AV_CODEC_ID_MMVIDEO:
      return 80;
    case AV_CODEC_ID_ZMBV:
      return 81;
    case AV_CODEC_ID_AVS:
      return 82;
    case AV_CODEC_ID_SMACKVIDEO:
      return 83;
    case AV_CODEC_ID_NUV:
      return 84;
    case AV_CODEC_ID_KMVC:
      return 85;
    case AV_CODEC_ID_FLASHSV:
      return 86;
    case AV_CODEC_ID_CAVS:
      return 87;
    case AV_CODEC_ID_JPEG2000:
      return 88;
    case AV_CODEC_ID_VMNC:
      return 89;
    case AV_CODEC_ID_VP5:
      return 90;
    case AV_CODEC_ID_VP6:
      return 91;
    case AV_CODEC_ID_VP6F:
      return 92;
    case AV_CODEC_ID_TARGA:
      return 93;
    case AV_CODEC_ID_DSICINVIDEO:
      return 94;
    case AV_CODEC_ID_TIERTEXSEQVIDEO:
      return 95;
    case AV_CODEC_ID_TIFF:
      return 96;
    case AV_CODEC_ID_GIF:
      return 97;
    case AV_CODEC_ID_DXA:
      return 98;
    case AV_CODEC_ID_DNXHD:
      return 99;
    case AV_CODEC_ID_THP:
      return 100;
    case AV_CODEC_ID_SGI:
      return 101;
    case AV_CODEC_ID_C93:
      return 102;
    case AV_CODEC_ID_BETHSOFTVID:
      return 103;
    case AV_CODEC_ID_PTX:
      return 104;
    case AV_CODEC_ID_TXD:
      return 105;
    case AV_CODEC_ID_VP6A:
      return 106;
    case AV_CODEC_ID_AMV:
      return 107;
    case AV_CODEC_ID_VB:
      return 108;
    case AV_CODEC_ID_PCX:
      return 109;
    case AV_CODEC_ID_SUNRAST:
      return 110;
    case AV_CODEC_ID_INDEO4:
      return 111;
    case AV_CODEC_ID_INDEO5:
      return 112;
    case AV_CODEC_ID_MIMIC:
      return 113;
    case AV_CODEC_ID_RL2:
      return 114;
    case AV_CODEC_ID_ESCAPE124:
      return 115;
    case AV_CODEC_ID_DIRAC:
      return 116;
    case AV_CODEC_ID_BFI:
      return 117;
    case AV_CODEC_ID_CMV:
      return 118;
    case AV_CODEC_ID_MOTIONPIXELS:
      return 119;
    case AV_CODEC_ID_TGV:
      return 120;
    case AV_CODEC_ID_TGQ:
      return 121;
    case AV_CODEC_ID_TQI:
      return 122;
    case AV_CODEC_ID_AURA:
      return 123;
    case AV_CODEC_ID_AURA2:
      return 124;
    case AV_CODEC_ID_V210X:
      return 125;
    case AV_CODEC_ID_TMV:
      return 126;
    case AV_CODEC_ID_V210:
      return 127;
    case AV_CODEC_ID_DPX:
      return 128;
    case AV_CODEC_ID_MAD:
      return 129;
    case AV_CODEC_ID_FRWU:
      return 130;
    case AV_CODEC_ID_FLASHSV2:
      return 131;
    case AV_CODEC_ID_CDGRAPHICS:
      return 132;
    case AV_CODEC_ID_R210:
      return 133;
    case AV_CODEC_ID_ANM:
      return 134;
    case AV_CODEC_ID_BINKVIDEO:
      return 135;
    case AV_CODEC_ID_IFF_ILBM:
      return 136;
      //    case AV_CODEC_ID_IFF_ILBM:
      //      return 137;
    case AV_CODEC_ID_KGV1:
      return 138;
    case AV_CODEC_ID_YOP:
      return 139;
    case AV_CODEC_ID_VP8:
      return 140;
    case AV_CODEC_ID_PICTOR:
      return 141;
    case AV_CODEC_ID_ANSI:
      return 142;
    case AV_CODEC_ID_A64_MULTI:
      return 143;
    case AV_CODEC_ID_A64_MULTI5:
      return 144;
    case AV_CODEC_ID_R10K:
      return 145;
    case AV_CODEC_ID_MXPEG:
      return 146;
    case AV_CODEC_ID_LAGARITH:
      return 147;
    case AV_CODEC_ID_PRORES:
      return 148;
    case AV_CODEC_ID_JV:
      return 149;
    case AV_CODEC_ID_DFA:
      return 150;
    case AV_CODEC_ID_WMV3IMAGE:
      return 151;
    case AV_CODEC_ID_VC1IMAGE:
      return 152;
    case AV_CODEC_ID_UTVIDEO:
      return 153;
    case AV_CODEC_ID_BMV_VIDEO:
      return 154;
    case AV_CODEC_ID_VBLE:
      return 155;
    case AV_CODEC_ID_DXTORY:
      return 156;
    case AV_CODEC_ID_V410:
      return 157;
    case AV_CODEC_ID_XWD:
      return 158;
    case AV_CODEC_ID_CDXL:
      return 159;
    case AV_CODEC_ID_XBM:
      return 160;
    case AV_CODEC_ID_ZEROCODEC:
      return 161;
    case AV_CODEC_ID_MSS1:
      return 162;
    case AV_CODEC_ID_MSA1:
      return 163;
    case AV_CODEC_ID_TSCC2:
      return 164;
    case AV_CODEC_ID_MTS2:
      return 165;
    case AV_CODEC_ID_CLLC:
      return 166;
    case AV_CODEC_ID_MSS2:
      return 167;
    case AV_CODEC_ID_VP9:
      return 168;
    case AV_CODEC_ID_AIC:
      return 169;
    case AV_CODEC_ID_ESCAPE130:
      return 170;
    case AV_CODEC_ID_G2M:
      return 171;
    case AV_CODEC_ID_WEBP:
      return 172;
    case AV_CODEC_ID_HNM4_VIDEO:
      return 173;
    case AV_CODEC_ID_HEVC:
      return 174;
      //    case AV_CODEC_ID_HEVC:
      //      return 175;
    case AV_CODEC_ID_FIC:
      return 176;
    case AV_CODEC_ID_ALIAS_PIX:
      return 177;
    case AV_CODEC_ID_BRENDER_PIX:
      return 178;
    case AV_CODEC_ID_PAF_VIDEO:
      return 179;
    case AV_CODEC_ID_EXR:
      return 180;
    case AV_CODEC_ID_VP7:
      return 181;
    case AV_CODEC_ID_SANM:
      return 182;
    case AV_CODEC_ID_SGIRLE:
      return 183;
    case AV_CODEC_ID_MVC1:
      return 184;
    case AV_CODEC_ID_MVC2:
      return 185;
    case AV_CODEC_ID_HQX:
      return 186;
    case AV_CODEC_ID_TDSC:
      return 187;
    case AV_CODEC_ID_HQ_HQA:
      return 188;
    case AV_CODEC_ID_HAP:
      return 189;
    case AV_CODEC_ID_DDS:
      return 190;
    case AV_CODEC_ID_DXV:
      return 191;
    case AV_CODEC_ID_SCREENPRESSO:
      return 192;
    case AV_CODEC_ID_RSCC:
      return 193;
      ///////////////////////////////
      //    return ;
      //      case AV_CODEC_ID_Y41P:
      //    return ;
      //      case AV_CODEC_ID_AVS2:
    case AV_CODEC_ID_Y41P:
      return 194;
    case AV_CODEC_ID_AVRP:
      return 195;
    case AV_CODEC_ID_012V:
      return 196;
    case AV_CODEC_ID_AVUI:
      return 197;
    case AV_CODEC_ID_AYUV:
      return 198;
    case AV_CODEC_ID_TARGA_Y216:
      return 199;
    case AV_CODEC_ID_V308:
      return 200;
    case AV_CODEC_ID_V408:
      return 201;
    case AV_CODEC_ID_YUV4:
      return 202;
    case AV_CODEC_ID_AVRN:
      return 203;
    case AV_CODEC_ID_CPIA:
      return 204;
    case AV_CODEC_ID_XFACE:
      return 205;
    case AV_CODEC_ID_SNOW:
      return 206;
    case AV_CODEC_ID_SMVJPEG:
      return 207;
    case AV_CODEC_ID_APNG:
      return 208;
    case AV_CODEC_ID_DAALA:
      return 209;
    case AV_CODEC_ID_CFHD:
      return 210;
    case AV_CODEC_ID_TRUEMOTION2RT:
      return 211;
    case AV_CODEC_ID_M101:
      return 212;
    case AV_CODEC_ID_MAGICYUV:
      return 213;
    case AV_CODEC_ID_SHEERVIDEO:
      return 214;
    case AV_CODEC_ID_YLC:
      return 215;
      // =================================
      // =================================
      // =================================
    case AV_CODEC_ID_PCM_S16LE:
      return 216;
    case AV_CODEC_ID_PCM_S16BE:
      return 217;
    case AV_CODEC_ID_PCM_U16LE:
      return 218;
    case AV_CODEC_ID_PCM_U16BE:
      return 219;
    case AV_CODEC_ID_PCM_S8:
      return 220;
    case AV_CODEC_ID_PCM_U8:
      return 221;
    case AV_CODEC_ID_PCM_MULAW:
      return 222;
    case AV_CODEC_ID_PCM_ALAW:
      return 223;
    case AV_CODEC_ID_PCM_S32LE:
      return 224;
    case AV_CODEC_ID_PCM_S32BE:
      return 225;
    case AV_CODEC_ID_PCM_U32LE:
      return 226;
    case AV_CODEC_ID_PCM_U32BE:
      return 227;
    case AV_CODEC_ID_PCM_S24LE:
      return 228;
    case AV_CODEC_ID_PCM_S24BE:
      return 229;
    case AV_CODEC_ID_PCM_U24LE:
      return 230;
    case AV_CODEC_ID_PCM_U24BE:
      return 231;
    case AV_CODEC_ID_PCM_S24DAUD:
      return 232;
    case AV_CODEC_ID_PCM_ZORK:
      return 233;
    case AV_CODEC_ID_PCM_S16LE_PLANAR:
      return 234;
    case AV_CODEC_ID_PCM_DVD:
      return 235;
    case AV_CODEC_ID_PCM_F32BE:
      return 236;
    case AV_CODEC_ID_PCM_F32LE:
      return 237;
    case AV_CODEC_ID_PCM_F64BE:
      return 238;
    case AV_CODEC_ID_PCM_F64LE:
      return 239;
    case AV_CODEC_ID_PCM_BLURAY:
      return 240;
    case AV_CODEC_ID_PCM_LXF:
      return 241;
    case AV_CODEC_ID_S302M:
      return 242;
    case AV_CODEC_ID_PCM_S8_PLANAR:
      return 243;
    case AV_CODEC_ID_PCM_S24LE_PLANAR:
      return 244;
    case AV_CODEC_ID_PCM_S32LE_PLANAR:
      return 245;
    case AV_CODEC_ID_PCM_S16BE_PLANAR:
      return 246;
    case AV_CODEC_ID_PCM_S64LE:
      return 247;
    case AV_CODEC_ID_PCM_S64BE:
      return 248;
    case AV_CODEC_ID_ADPCM_IMA_QT:
      return 249;
    case AV_CODEC_ID_ADPCM_IMA_WAV:
      return 250;
    case AV_CODEC_ID_ADPCM_IMA_DK3:
      return 251;
    case AV_CODEC_ID_ADPCM_IMA_DK4:
      return 252;
    case AV_CODEC_ID_ADPCM_IMA_WS:
      return 253;
    case AV_CODEC_ID_ADPCM_IMA_SMJPEG:
      return 254;
    case AV_CODEC_ID_ADPCM_MS:
      return 255;
    case AV_CODEC_ID_ADPCM_4XM:
      return 256;
    case AV_CODEC_ID_ADPCM_XA:
      return 257;
    case AV_CODEC_ID_ADPCM_ADX:
      return 258;
    case AV_CODEC_ID_ADPCM_EA:
      return 259;
    case AV_CODEC_ID_ADPCM_G726:
      return 260;
    case AV_CODEC_ID_ADPCM_CT:
      return 261;
    case AV_CODEC_ID_ADPCM_SWF:
      return 262;
    case AV_CODEC_ID_ADPCM_YAMAHA:
      return 263;
    case AV_CODEC_ID_ADPCM_SBPRO_4:
      return 264;
    case AV_CODEC_ID_ADPCM_SBPRO_3:
      return 265;
    case AV_CODEC_ID_ADPCM_SBPRO_2:
      return 266;
    case AV_CODEC_ID_ADPCM_THP:
      return 267;
    case AV_CODEC_ID_ADPCM_IMA_AMV:
      return 268;
    case AV_CODEC_ID_ADPCM_EA_R1:
      return 269;
    case AV_CODEC_ID_ADPCM_EA_R3:
      return 270;
    case AV_CODEC_ID_ADPCM_EA_R2:
      return 271;
    case AV_CODEC_ID_ADPCM_IMA_EA_SEAD:
      return 272;
    case AV_CODEC_ID_ADPCM_IMA_EA_EACS:
      return 273;
    case AV_CODEC_ID_ADPCM_EA_XAS:
      return 274;
    case AV_CODEC_ID_ADPCM_EA_MAXIS_XA:
      return 275;
    case AV_CODEC_ID_ADPCM_IMA_ISS:
      return 276;
    case AV_CODEC_ID_ADPCM_G722:
      return 277;
    case AV_CODEC_ID_ADPCM_IMA_APC:
      return 278;
    case AV_CODEC_ID_ADPCM_VIMA:
      return 279;
    case AV_CODEC_ID_ADPCM_AFC:
      return 280;
    case AV_CODEC_ID_ADPCM_IMA_OKI:
      return 281;
    case AV_CODEC_ID_ADPCM_DTK:
      return 282;
    case AV_CODEC_ID_ADPCM_IMA_RAD:
      return 283;
    case AV_CODEC_ID_ADPCM_G726LE:
      return 284;
    case AV_CODEC_ID_ADPCM_THP_LE:
      return 285;
    case AV_CODEC_ID_ADPCM_PSX:
      return 286;
    case AV_CODEC_ID_ADPCM_AICA:
      return 287;
    case AV_CODEC_ID_ADPCM_IMA_DAT4:
      return 288;
    case AV_CODEC_ID_ADPCM_MTAF:
      return 289;
    case AV_CODEC_ID_AMR_NB:
      return 290;
    case AV_CODEC_ID_AMR_WB:
      return 291;
    case AV_CODEC_ID_RA_144:
      return 292;
    case AV_CODEC_ID_RA_288:
      return 293;
    case AV_CODEC_ID_ROQ_DPCM:
      return 294;
    case AV_CODEC_ID_INTERPLAY_DPCM:
      return 295;
    case AV_CODEC_ID_XAN_DPCM:
      return 296;
    case AV_CODEC_ID_SOL_DPCM:
      return 297;
    case AV_CODEC_ID_SDX2_DPCM:
      return 298;
    case AV_CODEC_ID_MP2:
      return 299;
    case AV_CODEC_ID_MP3:
      return 300;
    case AV_CODEC_ID_AAC:
      return 301;
    case AV_CODEC_ID_AC3:
      return 302;
    case AV_CODEC_ID_DTS:
      return 303;
    case AV_CODEC_ID_VORBIS:
      return 304;
    case AV_CODEC_ID_DVAUDIO:
      return 305;
    case AV_CODEC_ID_WMAV1:
      return 306;
    case AV_CODEC_ID_WMAV2:
      return 307;
    case AV_CODEC_ID_MACE3:
      return 308;
    case AV_CODEC_ID_MACE6:
      return 309;
    case AV_CODEC_ID_VMDAUDIO:
      return 310;
    case AV_CODEC_ID_FLAC:
      return 311;
    case AV_CODEC_ID_MP3ADU:
      return 312;
    case AV_CODEC_ID_MP3ON4:
      return 313;
    case AV_CODEC_ID_SHORTEN:
      return 314;
    case AV_CODEC_ID_ALAC:
      return 315;
    case AV_CODEC_ID_WESTWOOD_SND1:
      return 316;
    case AV_CODEC_ID_GSM:
      return 317;
    case AV_CODEC_ID_QDM2:
      return 318;
    case AV_CODEC_ID_COOK:
      return 319;
    case AV_CODEC_ID_TRUESPEECH:
      return 320;
    case AV_CODEC_ID_TTA:
      return 321;
    case AV_CODEC_ID_SMACKAUDIO:
      return 322;
    case AV_CODEC_ID_QCELP:
      return 323;
    case AV_CODEC_ID_WAVPACK:
      return 324;
    case AV_CODEC_ID_DSICINAUDIO:
      return 325;
    case AV_CODEC_ID_IMC:
      return 326;
    case AV_CODEC_ID_MUSEPACK7:
      return 327;
    case AV_CODEC_ID_MLP:
      return 328;
    case AV_CODEC_ID_GSM_MS:
      return 329;
    case AV_CODEC_ID_ATRAC3:
      return 330;
      // #[cfg(feature = "ff_api_voxware")]
      //      case AV_CODEC_ID_VOXWARE:
      //    return 331;
    case AV_CODEC_ID_APE:
      return 332;
    case AV_CODEC_ID_NELLYMOSER:
      return 333;
    case AV_CODEC_ID_MUSEPACK8:
      return 334;
    case AV_CODEC_ID_SPEEX:
      return 335;
    case AV_CODEC_ID_WMAVOICE:
      return 336;
    case AV_CODEC_ID_WMAPRO:
      return 337;
    case AV_CODEC_ID_WMALOSSLESS:
      return 338;
    case AV_CODEC_ID_ATRAC3P:
      return 339;
    case AV_CODEC_ID_EAC3:
      return 340;
    case AV_CODEC_ID_SIPR:
      return 341;
    case AV_CODEC_ID_MP1:
      return 342;
    case AV_CODEC_ID_TWINVQ:
      return 343;
    case AV_CODEC_ID_TRUEHD:
      return 344;
    case AV_CODEC_ID_MP4ALS:
      return 345;
    case AV_CODEC_ID_ATRAC1:
      return 346;
    case AV_CODEC_ID_BINKAUDIO_RDFT:
      return 347;
    case AV_CODEC_ID_BINKAUDIO_DCT:
      return 348;
    case AV_CODEC_ID_AAC_LATM:
      return 349;
    case AV_CODEC_ID_QDMC:
      return 350;
    case AV_CODEC_ID_CELT:
      return 351;
    case AV_CODEC_ID_G723_1:
      return 352;
    case AV_CODEC_ID_G729:
      return 353;
    case AV_CODEC_ID_8SVX_EXP:
      return 354;
    case AV_CODEC_ID_8SVX_FIB:
      return 355;
    case AV_CODEC_ID_BMV_AUDIO:
      return 356;
    case AV_CODEC_ID_RALF:
      return 357;
    case AV_CODEC_ID_IAC:
      return 358;
    case AV_CODEC_ID_ILBC:
      return 359;
    case AV_CODEC_ID_OPUS:
      return 360;
    case AV_CODEC_ID_COMFORT_NOISE:
      return 361;
    case AV_CODEC_ID_TAK:
      return 362;
    case AV_CODEC_ID_METASOUND:
      return 363;
    case AV_CODEC_ID_PAF_AUDIO:
      return 364;
    case AV_CODEC_ID_ON2AVC:
      return 365;
    case AV_CODEC_ID_DSS_SP:
      return 366;
    case AV_CODEC_ID_CODEC2:
      return 367;
    case AV_CODEC_ID_FFWAVESYNTH:
      return 368;
    case AV_CODEC_ID_SONIC:
      return 369;
    case AV_CODEC_ID_SONIC_LS:
      return 370;
    case AV_CODEC_ID_EVRC:
      return 371;
    case AV_CODEC_ID_SMV:
      return 372;
    case AV_CODEC_ID_DSD_LSBF:
      return 373;
    case AV_CODEC_ID_DSD_MSBF:
      return 374;
    case AV_CODEC_ID_DSD_LSBF_PLANAR:
      return 375;
    case AV_CODEC_ID_DSD_MSBF_PLANAR:
      return 376;
    case AV_CODEC_ID_4GV:
      return 377;
    case AV_CODEC_ID_INTERPLAY_ACM:
      return 378;
    case AV_CODEC_ID_XMA1:
      return 379;
    case AV_CODEC_ID_XMA2:
      return 380;
    case AV_CODEC_ID_DST:
      return 381;
    case AV_CODEC_ID_DVD_SUBTITLE:
      return 382;
    case AV_CODEC_ID_DVB_SUBTITLE:
      return 383;
    case AV_CODEC_ID_TEXT:
      return 384;
    case AV_CODEC_ID_XSUB:
      return 385;
    case AV_CODEC_ID_SSA:
      return 386;
    case AV_CODEC_ID_MOV_TEXT:
      return 387;
    case AV_CODEC_ID_HDMV_PGS_SUBTITLE:
      return 388;
    case AV_CODEC_ID_DVB_TELETEXT:
      return 389;
    case AV_CODEC_ID_SRT:
      return 390;
    case AV_CODEC_ID_MICRODVD:
      return 391;
    case AV_CODEC_ID_EIA_608:
      return 392;
    case AV_CODEC_ID_JACOSUB:
      return 393;
    case AV_CODEC_ID_SAMI:
      return 394;
    case AV_CODEC_ID_REALTEXT:
      return 395;
    case AV_CODEC_ID_STL:
      return 396;
    case AV_CODEC_ID_SUBVIEWER1:
      return 397;
    case AV_CODEC_ID_SUBVIEWER:
      return 398;
    case AV_CODEC_ID_SUBRIP:
      return 399;
    case AV_CODEC_ID_WEBVTT:
      return 400;
    case AV_CODEC_ID_MPL2:
      return 401;
    case AV_CODEC_ID_VPLAYER:
      return 402;
    case AV_CODEC_ID_PJS:
      return 403;
    case AV_CODEC_ID_ASS:
      return 404;
    case AV_CODEC_ID_HDMV_TEXT_SUBTITLE:
      return 405;
    case AV_CODEC_ID_TTF:
      return 406;
    case AV_CODEC_ID_SCTE_35:
      return 407;
    case AV_CODEC_ID_BINTEXT:
      return 408;
    case AV_CODEC_ID_XBIN:
      return 409;
    case AV_CODEC_ID_IDF:
      return 410;
    case AV_CODEC_ID_OTF:
      return 411;
    case AV_CODEC_ID_SMPTE_KLV:
      return 412;
    case AV_CODEC_ID_DVD_NAV:
      return 413;
    case AV_CODEC_ID_TIMED_ID3:
      return 414;
    case AV_CODEC_ID_BIN_DATA:
      return 415;
    case AV_CODEC_ID_PROBE:
      return 416;
    case AV_CODEC_ID_MPEG2TS:
      return 417;
    case AV_CODEC_ID_MPEG4SYSTEMS:
      return 418;
    case AV_CODEC_ID_FFMETADATA:
      return 419;
    case AV_CODEC_ID_WRAPPED_AVFRAME:
      return 420;
    case AV_CODEC_ID_PSD:
      return 421;
    case AV_CODEC_ID_PIXLET:
      return 422;
    case AV_CODEC_ID_SPEEDHQ:
      return 423;
    case AV_CODEC_ID_CLEARVIDEO:
      return 424;
    case AV_CODEC_ID_FMVC:
      return 425;
    case AV_CODEC_ID_SCPR:
      return 426;
    case AV_CODEC_ID_XPM:
      return 427;
    case AV_CODEC_ID_AV1:
      return 428;
    case AV_CODEC_ID_PCM_F16LE:
      return 429;
    case AV_CODEC_ID_PCM_F24LE:
      return 430;
      ////////////
    case AV_CODEC_ID_ATRAC3AL:
      return 431;
    case AV_CODEC_ID_ATRAC3PAL:
      return 432;
    case AV_CODEC_ID_BITPACKED:
      return 433;
    case AV_CODEC_ID_MSCC:
      return 434;
    case AV_CODEC_ID_SRGC:
      return 435;
    case AV_CODEC_ID_SVG:
      return 436;
    case AV_CODEC_ID_GDV:
      return 437;
    case AV_CODEC_ID_FITS:
      return 438;
    case AV_CODEC_ID_GREMLIN_DPCM:
      return 439;
    case AV_CODEC_ID_DOLBY_E:
      return 440;
    case AV_CODEC_ID_APTX:
      return 441;
    case AV_CODEC_ID_APTX_HD:
      return 442;
    case AV_CODEC_ID_SBC:
      return 443;
    case AV_CODEC_ID_AVS2:
      return 444;
    case AV_CODEC_ID_IMM4:
      return 445;
    case AV_CODEC_ID_PROSUMER:
      return 446;
    case AV_CODEC_ID_MWSC:
      return 447;
    case AV_CODEC_ID_WCMV:
      return 448;
    case AV_CODEC_ID_RASC:
      return 449;
    case AV_CODEC_ID_PCM_VIDC:
      return 450;
    case AV_CODEC_ID_ATRAC9:
      return 451;
    case AV_CODEC_ID_TTML:
      return 452;
    case AV_CODEC_ID_HYMT:
      return 453;
    case AV_CODEC_ID_ARBC:
      return 454;
    case AV_CODEC_ID_AGM:
      return 455;
    case AV_CODEC_ID_LSCR:
      return 456;
    case AV_CODEC_ID_VP4:
      return 457;
    case AV_CODEC_ID_ADPCM_AGM:
      return 458;
    case AV_CODEC_ID_HCOM:
      return 459;
    case AV_CODEC_ID_ARIB_CAPTION:
      return 460;
    case AV_CODEC_ID_IMM5:
      return 461;
    case AV_CODEC_ID_MVDV:
      return 462;
    case AV_CODEC_ID_MVHA:
      return 463;
    case AV_CODEC_ID_CDTOONS:
      return 464;
    case AV_CODEC_ID_MV30:
      return 465;
    case AV_CODEC_ID_NOTCHLC:
      return 466;
    case AV_CODEC_ID_PFM:
      return 467;
    case AV_CODEC_ID_ARGO:
      return 468;
    case AV_CODEC_ID_ADPCM_IMA_SSI:
      return 469;
    case AV_CODEC_ID_ADPCM_ZORK:
      return 470;
    case AV_CODEC_ID_ADPCM_IMA_APM:
      return 471;
    case AV_CODEC_ID_ADPCM_IMA_ALP:
      return 472;
    case AV_CODEC_ID_ADPCM_IMA_MTF:
      return 473;
    case AV_CODEC_ID_ADPCM_IMA_CUNNING:
      return 474;
    case AV_CODEC_ID_DERF_DPCM:
      return 475;
    case AV_CODEC_ID_ACELP_KELVIN:
      return 476;
    case AV_CODEC_ID_MPEGH_3D_AUDIO:
      return 477;
    case AV_CODEC_ID_SIREN:
      return 478;
    case AV_CODEC_ID_HCA:
      return 479;
    case AV_CODEC_ID_EPG:
      return 480;
    case AV_CODEC_ID_AVS3:
      return 481;
    case AV_CODEC_ID_PGX:
      return 482;
    case AV_CODEC_ID_MSP2:
      return 483;
    case AV_CODEC_ID_VVC:
      return 484;
    case AV_CODEC_ID_MOBICLIP:
      return 485;
    case AV_CODEC_ID_PHOTOCD:
      return 486;
    case AV_CODEC_ID_ADPCM_ARGO:
      return 487;
    case AV_CODEC_ID_CRI:
      return 488;
    case AV_CODEC_ID_IPU:
      return 489;
    case AV_CODEC_ID_SIMBIOSIS_IMX:
      return 490;
    case AV_CODEC_ID_SGA_VIDEO:
      return 491;
    case AV_CODEC_ID_PCM_SGA:
      return 492;
    case AV_CODEC_ID_ADPCM_IMA_MOFLEX:
      return 493;
    case AV_CODEC_ID_FASTAUDIO:
      return 494;
    case AV_CODEC_ID_GEM:
      return 495;
    case AV_CODEC_ID_ADPCM_IMA_ACORN:
      return 496;
    case AV_CODEC_ID_MSNSIREN:
      return 497;
    case AV_CODEC_ID_VBN:
      return 498;
    case AV_CODEC_ID_JPEGXL:
      return 499;
    case AV_CODEC_ID_QOI:
      return 500;
    case AV_CODEC_ID_PHM:
      return 501;
    case AV_CODEC_ID_DFPWM:
      return 502;
    case AV_CODEC_ID_RADIANCE_HDR:
      return 503;
    case AV_CODEC_ID_WBMP:
      return 504;
    case AV_CODEC_ID_MEDIA100:
      return 505;
    case AV_CODEC_ID_VQC:
      return 506;
    case AV_CODEC_ID_ADPCM_XMD:
      return 507;
    case AV_CODEC_ID_WADY_DPCM:
      return 508;
    case AV_CODEC_ID_CBD2_DPCM:
      return 509;
    case AV_CODEC_ID_BONK:
      return 510;
    case AV_CODEC_ID_MISC4:
      return 511;
    case AV_CODEC_ID_APAC:
      return 512;
    case AV_CODEC_ID_FTR:
      return 513;
    case AV_CODEC_ID_WAVARC:
      return 514;
    case AV_CODEC_ID_RKA:
      return 515;
    case AV_CODEC_ID_VNULL:
      return 516;
    case AV_CODEC_ID_ANULL:
      return 517;
      //      case AV_CODEC_ID_MPEG2VIDEO_XVMC:
      //    return 518;
    default:
      return 0;
    }
  }
};

class PixFmt {

public:
  static uint32_t fromAVPixFmt(AVPixelFormat AvPixelFormat) {
    switch (AvPixelFormat) {
    case AV_PIX_FMT_NONE:
      return 0;
    case AV_PIX_FMT_YUV420P:
      return 1;
    case AV_PIX_FMT_YUYV422:
      return 2;
    case AV_PIX_FMT_RGB24:
      return 3;
    case AV_PIX_FMT_BGR24:
      return 4;
    case AV_PIX_FMT_YUV422P:
      return 5;
    case AV_PIX_FMT_YUV444P:
      return 7;
    case AV_PIX_FMT_YUV410P:
      return 8;
    case AV_PIX_FMT_YUV411P:
      return 9;
    case AV_PIX_FMT_GRAY8:
      return 10;
    case AV_PIX_FMT_MONOWHITE:
      return 11;
    case AV_PIX_FMT_MONOBLACK:
      return 12;
    case AV_PIX_FMT_PAL8:
      return 13;
    case AV_PIX_FMT_YUVJ420P:
      return 14;
    case AV_PIX_FMT_YUVJ422P:
      return 15;
    case AV_PIX_FMT_YUVJ444P:
      return 16;
      //       case AV_PIX_FMT_XVMC_MPEG2_MC :     // Lower FFmpeg Version
      //     return 17;
      //       case AV_PIX_FMT_XVMC_MPEG2_IDCT :
      //     return 18;
    case AV_PIX_FMT_UYVY422:
      return 19;
    case AV_PIX_FMT_UYYVYY411:
      return 20;
    case AV_PIX_FMT_BGR8:
      return 21;
    case AV_PIX_FMT_BGR4:
      return 22;
    case AV_PIX_FMT_BGR4_BYTE:
      return 23;
    case AV_PIX_FMT_RGB8:
      return 24;
    case AV_PIX_FMT_RGB4:
      return 25;
    case AV_PIX_FMT_RGB4_BYTE:
      return 26;
    case AV_PIX_FMT_NV12:
      return 27;
    case AV_PIX_FMT_NV21:
      return 28;
    case AV_PIX_FMT_ARGB: // Big Endian
      return 29;
    case AV_PIX_FMT_RGBA: // Big
      return 30;
    case AV_PIX_FMT_ABGR: // Big
      return 31;
    case AV_PIX_FMT_BGRA: // little
      return 32;
    case AV_PIX_FMT_GRAY16BE: // big
      return 33;
    case AV_PIX_FMT_GRAY16LE:
      return 34;
    case AV_PIX_FMT_YUV440P:
      return 35;
    case AV_PIX_FMT_YUVJ440P:
      return 36;
    case AV_PIX_FMT_YUVA420P:
      return 37;
      //       case AV_PIX_FMT_VDPAU_H264 :
      //     return 38;
      //       case AV_PIX_FMT_VDPAU_MPEG1 :
      //     return 39;
      //       case AV_PIX_FMT_VDPAU_MPEG2 :
      //     return 40;
      //       case AV_PIX_FMT_VDPAU_WMV3 :    // Conditional compile.
      //     return 41;
      //       case AV_PIX_FMT_VDPAU_VC1 :     // ff_api_vdpau is present
      //     return 42;
    case AV_PIX_FMT_RGB48BE:
      return 43;
    case AV_PIX_FMT_RGB48LE:
      return 44;
    case AV_PIX_FMT_RGB565BE:
      return 45;
    case AV_PIX_FMT_RGB565LE:
      return 46;
    case AV_PIX_FMT_RGB555BE:
      return 47;
    case AV_PIX_FMT_RGB555LE:
      return 48;
    case AV_PIX_FMT_BGR565BE:
      return 49;
    case AV_PIX_FMT_BGR565LE:
      return 50;
    case AV_PIX_FMT_BGR555BE:
      return 51;
    case AV_PIX_FMT_BGR555LE:
      return 52;
      //       case AV_PIX_FMT_VAAPI_MOCO :
      //     return 53;
      //       case AV_PIX_FMT_VAAPI_IDCT :
      //     return 54;
      //       case AV_PIX_FMT_VAAPI_VLD :
      //     return 55;
      //       case AV_PIX_FMT_VAAPI :    // ff_api_vdpau is present
      //     return 56;
    case AV_PIX_FMT_YUV420P16LE:
      return 57;
    case AV_PIX_FMT_YUV420P16BE:
      return 58;
    case AV_PIX_FMT_YUV422P16LE:
      return 59;
    case AV_PIX_FMT_YUV422P16BE:
      return 60;
    case AV_PIX_FMT_YUV444P16LE:
      return 61;
    case AV_PIX_FMT_YUV444P16BE:
      return 62;
      //       case AV_PIX_FMT_VDPAU_MPEG4 :   // ff_api_vdpau is present
      //     return 63;
    case AV_PIX_FMT_DXVA2_VLD:
      return 64;
    case AV_PIX_FMT_RGB444LE:
      return 65;
    case AV_PIX_FMT_RGB444BE:
      return 66;
    case AV_PIX_FMT_BGR444LE:
      return 67;
    case AV_PIX_FMT_BGR444BE:
      return 68;
    case AV_PIX_FMT_YA8:
      return 69;
    case AV_PIX_FMT_BGR48BE:
      return 70;
    case AV_PIX_FMT_BGR48LE:
      return 71;
    case AV_PIX_FMT_YUV420P9BE:
      return 72;
    case AV_PIX_FMT_YUV420P9LE:
      return 73;
    case AV_PIX_FMT_YUV420P10BE:
      return 74;
    case AV_PIX_FMT_YUV420P10LE:
      return 75;
    case AV_PIX_FMT_YUV422P10BE:
      return 76;
    case AV_PIX_FMT_YUV422P10LE:
      return 77;
    case AV_PIX_FMT_YUV444P9BE:
      return 78;
    case AV_PIX_FMT_YUV444P9LE:
      return 79;
    case AV_PIX_FMT_YUV444P10BE:
      return 80;
    case AV_PIX_FMT_YUV444P10LE:
      return 81;
    case AV_PIX_FMT_YUV422P9BE:
      return 82;
    case AV_PIX_FMT_YUV422P9LE:
      return 83;
      //       case AV_PIX_FMT_VDA_VLD :   // Lower than ffmpeg version 4
      //     return 84;
    case AV_PIX_FMT_GBRP:
      return 85;
    case AV_PIX_FMT_GBRP9BE:
      return 86;
    case AV_PIX_FMT_GBRP9LE:
      return 87;
    case AV_PIX_FMT_GBRP10BE:
      return 88;
    case AV_PIX_FMT_GBRP10LE:
      return 89;
    case AV_PIX_FMT_GBRP16BE:
      return 90;
    case AV_PIX_FMT_GBRP16LE:
      return 91;
    case AV_PIX_FMT_YUVA420P9BE:
      return 92;
    case AV_PIX_FMT_YUVA420P9LE:
      return 93;
    case AV_PIX_FMT_YUVA422P9BE:
      return 94;
    case AV_PIX_FMT_YUVA422P9LE:
      return 95;
    case AV_PIX_FMT_YUVA444P9BE:
      return 96;
    case AV_PIX_FMT_YUVA444P9LE:
      return 97;
    case AV_PIX_FMT_YUVA420P10BE:
      return 98;
    case AV_PIX_FMT_YUVA420P10LE:
      return 99;
    case AV_PIX_FMT_YUVA422P10BE:
      return 100;
    case AV_PIX_FMT_YUVA422P10LE:
      return 101;
    case AV_PIX_FMT_YUVA444P10BE:
      return 102;
    case AV_PIX_FMT_YUVA444P10LE:
      return 103;
    case AV_PIX_FMT_YUVA420P16BE:
      return 104;
    case AV_PIX_FMT_YUVA420P16LE:
      return 105;
    case AV_PIX_FMT_YUVA422P16BE:
      return 106;
    case AV_PIX_FMT_YUVA422P16LE:
      return 107;
    case AV_PIX_FMT_YUVA444P16BE:
      return 108;
    case AV_PIX_FMT_YUVA444P16LE:
      return 109;
    case AV_PIX_FMT_VDPAU:
      return 110;
    case AV_PIX_FMT_XYZ12LE:
      return 111;
    case AV_PIX_FMT_XYZ12BE:
      return 112;
    case AV_PIX_FMT_NV16:
      return 113;
    case AV_PIX_FMT_NV20LE:
      return 114;
    case AV_PIX_FMT_NV20BE:
      return 115;
    case AV_PIX_FMT_RGBA64BE:
      return 116;
    case AV_PIX_FMT_RGBA64LE:
      return 117;
    case AV_PIX_FMT_BGRA64BE:
      return 118;
    case AV_PIX_FMT_BGRA64LE:
      return 119;
    case AV_PIX_FMT_YVYU422:
      return 120;
      //       case AV_PIX_FMT_VDA :   // Lower than ffmpeg version 4.
      //     return 121;
    case AV_PIX_FMT_YA16BE: // big
      return 122;
    case AV_PIX_FMT_YA16LE:
      return 123;
    case AV_PIX_FMT_QSV:
      return 124;
    case AV_PIX_FMT_MMAL:
      return 125;
    case AV_PIX_FMT_D3D11VA_VLD:
      return 126;
    case AV_PIX_FMT_CUDA:
      return 127;
    case AV_PIX_FMT_0RGB: // big
      return 128;
    case AV_PIX_FMT_RGB0:
      return 129;
    case AV_PIX_FMT_0BGR: // big
      return 130;
    case AV_PIX_FMT_BGR0:
      return 131;
    case AV_PIX_FMT_YUVA444P:
      return 132;
    case AV_PIX_FMT_YUVA422P:
      return 133;
    case AV_PIX_FMT_YUV420P12BE:
      return 134;
    case AV_PIX_FMT_YUV420P12LE:
      return 135;
    case AV_PIX_FMT_YUV420P14BE:
      return 136;
    case AV_PIX_FMT_YUV420P14LE:
      return 137;
    case AV_PIX_FMT_YUV422P12BE:
      return 138;
    case AV_PIX_FMT_YUV422P12LE:
      return 139;
    case AV_PIX_FMT_YUV422P14BE:
      return 140;
    case AV_PIX_FMT_YUV422P14LE:
      return 141;
    case AV_PIX_FMT_YUV444P12BE:
      return 142;
    case AV_PIX_FMT_YUV444P12LE:
      return 143;
    case AV_PIX_FMT_YUV444P14BE:
      return 144;
    case AV_PIX_FMT_YUV444P14LE:
      return 146;
    case AV_PIX_FMT_GBRP12BE:
      return 147;
    case AV_PIX_FMT_GBRP12LE:
      return 148;
    case AV_PIX_FMT_GBRP14BE:
      return 149;
    case AV_PIX_FMT_GBRP14LE:
      return 150;
    case AV_PIX_FMT_GBRAP:
      return 151;
    case AV_PIX_FMT_GBRAP16BE:
      return 152;
    case AV_PIX_FMT_GBRAP16LE:
      return 153;
    case AV_PIX_FMT_YUVJ411P:
      return 154;
    case AV_PIX_FMT_BAYER_BGGR8:
      return 155;
    case AV_PIX_FMT_BAYER_RGGB8:
      return 156;
    case AV_PIX_FMT_BAYER_GBRG8:
      return 157;
    case AV_PIX_FMT_BAYER_GRBG8:
      return 158;
    case AV_PIX_FMT_BAYER_BGGR16LE:
      return 159;
    case AV_PIX_FMT_BAYER_BGGR16BE:
      return 160;
    case AV_PIX_FMT_BAYER_RGGB16LE:
      return 161;
    case AV_PIX_FMT_BAYER_RGGB16BE:
      return 162;
    case AV_PIX_FMT_BAYER_GBRG16LE:
      return 163;
    case AV_PIX_FMT_BAYER_GBRG16BE:
      return 164;
    case AV_PIX_FMT_BAYER_GRBG16LE:
      return 165;
    case AV_PIX_FMT_BAYER_GRBG16BE:
      return 166;
    case AV_PIX_FMT_YUV440P10LE:
      return 167;
    case AV_PIX_FMT_YUV440P10BE:
      return 168;
    case AV_PIX_FMT_YUV440P12LE:
      return 169;
    case AV_PIX_FMT_YUV440P12BE:
      return 170;
    case AV_PIX_FMT_AYUV64LE:
      return 171;
    case AV_PIX_FMT_AYUV64BE:
      return 172;
    case AV_PIX_FMT_VIDEOTOOLBOX:
      return 173;
    case AV_PIX_FMT_XVMC:
      return 174;
      //        case AV_PIX_FMT_RGB32:  // IF format is this type, based on
      //        endianness, it resolves to big endian or small endian.
      //          return 175;           // The Switch case contains both big and
      //          small endian, so No need to add these in switch case.
      //        case AV_PIX_FMT_RGB32_1:  // Will Automatically resolve.
      //          return 176;
      //        case AV_PIX_FMT_BGR32:
      //          return 177;
      //        case AV_PIX_FMT_BGR32_1:
      //          return 178;
      //        case AV_PIX_FMT_0RGB32:
      //          return 179;
      //        case AV_PIX_FMT_0BGR32:
      //          return 180;
      //        case AV_PIX_FMT_GRAY16:
      //          return 181;
      //        case AV_PIX_FMT_YA16:
      //          return 182;
      //        case AV_PIX_FMT_RGB48:
      //          return 183;
      //        case AV_PIX_FMT_RGB565:
      //          return 184;
      //        case AV_PIX_FMT_RGB444:
      //          return 185;
      //        case AV_PIX_FMT_BGR48:
      //          return 186;
      //        case AV_PIX_FMT_BGR565:
      //          return 187;
      //        case AV_PIX_FMT_BGR555:
      //          return 188;
      //        case AV_PIX_FMT_BGR444:
      //          return 189;
      //        case AV_PIX_FMT_YUV420P9:
      //          return 190;
      //        case AV_PIX_FMT_YUV422P9:
      //          return 191;
      //        case AV_PIX_FMT_YUV444P9:
      //          return 192;
      //        case AV_PIX_FMT_YUV420P10:
      //          return 193;
      //        case AV_PIX_FMT_YUV422P10:
      //          return 194;
      //        case AV_PIX_FMT_YUV440P10:
      //          return 195;
      //        case AV_PIX_FMT_YUV444P10:
      //          return 196;
      //        case AV_PIX_FMT_YUV420P12:
      //          return 197;
      //        case AV_PIX_FMT_YUV422P12:
      //          return 198;
      //        case AV_PIX_FMT_YUV440P12:
      //          return 199;
      //        case AV_PIX_FMT_YUV444P12:
      //          return 200;
      //        case AV_PIX_FMT_YUV420P14:
      //          return 201;
      //        case AV_PIX_FMT_YUV422P14:
      //          return 202;
      //        case AV_PIX_FMT_YUV444P14:
      //          return 203;
      //        case AV_PIX_FMT_YUV420P16:
      //          return 204;
      //        case AV_PIX_FMT_YUV422P16:
      //          return 205;
      //        case AV_PIX_FMT_YUV444P16:
      //          return 206;
      //        case AV_PIX_FMT_GBRP9:
      //          return 207;
      //        case AV_PIX_FMT_GBRP10:
      //          return 208;
      //        case AV_PIX_FMT_GBRP12:
      //          return 209;
      //        case AV_PIX_FMT_GBRP14:
      //          return 210;
      //        case AV_PIX_FMT_GBRP16:
      //          return 211;
      //        case AV_PIX_FMT_GBRAP16:
      //          return 212;
      //        case AV_PIX_FMT_BAYER_BGGR16:
      //          return 213;
      //        case AV_PIX_FMT_BAYER_RGGB16:
      //          return 214;
      //        case AV_PIX_FMT_BAYER_GBRG16:
      //          return 215;
      //        case AV_PIX_FMT_BAYER_GRBG16:
      //          return 216;
      //        case AV_PIX_FMT_YUVA420P9:
      //          return 217;
      //        case AV_PIX_FMT_YUVA422P9:
      //          return 218;
      //        case AV_PIX_FMT_YUVA444P9:
      //          return 219;
      //        case AV_PIX_FMT_YUVA420P10:
      //          return 220;
      //        case AV_PIX_FMT_YUVA422P10:
      //          return 221;
      //        case AV_PIX_FMT_YUVA444P10:
      //          return 222;
      //        case AV_PIX_FMT_YUVA420P16:
      //          return 223;
      //        case AV_PIX_FMT_YUVA422P16:
      //          return 224;
      //        case AV_PIX_FMT_YUVA444P16:
      //          return 225;
      //        case AV_PIX_FMT_XYZ12:
      //          return 226;
      //        case AV_PIX_FMT_NV20:
      //          return 227;
      //        case AV_PIX_FMT_AYUV64:
      //          return 228;
    case AV_PIX_FMT_P010LE:
      return 229;
    case AV_PIX_FMT_P010BE:
      return 230;
    case AV_PIX_FMT_GBRAP12BE:
      return 231;
    case AV_PIX_FMT_GBRAP12LE:
      return 232;
    case AV_PIX_FMT_GBRAP10LE:
      return 233;
    case AV_PIX_FMT_GBRAP10BE:
      return 234;
    case AV_PIX_FMT_MEDIACODEC:
      return 235;
    case AV_PIX_FMT_GRAY12BE:
      return 236;
    case AV_PIX_FMT_GRAY12LE:
      return 237;
    case AV_PIX_FMT_GRAY10BE:
      return 238;
    case AV_PIX_FMT_GRAY10LE:
      return 239;
    case AV_PIX_FMT_P016LE:
      return 240;
    case AV_PIX_FMT_P016BE:
      return 241;
    case AV_PIX_FMT_D3D11:
      return 242;
    case AV_PIX_FMT_GRAY9BE:
      return 243;
    case AV_PIX_FMT_GRAY9LE:
      return 244;
    case AV_PIX_FMT_GBRPF32BE:
      return 245;
    case AV_PIX_FMT_GBRPF32LE:
      return 246;
    case AV_PIX_FMT_GBRAPF32BE:
      return 247;
    case AV_PIX_FMT_GBRAPF32LE:
      return 248;
    case AV_PIX_FMT_DRM_PRIME:
      return 249;
    // Above ffmpeg 4.0  Need to add versions.
    case AV_PIX_FMT_OPENCL:
      return 250;
    case AV_PIX_FMT_GRAY14BE:
      return 251;
    case AV_PIX_FMT_GRAY14LE:
      return 252;
    case AV_PIX_FMT_GRAYF32BE:
      return 253;
    case AV_PIX_FMT_GRAYF32LE:
      return 254;
    case AV_PIX_FMT_YUVA422P12BE:
      return 255;
    case AV_PIX_FMT_YUVA422P12LE:
      return 256;
    case AV_PIX_FMT_YUVA444P12BE:
      return 257;
    case AV_PIX_FMT_YUVA444P12LE:
      return 258;
    case AV_PIX_FMT_NV24:
      return 259;
    case AV_PIX_FMT_NV42:
      return 260;
    case AV_PIX_FMT_VULKAN:
      return 261;
    case AV_PIX_FMT_Y210BE:
      return 262;
    case AV_PIX_FMT_Y210LE:
      return 263;
    case AV_PIX_FMT_X2RGB10LE:
      return 264;
    case AV_PIX_FMT_X2RGB10BE:
      return 265;
    case AV_PIX_FMT_X2BGR10LE:
      return 266;
    case AV_PIX_FMT_X2BGR10BE:
      return 267;
    case AV_PIX_FMT_P210BE:
      return 268;
    case AV_PIX_FMT_P210LE:
      return 269;
    case AV_PIX_FMT_P410BE:
      return 270;
    case AV_PIX_FMT_P410LE:
      return 271;
    case AV_PIX_FMT_P216BE:
      return 272;
    case AV_PIX_FMT_P216LE:
      return 273;
    case AV_PIX_FMT_P416BE:
      return 274;
    case AV_PIX_FMT_P416LE:
      return 275;
    case AV_PIX_FMT_VUYA:
      return 276;
    case AV_PIX_FMT_RGBAF16BE:
      return 277;
    case AV_PIX_FMT_RGBAF16LE:
      return 278;
    case AV_PIX_FMT_VUYX:
      return 279;
    case AV_PIX_FMT_P012LE:
      return 280;
    case AV_PIX_FMT_P012BE:
      return 281;
    case AV_PIX_FMT_Y212BE:
      return 282;
    case AV_PIX_FMT_Y212LE:
      return 283;
    case AV_PIX_FMT_XV30BE:
      return 284;
    case AV_PIX_FMT_XV30LE:
      return 285;
    case AV_PIX_FMT_XV36BE:
      return 286;
    case AV_PIX_FMT_XV36LE:
      return 287;
    case AV_PIX_FMT_RGBF32BE:
      return 288;
    case AV_PIX_FMT_RGBF32LE:
      return 289;
    case AV_PIX_FMT_RGBAF32BE:
      return 290;
    case AV_PIX_FMT_RGBAF32LE:
      return 291;
      //       case AV_PIX_FMT_RPI :
      //     return 292;
      //       case AV_PIX_FMT_SAND128 :
      //     return 293;
      //       case AV_PIX_FMT_SAND64_10 :
      //     return 294;
      //       case AV_PIX_FMT_SAND64_16 :
      //     return 295;
      //       case AV_PIX_FMT_RPI4_8 :       // rpi turn on then only
      //     return 296;
      //      case AV_PIX_FMT_RPI4_10 :
      //     return 297;
      //        case AV_PIX_FMT_RGB555:      // Little Endian, Big Endian WIll
      //        Resolve on it's own.
      //          return 298;
    default:
      return 0;
    }
  }

  static AVPixelFormat intoAVPixFmt(uint32_t AvPixFmtId) {
    switch (AvPixFmtId) {
    case 0:
      return AV_PIX_FMT_NONE;
    case 1:
      return AV_PIX_FMT_YUV420P;
    case 2:
      return AV_PIX_FMT_YUYV422;
    case 3:
      return AV_PIX_FMT_RGB24;
    case 4:
      return AV_PIX_FMT_BGR24;
    case 5:
      return AV_PIX_FMT_YUV422P;
    case 7:
      return AV_PIX_FMT_YUV444P;
    case 8:
      return AV_PIX_FMT_YUV410P;
    case 9:
      return AV_PIX_FMT_YUV411P;
    case 10:
      return AV_PIX_FMT_GRAY8;
    case 11:
      return AV_PIX_FMT_MONOWHITE;
    case 12:
      return AV_PIX_FMT_MONOBLACK;
    case 13:
      return AV_PIX_FMT_PAL8;
    case 14:
      return AV_PIX_FMT_YUVJ420P;
    case 15:
      return AV_PIX_FMT_YUVJ422P;
    case 16:
      return AV_PIX_FMT_YUVJ444P;
      //     case 17:
      //       return AV_PIX_FMT_XVMC_MPEG2_MC ;     // Lower FFmpeg Version
      //     case 18:
      //       return AV_PIX_FMT_XVMC_MPEG2_IDCT ;
    case 19:
      return AV_PIX_FMT_UYVY422;
    case 20:
      return AV_PIX_FMT_UYYVYY411;
    case 21:
      return AV_PIX_FMT_BGR8;
    case 22:
      return AV_PIX_FMT_BGR4;
    case 23:
      return AV_PIX_FMT_BGR4_BYTE;
    case 24:
      return AV_PIX_FMT_RGB8;
    case 25:
      return AV_PIX_FMT_RGB4;
    case 26:
      return AV_PIX_FMT_RGB4_BYTE;
    case 27:
      return AV_PIX_FMT_NV12;
    case 28:
      return AV_PIX_FMT_NV21;
    case 29:
      return AV_PIX_FMT_ARGB;
    case 30:
      return AV_PIX_FMT_RGBA;
    case 31:
      return AV_PIX_FMT_ABGR;
    case 32:
      return AV_PIX_FMT_BGRA; // Little
    case 33:
      return AV_PIX_FMT_GRAY16BE;
    case 34:
      return AV_PIX_FMT_GRAY16LE;
    case 35:
      return AV_PIX_FMT_YUV440P;
    case 36:
      return AV_PIX_FMT_YUVJ440P;
    case 37:
      return AV_PIX_FMT_YUVA420P;
      //     case 38:
      //       return AV_PIX_FMT_VDPAU_H264 ;
      //     case 39:
      //       return AV_PIX_FMT_VDPAU_MPEG1 ;
      //     case 40:
      //       return AV_PIX_FMT_VDPAU_MPEG2 ;
      //     case 41:
      //       return AV_PIX_FMT_VDPAU_WMV3 ;    // Conditional compile.
      //     case 42:
      //       return AV_PIX_FMT_VDPAU_VC1 ;     // ff_api_vdpau is present
    case 43:
      return AV_PIX_FMT_RGB48BE;
    case 44:
      return AV_PIX_FMT_RGB48LE;
    case 45:
      return AV_PIX_FMT_RGB565BE;
    case 46:
      return AV_PIX_FMT_RGB565LE;
    case 47:
      return AV_PIX_FMT_RGB555BE;
    case 48:
      return AV_PIX_FMT_RGB555LE;
    case 49:
      return AV_PIX_FMT_BGR565BE;
    case 50:
      return AV_PIX_FMT_BGR565LE;
    case 51:
      return AV_PIX_FMT_BGR555BE;
    case 52:
      return AV_PIX_FMT_BGR555LE;
      //     case 53:
      //       return AV_PIX_FMT_VAAPI_MOCO ;
      //     case 54:
      //       return AV_PIX_FMT_VAAPI_IDCT ;
      //     case 55:
      //       return AV_PIX_FMT_VAAPI_VLD ;
      //     case 56:
      //       return AV_PIX_FMT_VAAPI ;    // ff_api_vdpau is present
    case 57:
      return AV_PIX_FMT_YUV420P16LE;
    case 58:
      return AV_PIX_FMT_YUV420P16BE;
    case 59:
      return AV_PIX_FMT_YUV422P16LE;
    case 60:
      return AV_PIX_FMT_YUV422P16BE;
    case 61:
      return AV_PIX_FMT_YUV444P16LE;
    case 62:
      return AV_PIX_FMT_YUV444P16BE;
      //     case 63:
      //       return AV_PIX_FMT_VDPAU_MPEG4 ;   // ff_api_vdpau is present
    case 64:
      return AV_PIX_FMT_DXVA2_VLD;
    case 65:
      return AV_PIX_FMT_RGB444LE;
    case 66:
      return AV_PIX_FMT_RGB444BE;
    case 67:
      return AV_PIX_FMT_BGR444LE;
    case 68:
      return AV_PIX_FMT_BGR444BE;
    case 69:
      return AV_PIX_FMT_YA8;
    case 70:
      return AV_PIX_FMT_BGR48BE;
    case 71:
      return AV_PIX_FMT_BGR48LE;
    case 72:
      return AV_PIX_FMT_YUV420P9BE;
    case 73:
      return AV_PIX_FMT_YUV420P9LE;
    case 74:
      return AV_PIX_FMT_YUV420P10BE;
    case 75:
      return AV_PIX_FMT_YUV420P10LE;
    case 76:
      return AV_PIX_FMT_YUV422P10BE;
    case 77:
      return AV_PIX_FMT_YUV422P10LE;
    case 78:
      return AV_PIX_FMT_YUV444P9BE;
    case 79:
      return AV_PIX_FMT_YUV444P9LE;
    case 80:
      return AV_PIX_FMT_YUV444P10BE;
    case 81:
      return AV_PIX_FMT_YUV444P10LE;
    case 82:
      return AV_PIX_FMT_YUV422P9BE;
    case 83:
      return AV_PIX_FMT_YUV422P9LE;
      //     case 84:
      //       return AV_PIX_FMT_VDA_VLD ;   // Lower than ffmpeg version 4
    case 85:
      return AV_PIX_FMT_GBRP;
    case 86:
      return AV_PIX_FMT_GBRP9BE;
    case 87:
      return AV_PIX_FMT_GBRP9LE;
    case 88:
      return AV_PIX_FMT_GBRP10BE;
    case 89:
      return AV_PIX_FMT_GBRP10LE;
    case 90:
      return AV_PIX_FMT_GBRP16BE;
    case 91:
      return AV_PIX_FMT_GBRP16LE;
    case 92:
      return AV_PIX_FMT_YUVA420P9BE;
    case 93:
      return AV_PIX_FMT_YUVA420P9LE;
    case 94:
      return AV_PIX_FMT_YUVA422P9BE;
    case 95:
      return AV_PIX_FMT_YUVA422P9LE;
    case 96:
      return AV_PIX_FMT_YUVA444P9BE;
    case 97:
      return AV_PIX_FMT_YUVA444P9LE;
    case 98:
      return AV_PIX_FMT_YUVA420P10BE;
    case 99:
      return AV_PIX_FMT_YUVA420P10LE;
    case 100:
      return AV_PIX_FMT_YUVA422P10BE;
    case 101:
      return AV_PIX_FMT_YUVA422P10LE;
    case 102:
      return AV_PIX_FMT_YUVA444P10BE;
    case 103:
      return AV_PIX_FMT_YUVA444P10LE;
    case 104:
      return AV_PIX_FMT_YUVA420P16BE;
    case 105:
      return AV_PIX_FMT_YUVA420P16LE;
    case 106:
      return AV_PIX_FMT_YUVA422P16BE;
    case 107:
      return AV_PIX_FMT_YUVA422P16LE;
    case 108:
      return AV_PIX_FMT_YUVA444P16BE;
    case 109:
      return AV_PIX_FMT_YUVA444P16LE;
    case 110:
      return AV_PIX_FMT_VDPAU;
    case 111:
      return AV_PIX_FMT_XYZ12LE;
    case 112:
      return AV_PIX_FMT_XYZ12BE;
    case 113:
      return AV_PIX_FMT_NV16;
    case 114:
      return AV_PIX_FMT_NV20LE;
    case 115:
      return AV_PIX_FMT_NV20BE;
    case 116:
      return AV_PIX_FMT_RGBA64BE;
    case 117:
      return AV_PIX_FMT_RGBA64LE;
    case 118:
      return AV_PIX_FMT_BGRA64BE;
    case 119:
      return AV_PIX_FMT_BGRA64LE;
    case 120:
      return AV_PIX_FMT_YVYU422;
      //     case 121:
      //       return AV_PIX_FMT_VDA ;   // Lower than ffmpeg version 4.
    case 122:
      return AV_PIX_FMT_YA16BE;
    case 123:
      return AV_PIX_FMT_YA16LE;
    case 124:
      return AV_PIX_FMT_QSV;
    case 125:
      return AV_PIX_FMT_MMAL;
    case 126:
      return AV_PIX_FMT_D3D11VA_VLD;
    case 127:
      return AV_PIX_FMT_CUDA;
    case 128:
      return AV_PIX_FMT_0RGB;
    case 129:
      return AV_PIX_FMT_RGB0;
    case 130:
      return AV_PIX_FMT_0BGR;
    case 131:
      return AV_PIX_FMT_BGR0;
    case 132:
      return AV_PIX_FMT_YUVA444P;
    case 133:
      return AV_PIX_FMT_YUVA422P;
    case 134:
      return AV_PIX_FMT_YUV420P12BE;
    case 135:
      return AV_PIX_FMT_YUV420P12LE;
    case 136:
      return AV_PIX_FMT_YUV420P14BE;
    case 137:
      return AV_PIX_FMT_YUV420P14LE;
    case 138:
      return AV_PIX_FMT_YUV422P12BE;
    case 139:
      return AV_PIX_FMT_YUV422P12LE;
    case 140:
      return AV_PIX_FMT_YUV422P14BE;
    case 141:
      return AV_PIX_FMT_YUV422P14LE;
    case 142:
      return AV_PIX_FMT_YUV444P12BE;
    case 143:
      return AV_PIX_FMT_YUV444P12LE;
    case 144:
      return AV_PIX_FMT_YUV444P14BE;
    case 146:
      return AV_PIX_FMT_YUV444P14LE;
    case 147:
      return AV_PIX_FMT_GBRP12BE;
    case 148:
      return AV_PIX_FMT_GBRP12LE;
    case 149:
      return AV_PIX_FMT_GBRP14BE;
    case 150:
      return AV_PIX_FMT_GBRP14LE;
    case 151:
      return AV_PIX_FMT_GBRAP;
    case 152:
      return AV_PIX_FMT_GBRAP16BE;
    case 153:
      return AV_PIX_FMT_GBRAP16LE;
    case 154:
      return AV_PIX_FMT_YUVJ411P;
    case 155:
      return AV_PIX_FMT_BAYER_BGGR8;
    case 156:
      return AV_PIX_FMT_BAYER_RGGB8;
    case 157:
      return AV_PIX_FMT_BAYER_GBRG8;
    case 158:
      return AV_PIX_FMT_BAYER_GRBG8;
    case 159:
      return AV_PIX_FMT_BAYER_BGGR16LE;
    case 160:
      return AV_PIX_FMT_BAYER_BGGR16BE;
    case 161:
      return AV_PIX_FMT_BAYER_RGGB16LE;
    case 162:
      return AV_PIX_FMT_BAYER_RGGB16BE;
    case 163:
      return AV_PIX_FMT_BAYER_GBRG16LE;
    case 164:
      return AV_PIX_FMT_BAYER_GBRG16BE;
    case 165:
      return AV_PIX_FMT_BAYER_GRBG16LE;
    case 166:
      return AV_PIX_FMT_BAYER_GRBG16BE;
    case 167:
      return AV_PIX_FMT_YUV440P10LE;
    case 168:
      return AV_PIX_FMT_YUV440P10BE;
    case 169:
      return AV_PIX_FMT_YUV440P12LE;
    case 170:
      return AV_PIX_FMT_YUV440P12BE;
    case 171:
      return AV_PIX_FMT_AYUV64LE;
    case 172:
      return AV_PIX_FMT_AYUV64BE;
    case 173:
      return AV_PIX_FMT_VIDEOTOOLBOX;
    case 174:
      return AV_PIX_FMT_XVMC;
    case 175:
      return AV_PIX_FMT_RGB32;
    case 176:
      return AV_PIX_FMT_RGB32_1;
    case 177:
      return AV_PIX_FMT_BGR32;
    case 178:
      return AV_PIX_FMT_BGR32_1;
    case 179:
      return AV_PIX_FMT_0RGB32;
    case 180:
      return AV_PIX_FMT_0BGR32;
    case 181:
      return AV_PIX_FMT_GRAY16;
    case 182:
      return AV_PIX_FMT_YA16;
    case 183:
      return AV_PIX_FMT_RGB48;
    case 184:
      return AV_PIX_FMT_RGB565;
    case 185:
      return AV_PIX_FMT_RGB444;
    case 186:
      return AV_PIX_FMT_BGR48;
    case 187:
      return AV_PIX_FMT_BGR565;
    case 188:
      return AV_PIX_FMT_BGR555;
    case 189:
      return AV_PIX_FMT_BGR444;
    case 190:
      return AV_PIX_FMT_YUV420P9;
    case 191:
      return AV_PIX_FMT_YUV422P9;
    case 192:
      return AV_PIX_FMT_YUV444P9;
    case 193:
      return AV_PIX_FMT_YUV420P10;
    case 194:
      return AV_PIX_FMT_YUV422P10;
    case 195:
      return AV_PIX_FMT_YUV440P10;
    case 196:
      return AV_PIX_FMT_YUV444P10;
    case 197:
      return AV_PIX_FMT_YUV420P12;
    case 198:
      return AV_PIX_FMT_YUV422P12;
    case 199:
      return AV_PIX_FMT_YUV440P12;
    case 200:
      return AV_PIX_FMT_YUV444P12;
    case 201:
      return AV_PIX_FMT_YUV420P14;
    case 202:
      return AV_PIX_FMT_YUV422P14;
    case 203:
      return AV_PIX_FMT_YUV444P14;
    case 204:
      return AV_PIX_FMT_YUV420P16;
    case 205:
      return AV_PIX_FMT_YUV422P16;
    case 206:
      return AV_PIX_FMT_YUV444P16;
    case 207:
      return AV_PIX_FMT_GBRP9;
    case 208:
      return AV_PIX_FMT_GBRP10;
    case 209:
      return AV_PIX_FMT_GBRP12;
    case 210:
      return AV_PIX_FMT_GBRP14;
    case 211:
      return AV_PIX_FMT_GBRP16;
    case 212:
      return AV_PIX_FMT_GBRAP16;
    case 213:
      return AV_PIX_FMT_BAYER_BGGR16;
    case 214:
      return AV_PIX_FMT_BAYER_RGGB16;
    case 215:
      return AV_PIX_FMT_BAYER_GBRG16;
    case 216:
      return AV_PIX_FMT_BAYER_GRBG16;
    case 217:
      return AV_PIX_FMT_YUVA420P9;
    case 218:
      return AV_PIX_FMT_YUVA422P9;
    case 219:
      return AV_PIX_FMT_YUVA444P9;
    case 220:
      return AV_PIX_FMT_YUVA420P10;
    case 221:
      return AV_PIX_FMT_YUVA422P10;
    case 222:
      return AV_PIX_FMT_YUVA444P10;
    case 223:
      return AV_PIX_FMT_YUVA420P16;
    case 224:
      return AV_PIX_FMT_YUVA422P16;
    case 225:
      return AV_PIX_FMT_YUVA444P16;
    case 226:
      return AV_PIX_FMT_XYZ12;
    case 227:
      return AV_PIX_FMT_NV20;
    case 228:
      return AV_PIX_FMT_AYUV64;
    case 229:
      return AV_PIX_FMT_P010LE;
    case 230:
      return AV_PIX_FMT_P010BE;
    case 231:
      return AV_PIX_FMT_GBRAP12BE;
    case 232:
      return AV_PIX_FMT_GBRAP12LE;
    case 233:
      return AV_PIX_FMT_GBRAP10LE;
    case 234:
      return AV_PIX_FMT_GBRAP10BE;
    case 235:
      return AV_PIX_FMT_MEDIACODEC;
    case 236:
      return AV_PIX_FMT_GRAY12BE;
    case 237:
      return AV_PIX_FMT_GRAY12LE;
    case 238:
      return AV_PIX_FMT_GRAY10BE;
    case 239:
      return AV_PIX_FMT_GRAY10LE;
    case 240:
      return AV_PIX_FMT_P016LE;
    case 241:
      return AV_PIX_FMT_P016BE;
    case 242:
      return AV_PIX_FMT_D3D11;
    case 243:
      return AV_PIX_FMT_GRAY9BE;
    case 244:
      return AV_PIX_FMT_GRAY9LE;
    case 245:
      return AV_PIX_FMT_GBRPF32BE;
    case 246:
      return AV_PIX_FMT_GBRPF32LE;
    case 247:
      return AV_PIX_FMT_GBRAPF32BE;
    case 248:
      return AV_PIX_FMT_GBRAPF32LE;
    case 249:
      return AV_PIX_FMT_DRM_PRIME;

      // Above ffmpeg 4.0  Need to add versions.
    case 250:
      return AV_PIX_FMT_OPENCL;
    case 251:
      return AV_PIX_FMT_GRAY14BE;
    case 252:
      return AV_PIX_FMT_GRAY14LE;
    case 253:
      return AV_PIX_FMT_GRAYF32BE;
    case 254:
      return AV_PIX_FMT_GRAYF32LE;
    case 255:
      return AV_PIX_FMT_YUVA422P12BE;
    case 256:
      return AV_PIX_FMT_YUVA422P12LE;
    case 257:
      return AV_PIX_FMT_YUVA444P12BE;
    case 258:
      return AV_PIX_FMT_YUVA444P12LE;
    case 259:
      return AV_PIX_FMT_NV24;
    case 260:
      return AV_PIX_FMT_NV42;
    case 261:
      return AV_PIX_FMT_VULKAN;
    case 262:
      return AV_PIX_FMT_Y210BE;
    case 263:
      return AV_PIX_FMT_Y210LE;
    case 264:
      return AV_PIX_FMT_X2RGB10LE;
    case 265:
      return AV_PIX_FMT_X2RGB10BE;
    case 266:
      return AV_PIX_FMT_X2BGR10LE;
    case 267:
      return AV_PIX_FMT_X2BGR10BE;
    case 268:
      return AV_PIX_FMT_P210BE;
    case 269:
      return AV_PIX_FMT_P210LE;
    case 270:
      return AV_PIX_FMT_P410BE;
    case 271:
      return AV_PIX_FMT_P410LE;
    case 272:
      return AV_PIX_FMT_P216BE;
    case 273:
      return AV_PIX_FMT_P216LE;
    case 274:
      return AV_PIX_FMT_P416BE;
    case 275:
      return AV_PIX_FMT_P416LE;
    case 276:
      return AV_PIX_FMT_VUYA;
    case 277:
      return AV_PIX_FMT_RGBAF16BE;
    case 278:
      return AV_PIX_FMT_RGBAF16LE;
    case 279:
      return AV_PIX_FMT_VUYX;
    case 280:
      return AV_PIX_FMT_P012LE;
    case 281:
      return AV_PIX_FMT_P012BE;
    case 282:
      return AV_PIX_FMT_Y212BE;
    case 283:
      return AV_PIX_FMT_Y212LE;
    case 284:
      return AV_PIX_FMT_XV30BE;
    case 285:
      return AV_PIX_FMT_XV30LE;
    case 286:
      return AV_PIX_FMT_XV36BE;
    case 287:
      return AV_PIX_FMT_XV36LE;
    case 288:
      return AV_PIX_FMT_RGBF32BE;
    case 289:
      return AV_PIX_FMT_RGBF32LE;
    case 290:
      return AV_PIX_FMT_RGBAF32BE;
    case 291:
      return AV_PIX_FMT_RGBAF32LE;
      //     case 292:
      //       return AV_PIX_FMT_RPI ;
      //     case 293:
      //       return AV_PIX_FMT_SAND128 ;
      //     case 294:
      //       return AV_PIX_FMT_SAND64_10 ;
      //     case 295:
      //       return AV_PIX_FMT_SAND64_16 ;
      //     case 296:
      //       return AV_PIX_FMT_RPI4_8 ;       // rpi turn on then only
      //     case 297:
      //      return AV_PIX_FMT_RPI4_10 ;
    case 298:
      return AV_PIX_FMT_RGB555;
    default:
      return AV_PIX_FMT_NONE;
    }
  }
};

class SampleFmt {
public:
  static AVSampleFormat fromSampleID(uint32_t SampleID) {
    switch (SampleID) {
    case 0:
      return AV_SAMPLE_FMT_NONE;
    case 1:
      return AV_SAMPLE_FMT_U8;
    case 2:
      return AV_SAMPLE_FMT_S16;
    case 3:
      return AV_SAMPLE_FMT_S32;
    case 4:
      return AV_SAMPLE_FMT_FLT;
    case 5:
      return AV_SAMPLE_FMT_DBL;
    case 6:
      return AV_SAMPLE_FMT_U8P;
    case 7:
      return AV_SAMPLE_FMT_S16P;
    case 8:
      return AV_SAMPLE_FMT_S32P;
    case 9:
      return AV_SAMPLE_FMT_FLTP;
    case 10:
      return AV_SAMPLE_FMT_DBLP;
    case 11:
      return AV_SAMPLE_FMT_S64;
    case 12:
      return AV_SAMPLE_FMT_S64P;
    case 13:
      return AV_SAMPLE_FMT_NB;
    default:
      return AV_SAMPLE_FMT_NONE;
    }
  }

  static uint32_t toSampleID(AVSampleFormat AvSampleFormat) {
    switch (AvSampleFormat) {
    case AV_SAMPLE_FMT_NONE:
      return 0;
    case AV_SAMPLE_FMT_U8:
      return 1;
    case AV_SAMPLE_FMT_S16:
      return 2;
    case AV_SAMPLE_FMT_S32:
      return 3;
    case AV_SAMPLE_FMT_FLT:
      return 4;
    case AV_SAMPLE_FMT_DBL:
      return 5;
    case AV_SAMPLE_FMT_U8P:
      return 6;
    case AV_SAMPLE_FMT_S16P:
      return 7;
    case AV_SAMPLE_FMT_S32P:
      return 8;
    case AV_SAMPLE_FMT_FLTP:
      return 9;
    case AV_SAMPLE_FMT_DBLP:
      return 10;
    case AV_SAMPLE_FMT_S64:
      return 11;
    case AV_SAMPLE_FMT_S64P:
      return 12;
    case AV_SAMPLE_FMT_NB:
      return 13;
    default:
      return 0;
    }
  }
};

// Could have avoided, but Did this to support older version of FFMPEG
// (V5,V4,V3) Version 6 FFmpeg uses AVChannelLayout Struct;
class ChannelLayout {
private:
  const static uint64_t FRONT_LEFT = 1;
  const static uint64_t FRONT_RIGHT = 1ULL << 1;
  const static uint64_t FRONT_CENTER = 1ULL << 2;
  const static uint64_t LOW_FREQUENCY = 1ULL << 3;
  const static uint64_t BACK_LEFT = 1ULL << 4;
  const static uint64_t BACK_RIGHT = 1ULL << 5;
  const static uint64_t FRONT_LEFT_OF_CENTER = 1ULL << 6;
  const static uint64_t FRONT_RIGHT_OF_CENTER = 1ULL << 7;
  const static uint64_t BACK_CENTER = 1ULL << 8;
  const static uint64_t SIDE_LEFT = 1ULL << 9;
  const static uint64_t SIDE_RIGHT = 1ULL << 10;
  const static uint64_t TOP_CENTER = 1ULL << 11;
  const static uint64_t TOP_FRONT_LEFT = 1ULL << 12;
  const static uint64_t TOP_FRONT_CENTER = 1ULL << 13;
  const static uint64_t TOP_FRONT_RIGHT = 1ULL << 14;
  const static uint64_t TOP_BACK_LEFT = 1ULL << 15;
  const static uint64_t TOP_BACK_CENTER = 1ULL << 16;
  const static uint64_t TOP_BACK_RIGHT = 1ULL << 17;
  const static uint64_t STEREO_LEFT = 1ULL << 18;
  const static uint64_t STEREO_RIGHT = 1ULL << 19;
  const static uint64_t WIDE_LEFT = 1ULL << 20;
  const static uint64_t WIDE_RIGHT = 1ULL << 21;
  const static uint64_t SURROUND_DIRECT_LEFT = 1ULL << 22;
  const static uint64_t SURROUND_DIRECT_RIGHT = 1ULL << 23;
  const static uint64_t LOW_FREQUENCY_2 = 1ULL << 24;
  const static uint64_t NATIVE = 1ULL << 25;

  const static uint64_t MONO = 1ULL << 26;
  const static uint64_t STEREO = 1ULL << 27;
  const static uint64_t _2POINT1 = 1ULL << 28;
  const static uint64_t _2_1 = 1ULL << 29;
  const static uint64_t SURROUND = 1ULL << 30;
  const static uint64_t _3POINT1 = 1ULL << 31;
  const static uint64_t _4POINT0 = 1ULL << 32;
  const static uint64_t _4POINT1 = 1ULL << 33;
  const static uint64_t _2_2 = 1ULL << 34;
  const static uint64_t QUAD = 1ULL << 35;
  const static uint64_t _5POINT0 = 1ULL << 36;
  const static uint64_t _5POINT1 = 1ULL << 37;
  const static uint64_t _5POINT0_BACK = 1ULL << 38;
  const static uint64_t _5POINT1_BACK = 1ULL << 39;
  const static uint64_t _6POINT0 = 1ULL << 40;
  const static uint64_t _6POINT0_FRONT = 1ULL << 41;
  const static uint64_t HEXAGONAL = 1ULL << 42;
  const static uint64_t _6POINT1 = 1ULL << 43;
  const static uint64_t _6POINT1_BACK = 1ULL << 44;
  const static uint64_t _6POINT1_FRONT = 1ULL << 45;
  const static uint64_t _7POINT0 = 1ULL << 46;
  const static uint64_t _7POINT0_FRONT = 1ULL << 47;
  const static uint64_t _7POINT1 = 1ULL << 48;
  const static uint64_t _7POINT1_WIDE = 1ULL << 49;
  const static uint64_t _7POINT1_WIDE_BACK = 1ULL << 50;
  const static uint64_t OCTAGONAL = 1ULL << 51;
  const static uint64_t HEXADECAGONAL = 1ULL << 52;
  const static uint64_t STEREO_DOWNMIX = 1ULL << 53;

public:
  // Check This function. (Looks good, test it)
  static uint64_t fromChannelLayoutID(uint64_t ChannelLayout) {
    uint64_t Channel = 0UL;
    if (ChannelLayout & FRONT_LEFT) {
      Channel |= AV_CH_FRONT_LEFT;
    }
    if (ChannelLayout & FRONT_RIGHT) {
      Channel |= AV_CH_FRONT_RIGHT;
    }
    if (ChannelLayout & FRONT_CENTER) {
      Channel |= AV_CH_FRONT_CENTER;
    }
    if (ChannelLayout & LOW_FREQUENCY) {
      Channel |= AV_CH_LOW_FREQUENCY;
    }
    if (ChannelLayout & BACK_LEFT) {
      Channel |= AV_CH_BACK_LEFT;
    }
    if (ChannelLayout & BACK_RIGHT) {
      Channel |= AV_CH_BACK_RIGHT;
    }
    if (ChannelLayout & FRONT_LEFT_OF_CENTER) {
      Channel |= AV_CH_FRONT_LEFT_OF_CENTER;
    }
    if (ChannelLayout & FRONT_RIGHT_OF_CENTER) {
      Channel |= AV_CH_FRONT_RIGHT_OF_CENTER;
    }
    if (ChannelLayout & BACK_CENTER) {
      Channel |= AV_CH_BACK_CENTER;
    }
    if (ChannelLayout & SIDE_LEFT) {
      Channel |= AV_CH_SIDE_LEFT;
    }
    if (ChannelLayout & SIDE_RIGHT) {
      Channel |= AV_CH_SIDE_RIGHT;
    }
    if (ChannelLayout & TOP_CENTER) {
      Channel |= AV_CH_TOP_CENTER;
    }
    if (ChannelLayout & TOP_FRONT_LEFT) {
      Channel |= AV_CH_TOP_FRONT_LEFT;
    }
    if (ChannelLayout & TOP_FRONT_CENTER) {
      Channel |= AV_CH_TOP_FRONT_CENTER;
    }
    if (ChannelLayout & TOP_FRONT_RIGHT) {
      Channel |= AV_CH_TOP_FRONT_RIGHT;
    }
    if (ChannelLayout & TOP_BACK_LEFT) {
      Channel |= AV_CH_TOP_BACK_LEFT;
    }
    if (ChannelLayout & TOP_BACK_CENTER) {
      Channel |= AV_CH_TOP_BACK_CENTER;
    }
    if (ChannelLayout & TOP_BACK_RIGHT) {
      Channel |= AV_CH_TOP_BACK_RIGHT;
    }
    if (ChannelLayout & STEREO_LEFT) {
      Channel |= AV_CH_STEREO_LEFT;
    }
    if (ChannelLayout & STEREO_RIGHT) {
      Channel |= AV_CH_STEREO_RIGHT;
    }
    if (ChannelLayout & WIDE_LEFT) {
      Channel |= AV_CH_WIDE_LEFT;
    }
    if (ChannelLayout & WIDE_RIGHT) {
      Channel |= AV_CH_WIDE_RIGHT;
    }
    if (ChannelLayout & SURROUND_DIRECT_LEFT) {
      Channel |= AV_CH_SURROUND_DIRECT_LEFT;
    }
    if (ChannelLayout & SURROUND_DIRECT_RIGHT) {
      Channel |= AV_CH_SURROUND_DIRECT_RIGHT;
    }
    if (ChannelLayout & LOW_FREQUENCY_2) {
      Channel |= AV_CH_LOW_FREQUENCY_2;
    }
    if (ChannelLayout & NATIVE) {
      Channel |= AV_CH_LAYOUT_NATIVE;
    }
    if (ChannelLayout & MONO) {
      Channel |= AV_CH_LAYOUT_MONO;
    }
    if (ChannelLayout & STEREO) {
      Channel |= AV_CH_LAYOUT_STEREO;
    }
    if (ChannelLayout & _2POINT1) {
      Channel |= AV_CH_LAYOUT_2POINT1;
    }
    if (ChannelLayout & _2_1) {
      Channel |= AV_CH_LAYOUT_2_1;
    }
    if (ChannelLayout & SURROUND) {
      Channel |= AV_CH_LAYOUT_SURROUND;
    }
    if (ChannelLayout & _3POINT1) {
      Channel |= AV_CH_LAYOUT_3POINT1;
    }
    if (ChannelLayout & _4POINT0) {
      Channel |= AV_CH_LAYOUT_4POINT0;
    }
    if (ChannelLayout & _4POINT1) {
      Channel |= AV_CH_LAYOUT_4POINT1;
    }
    if (ChannelLayout & _2_2) {
      Channel |= AV_CH_LAYOUT_2_2;
    }
    if (ChannelLayout & QUAD) {
      Channel |= AV_CH_LAYOUT_QUAD;
    }
    if (ChannelLayout & _5POINT0) {
      Channel |= AV_CH_LAYOUT_5POINT0;
    }
    if (ChannelLayout & _5POINT1) {
      Channel |= AV_CH_LAYOUT_5POINT1;
    }
    if (ChannelLayout & _5POINT0_BACK) {
      Channel |= AV_CH_LAYOUT_5POINT0_BACK;
    }
    if (ChannelLayout & _5POINT1_BACK) {
      Channel |= AV_CH_LAYOUT_5POINT1_BACK;
    }
    if (ChannelLayout & _6POINT0) {
      Channel |= AV_CH_LAYOUT_6POINT0;
    }
    if (ChannelLayout & _6POINT0_FRONT) {
      Channel |= AV_CH_LAYOUT_6POINT0_FRONT;
    }
    if (ChannelLayout & HEXAGONAL) {
      Channel |= AV_CH_LAYOUT_HEXAGONAL;
    }
    if (ChannelLayout & _6POINT1) {
      Channel |= AV_CH_LAYOUT_6POINT1;
    }
    if (ChannelLayout & _6POINT1_BACK) {
      Channel |= AV_CH_LAYOUT_6POINT1_BACK;
    }
    if (ChannelLayout & _6POINT1_FRONT) {
      Channel |= AV_CH_LAYOUT_6POINT1_FRONT;
    }
    if (ChannelLayout & _7POINT0) {
      Channel |= AV_CH_LAYOUT_7POINT0;
    }
    if (ChannelLayout & _7POINT0_FRONT) {
      Channel |= AV_CH_LAYOUT_7POINT0_FRONT;
    }
    if (ChannelLayout & _7POINT1) {
      Channel |= AV_CH_LAYOUT_7POINT1;
    }
    if (ChannelLayout & _7POINT1_WIDE) {
      Channel |= AV_CH_LAYOUT_7POINT1_WIDE;
    }
    if (ChannelLayout & _7POINT1_WIDE_BACK) {
      Channel |= AV_CH_LAYOUT_7POINT1_WIDE_BACK;
    }
    if (ChannelLayout & OCTAGONAL) {
      Channel |= AV_CH_LAYOUT_OCTAGONAL;
    }
    if (ChannelLayout & HEXADECAGONAL) {
      Channel |= AV_CH_LAYOUT_HEXADECAGONAL;
    }
    if (ChannelLayout & STEREO_DOWNMIX) {
      Channel |= AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    return Channel;
  }

  // Perfect Logic :)
  static uint64_t intoChannelLayoutID(uint64_t ChannelLayout) {
    uint64_t Channel = 0;
    if ((ChannelLayout & AV_CH_FRONT_LEFT) == AV_CH_FRONT_LEFT) {
      Channel |= FRONT_LEFT;
    }
    if ((ChannelLayout & AV_CH_FRONT_RIGHT) == AV_CH_FRONT_RIGHT) {
      Channel |= FRONT_RIGHT;
    }
    if ((ChannelLayout & AV_CH_FRONT_CENTER) == AV_CH_FRONT_CENTER) {
      Channel |= FRONT_CENTER;
    }
    if ((ChannelLayout & AV_CH_LOW_FREQUENCY) == AV_CH_LOW_FREQUENCY) {
      Channel |= LOW_FREQUENCY;
    }
    if ((ChannelLayout & AV_CH_BACK_LEFT) == AV_CH_BACK_LEFT) {
      Channel |= BACK_LEFT;
    }
    if ((ChannelLayout & AV_CH_BACK_RIGHT) == AV_CH_BACK_RIGHT) {
      Channel |= BACK_RIGHT;
    }
    if ((ChannelLayout & AV_CH_FRONT_LEFT_OF_CENTER) ==
        AV_CH_FRONT_LEFT_OF_CENTER) {
      Channel |= FRONT_LEFT_OF_CENTER;
    }
    if ((ChannelLayout & AV_CH_FRONT_RIGHT_OF_CENTER) ==
        AV_CH_FRONT_RIGHT_OF_CENTER) {
      Channel |= FRONT_RIGHT_OF_CENTER;
    }
    if ((ChannelLayout & AV_CH_BACK_CENTER) == AV_CH_BACK_CENTER) {
      Channel |= BACK_CENTER;
    }
    if ((ChannelLayout & AV_CH_SIDE_LEFT) == AV_CH_SIDE_LEFT) {
      Channel |= SIDE_LEFT;
    }
    if ((ChannelLayout & AV_CH_SIDE_RIGHT) == AV_CH_SIDE_RIGHT) {
      Channel |= SIDE_RIGHT;
    }
    if ((ChannelLayout & AV_CH_TOP_CENTER) == AV_CH_TOP_CENTER) {
      Channel |= TOP_CENTER;
    }
    if ((ChannelLayout & AV_CH_TOP_FRONT_LEFT) == AV_CH_TOP_FRONT_LEFT) {
      Channel |= TOP_FRONT_LEFT;
    }
    if ((ChannelLayout & AV_CH_TOP_FRONT_CENTER) == AV_CH_TOP_FRONT_CENTER) {
      Channel |= TOP_FRONT_CENTER;
    }
    if ((ChannelLayout & AV_CH_TOP_FRONT_RIGHT) == AV_CH_TOP_FRONT_RIGHT) {
      Channel |= TOP_FRONT_RIGHT;
    }
    if ((ChannelLayout & AV_CH_TOP_BACK_LEFT) == AV_CH_TOP_BACK_LEFT) {
      Channel |= TOP_BACK_LEFT;
    }
    if ((ChannelLayout & AV_CH_TOP_BACK_CENTER) == AV_CH_TOP_BACK_CENTER) {
      Channel |= TOP_BACK_CENTER;
    }
    if ((ChannelLayout & AV_CH_TOP_BACK_RIGHT) == AV_CH_TOP_BACK_RIGHT) {
      Channel |= TOP_BACK_RIGHT;
    }
    if ((ChannelLayout & AV_CH_STEREO_LEFT) == AV_CH_STEREO_LEFT) {
      Channel |= STEREO_LEFT;
    }
    if ((ChannelLayout & AV_CH_STEREO_RIGHT) == AV_CH_STEREO_RIGHT) {
      Channel |= STEREO_RIGHT;
    }
    if ((ChannelLayout & AV_CH_WIDE_LEFT) == AV_CH_WIDE_LEFT) {
      Channel |= WIDE_LEFT;
    }
    if ((ChannelLayout & AV_CH_WIDE_RIGHT) == AV_CH_WIDE_RIGHT) {
      Channel |= WIDE_RIGHT;
    }
    if ((ChannelLayout & AV_CH_SURROUND_DIRECT_LEFT) ==
        AV_CH_SURROUND_DIRECT_LEFT) {
      Channel |= SURROUND_DIRECT_LEFT;
    }
    if ((ChannelLayout & AV_CH_SURROUND_DIRECT_RIGHT) ==
        AV_CH_SURROUND_DIRECT_RIGHT) {
      Channel |= SURROUND_DIRECT_RIGHT;
    }
    if ((ChannelLayout & AV_CH_LOW_FREQUENCY_2) == AV_CH_LOW_FREQUENCY_2) {
      Channel |= LOW_FREQUENCY_2;
    }

    // Channel Mask C;
    if ((ChannelLayout & AV_CH_LAYOUT_NATIVE) == AV_CH_LAYOUT_NATIVE) {
      Channel |= NATIVE;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_MONO) == AV_CH_LAYOUT_MONO) {
      Channel |= MONO;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_STEREO) == AV_CH_LAYOUT_STEREO) {
      Channel |= STEREO;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_2POINT1) == AV_CH_LAYOUT_2POINT1) {
      Channel |= _2POINT1;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_2_1) == AV_CH_LAYOUT_2_1) {
      Channel |= _2_1;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_SURROUND) == AV_CH_LAYOUT_SURROUND) {
      Channel |= SURROUND;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_3POINT1) == AV_CH_LAYOUT_3POINT1) {
      Channel |= _3POINT1;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_4POINT0) == AV_CH_LAYOUT_4POINT0) {
      Channel |= _4POINT0;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_4POINT1) == AV_CH_LAYOUT_4POINT1) {
      Channel |= _4POINT1;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_2_2) == AV_CH_LAYOUT_2_2) {
      Channel |= _2_2;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_QUAD) == AV_CH_LAYOUT_QUAD) {
      Channel |= QUAD;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_5POINT0) == AV_CH_LAYOUT_5POINT0) {
      Channel |= _5POINT0;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_5POINT1) == AV_CH_LAYOUT_5POINT1) {
      Channel |= _5POINT1;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_5POINT0_BACK) ==
        AV_CH_LAYOUT_5POINT0_BACK) {
      Channel |= _5POINT0_BACK;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_5POINT1_BACK) ==
        AV_CH_LAYOUT_5POINT1_BACK) {
      Channel |= _5POINT1_BACK;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_6POINT0) == AV_CH_LAYOUT_6POINT0) {
      Channel |= _6POINT0;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_6POINT0_FRONT) ==
        AV_CH_LAYOUT_6POINT0_FRONT) {
      Channel |= _6POINT0_FRONT;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_HEXAGONAL) == AV_CH_LAYOUT_HEXAGONAL) {
      Channel |= HEXAGONAL;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_6POINT1) == AV_CH_LAYOUT_6POINT1) {
      Channel |= _6POINT1;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_6POINT1_BACK) ==
        AV_CH_LAYOUT_6POINT1_BACK) {
      Channel |= _6POINT1_BACK;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_6POINT1_FRONT) ==
        AV_CH_LAYOUT_6POINT1_FRONT) {
      Channel |= _6POINT1_FRONT;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_7POINT0) == AV_CH_LAYOUT_7POINT0) {
      Channel |= _7POINT0;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_7POINT0_FRONT) ==
        AV_CH_LAYOUT_7POINT0_FRONT) {
      Channel |= _7POINT0_FRONT;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_7POINT1) == AV_CH_LAYOUT_7POINT1) {
      Channel |= _7POINT1;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_7POINT1_WIDE) ==
        AV_CH_LAYOUT_7POINT1_WIDE) {
      Channel |= _7POINT1_WIDE;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_7POINT1_WIDE_BACK) ==
        AV_CH_LAYOUT_7POINT1_WIDE_BACK) {
      Channel |= _7POINT1_WIDE_BACK;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_OCTAGONAL) == AV_CH_LAYOUT_OCTAGONAL) {
      Channel |= OCTAGONAL;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_HEXADECAGONAL) ==
        AV_CH_LAYOUT_HEXADECAGONAL) {
      Channel |= HEXADECAGONAL;
    }
    if ((ChannelLayout & AV_CH_LAYOUT_STEREO_DOWNMIX) ==
        AV_CH_LAYOUT_STEREO_DOWNMIX) {
      Channel |= STEREO_DOWNMIX;
    }
    return Channel;
  }
};

class SWRFilterType {
public:
  uint32_t fromSwrFilterType(SwrFilterType FilterType) {
    switch (FilterType) {
    case SWR_FILTER_TYPE_CUBIC:
      return 1;
    case SWR_FILTER_TYPE_BLACKMAN_NUTTALL:
      return 2;
    case SWR_FILTER_TYPE_KAISER:
      return 3;
    default:
      return 1;
    }
  }

  SwrFilterType intoSwrFilterType(uint32_t FilterID) {
    switch (FilterID) {
    case 1:
      return SWR_FILTER_TYPE_CUBIC;
    case 2:
      return SWR_FILTER_TYPE_BLACKMAN_NUTTALL;
    case 3:
      return SWR_FILTER_TYPE_KAISER;
    default:
      return SWR_FILTER_TYPE_CUBIC;
    }
  }
};

class SWREngine {
public:
  SwrEngine intoSwrEngine(uint32_t EngineId) {
    switch (EngineId) {
    case 1:
      return SWR_ENGINE_SWR;
    case 2:
      return SWR_ENGINE_SOXR;
    default:
      return SWR_ENGINE_SWR;
    }
  }

  uint32_t fromSwrEngine(SwrEngine Engine) {
    switch (Engine) {
    case SWR_ENGINE_SWR:
      return 1;
    case SWR_ENGINE_SOXR:
      return 2;
    case SWR_ENGINE_NB:
      return 3;
    default:
      return SWR_ENGINE_SWR;
    }
  }
};

class SWRDitherType {
public:
  SwrDitherType intoSwrDitherType(uint32_t SwrDitherId) {
    switch (SwrDitherId) {
    case 0:
      return SWR_DITHER_NONE;
    case 1:
      return SWR_DITHER_RECTANGULAR;
    case 2:
      return SWR_DITHER_TRIANGULAR;
    case 3:
      return SWR_DITHER_TRIANGULAR_HIGHPASS;
    case 64:
      return SWR_DITHER_NS;
    case 4:
      return SWR_DITHER_NS_LIPSHITZ;
    case 5:
      return SWR_DITHER_NS_F_WEIGHTED;
    case 6:
      return SWR_DITHER_NS_MODIFIED_E_WEIGHTED;
    case 7:
      return SWR_DITHER_NS_IMPROVED_E_WEIGHTED;
    case 8:
      return SWR_DITHER_NS_SHIBATA;
    case 9:
      return SWR_DITHER_NS_LOW_SHIBATA;
    case 10:
      return SWR_DITHER_NS_HIGH_SHIBATA;
    case 11:
      return SWR_DITHER_NB;
    default:
      return SWR_DITHER_NONE;
    }
  }

  uint32_t fromSwrDitherType(SwrDitherType SwrDitherType) {
    switch (SwrDitherType) {
    case SWR_DITHER_NONE:
      return 0;
    case SWR_DITHER_RECTANGULAR:
      return 1;
    case SWR_DITHER_TRIANGULAR:
      return 2;
    case SWR_DITHER_TRIANGULAR_HIGHPASS:
      return 3;
    case SWR_DITHER_NS:
      return 64;
    case SWR_DITHER_NS_LIPSHITZ:
      return 4;
    case SWR_DITHER_NS_F_WEIGHTED:
      return 5;
    case SWR_DITHER_NS_MODIFIED_E_WEIGHTED:
      return 6;
    case SWR_DITHER_NS_IMPROVED_E_WEIGHTED:
      return 7;
    case SWR_DITHER_NS_SHIBATA:
      return 8;
    case SWR_DITHER_NS_LOW_SHIBATA:
      return 9;
    case SWR_DITHER_NS_HIGH_SHIBATA:
      return 10;
    case SWR_DITHER_NB:
      return 11;
    default:
      return 0;
    }
  }
};

class ChromaLocation {
public:
  static AVChromaLocation intoAVChromaLocation(int32_t ChromaLocationId) {
    switch (ChromaLocationId) {
    case 0:
      return AVCHROMA_LOC_UNSPECIFIED;
    case 1:
      return AVCHROMA_LOC_LEFT;
    case 2:
      return AVCHROMA_LOC_CENTER;
    case 3:
      return AVCHROMA_LOC_TOPLEFT;
    case 4:
      return AVCHROMA_LOC_TOP;
    case 5:
      return AVCHROMA_LOC_BOTTOMLEFT;
    case 6:
      return AVCHROMA_LOC_BOTTOM;
    default:
      return AVCHROMA_LOC_UNSPECIFIED;
    }
  }

  static int32_t fromAVChromaLocation(AVChromaLocation ChromaLocation) {
    switch (ChromaLocation) {
    case AVCHROMA_LOC_UNSPECIFIED:
      return 0;
    case AVCHROMA_LOC_LEFT:
      return 1;
    case AVCHROMA_LOC_CENTER:
      return 2;
    case AVCHROMA_LOC_TOPLEFT:
      return 3;
    case AVCHROMA_LOC_TOP:
      return 4;
    case AVCHROMA_LOC_BOTTOMLEFT:
      return 5;
    case AVCHROMA_LOC_BOTTOM:
      return 6;
    default:
      return 0;
    }
  }
};

class Rounding {

public:
  static AVRounding intoAVRounding(int32_t RoundingId) {
    switch (RoundingId) {
    case 0:
      return AV_ROUND_ZERO;
    case 1:
      return AV_ROUND_INF;
    case 2:
      return AV_ROUND_DOWN;
    case 3:
      return AV_ROUND_UP;
    case 4:
      return AV_ROUND_NEAR_INF;
    case 5:
      return AV_ROUND_PASS_MINMAX;
    default:
      return AV_ROUND_ZERO;
    }
  }

  static int32_t fromAVRounding(AVRounding Rounding) {
    switch (Rounding) {
    case AV_ROUND_ZERO:
      return 0;
    case AV_ROUND_INF:
      return 1;
    case AV_ROUND_DOWN:
      return 2;
    case AV_ROUND_UP:
      return 3;
    case AV_ROUND_NEAR_INF:
      return 4;
    case AV_ROUND_PASS_MINMAX:
      return 5;
    default:
      return 0;
    }
  }
};

class OptionType {

public:
  static AVOptionType intoAVOptionType(int32_t RoundingId) {
    switch (RoundingId) {
    case 0:
      return AV_OPT_TYPE_FLAGS;
    case 1:
      return AV_OPT_TYPE_INT;
    case 2:
      return AV_OPT_TYPE_INT64;
    case 3:
      return AV_OPT_TYPE_DOUBLE;
    case 4:
      return AV_OPT_TYPE_FLOAT;
    case 5:
      return AV_OPT_TYPE_STRING;
    case 6:
      return AV_OPT_TYPE_RATIONAL;
    case 7:
      return AV_OPT_TYPE_BINARY;
    case 8:
      return AV_OPT_TYPE_DICT;
    case 9:
      return AV_OPT_TYPE_CONST;
    case 10:
      return AV_OPT_TYPE_IMAGE_SIZE;
    case 11:
      return AV_OPT_TYPE_PIXEL_FMT;
    case 12:
      return AV_OPT_TYPE_SAMPLE_FMT;
    case 13:
      return AV_OPT_TYPE_VIDEO_RATE;
    case 14:
      return AV_OPT_TYPE_DURATION;
    case 15:
      return AV_OPT_TYPE_COLOR;
    case 16:
      return AV_OPT_TYPE_CHANNEL_LAYOUT;
    case 17:
      return AV_OPT_TYPE_UINT64;
    case 18:
      return AV_OPT_TYPE_BOOL;
    case 19:
      return AV_OPT_TYPE_CHLAYOUT;
    default:
      return AV_OPT_TYPE_FLAGS;
    }
  }

  static int32_t fromAVOptionType(AVOptionType OptionType) {
    switch (OptionType) {
    case AV_OPT_TYPE_FLAGS:
      return 0;
    case AV_OPT_TYPE_INT:
      return 1;
    case AV_OPT_TYPE_INT64:
      return 2;
    case AV_OPT_TYPE_DOUBLE:
      return 3;
    case AV_OPT_TYPE_FLOAT:
      return 4;
    case AV_OPT_TYPE_STRING:
      return 5;
    case AV_OPT_TYPE_RATIONAL:
      return 6;
    case AV_OPT_TYPE_BINARY:
      return 7;
    case AV_OPT_TYPE_DICT:
      return 8;
    case AV_OPT_TYPE_CONST:
      return 9;
    case AV_OPT_TYPE_IMAGE_SIZE:
      return 10;
    case AV_OPT_TYPE_PIXEL_FMT:
      return 11;
    case AV_OPT_TYPE_SAMPLE_FMT:
      return 12;
    case AV_OPT_TYPE_VIDEO_RATE:
      return 13;
    case AV_OPT_TYPE_DURATION:
      return 14;
    case AV_OPT_TYPE_COLOR:
      return 15;
    case AV_OPT_TYPE_CHANNEL_LAYOUT:
      return 16;
    case AV_OPT_TYPE_UINT64:
      return 17;
    case AV_OPT_TYPE_BOOL:
      return 18;
    case AV_OPT_TYPE_CHLAYOUT:
      return 19;
    default:
      return 0;
    }
  }
};

class PictureType {
public:
  static AVPictureType intoAVPictureType(int32_t PictureId) {
    switch (PictureId) {
    case 0:
      return AV_PICTURE_TYPE_NONE;
    case 1:
      return AV_PICTURE_TYPE_I;
    case 2:
      return AV_PICTURE_TYPE_P;
    case 3:
      return AV_PICTURE_TYPE_B;
    case 4:
      return AV_PICTURE_TYPE_S;
    case 5:
      return AV_PICTURE_TYPE_SI;
    case 6:
      return AV_PICTURE_TYPE_SP;
    case 7:
      return AV_PICTURE_TYPE_BI;
    default:
      return AV_PICTURE_TYPE_NONE;
    }
  };

  static int32_t fromAVPictureType(AVPictureType PictureType) {
    switch (PictureType) {
    case AV_PICTURE_TYPE_NONE:
      return 0;
    case AV_PICTURE_TYPE_I:
      return 1;
    case AV_PICTURE_TYPE_P:
      return 2;
    case AV_PICTURE_TYPE_B:
      return 3;
    case AV_PICTURE_TYPE_S:
      return 4;
    case AV_PICTURE_TYPE_SI:
      return 5;
    case AV_PICTURE_TYPE_SP:
      return 6;
    case AV_PICTURE_TYPE_BI:
      return 7;
    default:
      return 0;
    }
  }
};

// Direct mapping in rust. Not required. Can be used for decoupling (Clean
// Code).
//
// class ColorTransferCharacteristic {
//
//  static AVColorTransferCharacteristic
//  intoColorTransferCharacteristic(uint32_t ColorTransferCharacteristicId) {
//    switch (ColorTransferCharacteristicId) {
//    case 0:
//      return AVCOL_TRC_RESERVED0;
//    case 1:
//      return AVCOL_TRC_BT709;
//    case 2:
//      return AVCOL_TRC_UNSPECIFIED;
//    case 3:
//      return AVCOL_TRC_RESERVED;
//    case 4:
//      return AVCOL_TRC_GAMMA22;
//    case 5:
//      return AVCOL_TRC_GAMMA28;
//    case 6:
//      return AVCOL_TRC_SMPTE170M;
//    case 7:
//      return AVCOL_TRC_SMPTE240M;
//    case 8:
//      return AVCOL_TRC_LINEAR;
//    case 9:
//      return AVCOL_TRC_LOG;
//    case 10:
//      return AVCOL_TRC_LOG_SQRT;
//    case 11:
//      return AVCOL_TRC_IEC61966_2_4;
//    case 12:
//      return AVCOL_TRC_BT1361_ECG;
//    case 13:
//      return AVCOL_TRC_IEC61966_2_1;
//    case 14:
//      return AVCOL_TRC_BT2020_10;
//    case 15:
//      return AVCOL_TRC_BT2020_12;
//    case 16:
//      return AVCOL_TRC_SMPTE2084;
//    case 17:
//      return AVCOL_TRC_SMPTE428;
//    case 18:
//      return AVCOL_TRC_ARIB_STD_B67;
//    case 19:
//      return AVCOL_TRC_NB;
//    default:
//      return AVCOL_TRC_RESERVED0;
//    }
//  };
//
//  static uint32_t
//  fromColorTransferCharacteristic(uint32_t ColorTransferCharacteristic) {
//    switch (ColorTransferCharacteristic) {
//    case AVCOL_TRC_RESERVED0:
//      return 0;
//    case AVCOL_TRC_BT709:
//      return 1;
//    case AVCOL_TRC_UNSPECIFIED:
//      return 2;
//    case AVCOL_TRC_RESERVED:
//      return 3;
//    case AVCOL_TRC_GAMMA22:
//      return 4;
//    case AVCOL_TRC_GAMMA28:
//      return 5;
//    case AVCOL_TRC_SMPTE170M:
//      return 6;
//    case AVCOL_TRC_SMPTE240M:
//      return 7;
//    case AVCOL_TRC_LINEAR:
//      return 8;
//    case AVCOL_TRC_LOG:
//      return 9;
//    case AVCOL_TRC_LOG_SQRT:
//      return 10;
//    case AVCOL_TRC_IEC61966_2_4:
//      return 11;
//    case AVCOL_TRC_BT1361_ECG:
//      return 12;
//    case AVCOL_TRC_IEC61966_2_1:
//      return 13;
//    case AVCOL_TRC_BT2020_10:
//      return 14;
//    case AVCOL_TRC_BT2020_12:
//      return 15;
//    case AVCOL_TRC_SMPTE2084:
//      return 16;
//    case AVCOL_TRC_SMPTE428:
//      return 17;
//    case AVCOL_TRC_ARIB_STD_B67:
//      return 18;
//    case AVCOL_TRC_NB:
//      return 19;
//    default:
//      return 0;
//    }
//  };
//};

// We can keep or remove the binding.
class ColorSpace {

public:
  static AVColorSpace intoAVColorSpace(int32_t ColorSpaceId) {

    switch (ColorSpaceId) {
    case 0:
      return AVCOL_SPC_RGB;
    case 1:
      return AVCOL_SPC_BT709;
    case 2:
      return AVCOL_SPC_UNSPECIFIED;
    case 3:
      return AVCOL_SPC_RESERVED;
    case 4:
      return AVCOL_SPC_FCC;
    case 5:
      return AVCOL_SPC_BT470BG;
    case 6:
      return AVCOL_SPC_SMPTE170M;
    case 7:
      return AVCOL_SPC_SMPTE240M;
    case 8:
      return AVCOL_SPC_YCGCO;
    case 9:
      return AVCOL_SPC_BT2020_NCL;
    case 10:
      return AVCOL_SPC_BT2020_CL;
    case 11:
      return AVCOL_SPC_SMPTE2085;
    case 12:
      return AVCOL_SPC_CHROMA_DERIVED_NCL;
    case 13:
      return AVCOL_SPC_CHROMA_DERIVED_CL;
    case 14:
      return AVCOL_SPC_ICTCP;
    default:
      return AVCOL_SPC_RGB;
    }
  };

  static int32_t fromAVColorSpace(AVColorSpace ColorSpace) {

    switch (ColorSpace) {
    case AVCOL_SPC_RGB:
      return 0;
    case AVCOL_SPC_BT709:
      return 1;
    case AVCOL_SPC_UNSPECIFIED:
      return 2;
    case AVCOL_SPC_RESERVED:
      return 3;
    case AVCOL_SPC_FCC:
      return 4;
    case AVCOL_SPC_BT470BG:
      return 5;
    case AVCOL_SPC_SMPTE170M:
      return 6;
    case AVCOL_SPC_SMPTE240M:
      return 7;
    case AVCOL_SPC_YCGCO:
      return 8;
    case AVCOL_SPC_BT2020_NCL:
      return 9;
    case AVCOL_SPC_BT2020_CL:
      return 10;
    case AVCOL_SPC_SMPTE2085:
      return 11;
    case AVCOL_SPC_CHROMA_DERIVED_NCL:
      return 12;
    case AVCOL_SPC_CHROMA_DERIVED_CL:
      return 13;
    case AVCOL_SPC_ICTCP:
      return 14;
    default:
      return 0;
    }
  };
};

class FieldOrder {
public:
  static AVFieldOrder intoAVFieldOrder(int32_t FieldOrderId) {
    switch (FieldOrderId) {
    case 0:
      return AV_FIELD_UNKNOWN;
    case 1:
      return AV_FIELD_PROGRESSIVE;
    case 2:
      return AV_FIELD_TT;
    case 3:
      return AV_FIELD_BB;
    case 4:
      return AV_FIELD_TB;
    case 5:
      return AV_FIELD_BT;
    default:
      return AV_FIELD_UNKNOWN;
    }
  }

  static int32_t fromAVFieldOrder(AVFieldOrder FieldOrder) {
    switch (FieldOrder) {
    case AV_FIELD_UNKNOWN:
      return 0;
    case AV_FIELD_PROGRESSIVE:
      return 1;
    case AV_FIELD_TT:
      return 2;
    case AV_FIELD_BB:
      return 3;
    case AV_FIELD_TB:
      return 4;
    case AV_FIELD_BT:
      return 5;
    default:
      return 0;
    }
  }
};

class ColorPrimaries {

public:
  static AVColorPrimaries intoAVColorPrimaries(int32_t ColorPrimariesId) {
    switch (ColorPrimariesId) {
    case 0:
      return AVCOL_PRI_RESERVED0;
    case 1:
      return AVCOL_PRI_BT709;
    case 2:
      return AVCOL_PRI_UNSPECIFIED;
    case 3:
      return AVCOL_PRI_RESERVED;
    case 4:
      return AVCOL_PRI_BT470M;
    case 5:
      return AVCOL_PRI_BT470BG;
    case 6:
      return AVCOL_PRI_SMPTE170M;
    case 7:
      return AVCOL_PRI_SMPTE240M;
    case 8:
      return AVCOL_PRI_FILM;
    case 9:
      return AVCOL_PRI_BT2020;
    case 10:
      return AVCOL_PRI_SMPTE428;
    case 11:
      return AVCOL_PRI_SMPTE431;
    case 12:
      return AVCOL_PRI_SMPTE432;
    case 13:
      return AVCOL_PRI_JEDEC_P22;
    case 14:
      return AVCOL_PRI_EBU3213;
    default:
      return AVCOL_PRI_RESERVED0;
    }
  };

  static int32_t fromAVColorPrimaries(AVColorPrimaries ColorPrimaries) {
    switch (ColorPrimaries) {
    case AVCOL_PRI_RESERVED0:
      return 0;
    case AVCOL_PRI_BT709:
      return 1;
    case AVCOL_PRI_UNSPECIFIED:
      return 2;
    case AVCOL_PRI_RESERVED:
      return 3;
    case AVCOL_PRI_BT470M:
      return 4;
    case AVCOL_PRI_BT470BG:
      return 5;
    case AVCOL_PRI_SMPTE170M:
      return 6;
    case AVCOL_PRI_SMPTE240M:
      return 7;
    case AVCOL_PRI_FILM:
      return 8;
    case AVCOL_PRI_BT2020:
      return 9;
    case AVCOL_PRI_SMPTE428:
      return 10;
    case AVCOL_PRI_SMPTE431:
      return 11;
    case AVCOL_PRI_SMPTE432:
      return 12;
      // #[cfg(not(feature = "ffmpeg_4_3"))]
      //     case AVCOL_PRI_JEDEC_P22:
      //       return 13;
    case AVCOL_PRI_EBU3213:
      return 14;
    default:
      return 0;
    }
  };
};

} // namespace FFmpegUtils
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge
