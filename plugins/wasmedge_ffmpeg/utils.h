extern "C" {
  #include "libavutil/avutil.h"
  #include "libavcodec/codec_id.h"
  #include "libavutil/channel_layout.h"
}
namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace FFmpegUtils {
class MediaType {
public:
  static AVMediaType intoMediaType(int32_t mediaTypeId){
    switch (mediaTypeId) {
    case 0:
      return AVMEDIA_TYPE_VIDEO;
    case 1:
      return AVMEDIA_TYPE_AUDIO;
    case 2 :
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

  static int32_t fromMediaType(AVMediaType mediaType){
    switch (mediaType) {
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
  static AVCodecID intoAVCodecID(uint32_t avCodecIndex) {
    switch (avCodecIndex) {
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
      return AV_CODEC_ID_KGV1;
    case 138:
      return AV_CODEC_ID_YOP;
    case 139:
      return AV_CODEC_ID_VP8;
    case 140:
      return AV_CODEC_ID_PICTOR;
    case 141:
      return AV_CODEC_ID_ANSI;
    case 142:
      return AV_CODEC_ID_A64_MULTI;
    case 143:
      return AV_CODEC_ID_A64_MULTI5;
    case 144:
      return AV_CODEC_ID_R10K;
    case 145:
      return AV_CODEC_ID_MXPEG;
    case 146:
      return AV_CODEC_ID_LAGARITH;
    case 147:
      return AV_CODEC_ID_PRORES;
    case 148:
      return AV_CODEC_ID_JV;
    case 149:
      return AV_CODEC_ID_DFA;
    case 150:
      return AV_CODEC_ID_WMV3IMAGE;
    case 151:
      return AV_CODEC_ID_VC1IMAGE;
    case 152:
      return AV_CODEC_ID_UTVIDEO;
    case 153:
      return AV_CODEC_ID_BMV_VIDEO;
    case 154:
      return AV_CODEC_ID_VBLE;
    case 155:
      return AV_CODEC_ID_DXTORY;
    case 156:
      return AV_CODEC_ID_V410;
    case 157:
      return AV_CODEC_ID_XWD;
    case 158:
      return AV_CODEC_ID_CDXL;
    case 159:
      return AV_CODEC_ID_XBM;
    case 160:
      return AV_CODEC_ID_ZEROCODEC;
    case 161:
      return AV_CODEC_ID_MSS1;
    case 162:
      return AV_CODEC_ID_MSA1;
    case 163:
      return AV_CODEC_ID_TSCC2;
    case 164:
      return AV_CODEC_ID_MTS2;
    case 165:
      return AV_CODEC_ID_CLLC;
    case 166:
      return AV_CODEC_ID_MSS2;
    case 167:
      return AV_CODEC_ID_VP9;
    case 168:
      return AV_CODEC_ID_AIC;
    case 169:
      return AV_CODEC_ID_ESCAPE130;
    case 170:
      return AV_CODEC_ID_G2M;
    case 171:
      return AV_CODEC_ID_WEBP;
    case 172:
      return AV_CODEC_ID_HNM4_VIDEO;
    case 173:
      return AV_CODEC_ID_HEVC;
    case 174:
      return AV_CODEC_ID_FIC;
    case 175:
      return AV_CODEC_ID_ALIAS_PIX;
    case 176:
      return AV_CODEC_ID_BRENDER_PIX;
    case 177:
      return AV_CODEC_ID_PAF_VIDEO;
    case 178:
      return AV_CODEC_ID_EXR;
    case 179:
      return AV_CODEC_ID_VP7;
    case 180:
      return AV_CODEC_ID_SANM;
    case 181:
      return AV_CODEC_ID_SGIRLE;
    case 182:
      return AV_CODEC_ID_MVC1;
    case 183:
      return AV_CODEC_ID_MVC2;
    case 184:
      return AV_CODEC_ID_HQX;
    case 185:
      return AV_CODEC_ID_TDSC;
    case 186:
      return AV_CODEC_ID_HQ_HQA;
    case 187:
      return AV_CODEC_ID_HAP;
    case 188:
      return AV_CODEC_ID_DDS;
    case 189:
      return AV_CODEC_ID_DXV;
    case 190:
      return AV_CODEC_ID_SCREENPRESSO;
    case 191:
      return AV_CODEC_ID_RSCC;
    case 192:
      return AV_CODEC_ID_AVS2;
    case 193:
      return AV_CODEC_ID_PGX;
    case 194:
      return AV_CODEC_ID_AVS3;
    case 195:
      return AV_CODEC_ID_MSP2;
    case 196:
      return AV_CODEC_ID_VVC;
    case 197:
      return AV_CODEC_ID_Y41P;
    case 198:
      return AV_CODEC_ID_AVRP;
    case 199:
      return AV_CODEC_ID_012V;
    case 200:
      return AV_CODEC_ID_AVUI;
    case 201:
      return AV_CODEC_ID_AYUV;
    case 202:
      return AV_CODEC_ID_TARGA_Y216;
    case 203:
      return AV_CODEC_ID_V308;
    case 204:
      return AV_CODEC_ID_V408;
    case 205:
      return AV_CODEC_ID_YUV4;
    case 206:
      return AV_CODEC_ID_AVRN;
    case 207:
      return AV_CODEC_ID_CPIA;
    case 208:
      return AV_CODEC_ID_XFACE;
    case 209:
      return AV_CODEC_ID_SNOW;
    case 210:
      return AV_CODEC_ID_SMVJPEG;
    case 211:
      return AV_CODEC_ID_APNG;
    case 212:
      return AV_CODEC_ID_DAALA;
    case 213:
      return AV_CODEC_ID_CFHD;
    case 214:
      return AV_CODEC_ID_TRUEMOTION2RT;
    case 215:
      return AV_CODEC_ID_M101;
    case 216:
      return AV_CODEC_ID_MAGICYUV;
    case 217:
      return AV_CODEC_ID_SHEERVIDEO;
    case 218:
      return AV_CODEC_ID_YLC;
    case 219:
      return AV_CODEC_ID_PSD;
    case 220:
      return AV_CODEC_ID_PIXLET;
    case 221:
      return AV_CODEC_ID_SPEEDHQ;
    case 222:
      return AV_CODEC_ID_FMVC;
    case 223:
      return AV_CODEC_ID_SCPR;
    case 224:
      return AV_CODEC_ID_CLEARVIDEO;
    case 225:
      return AV_CODEC_ID_XPM;
    case 226:
      return AV_CODEC_ID_AV1;
    case 227:
      return AV_CODEC_ID_BITPACKED;
    case 228:
      return AV_CODEC_ID_MSCC;
    case 229:
      return AV_CODEC_ID_SRGC;
    case 230:
      return AV_CODEC_ID_SVG;
    case 231:
      return AV_CODEC_ID_GDV;
    case 232:
      return AV_CODEC_ID_FITS;
    case 233:
      return AV_CODEC_ID_IMM4;
    case 234:
      return AV_CODEC_ID_PROSUMER;
    case 235:
      return AV_CODEC_ID_MWSC;
    case 236:
      return AV_CODEC_ID_WCMV;
    case 237:
      return AV_CODEC_ID_RASC;
    case 238:
      return AV_CODEC_ID_HYMT;
    case 239:
      return AV_CODEC_ID_ARBC;
    case 240:
      return AV_CODEC_ID_AGM;
    case 241:
      return AV_CODEC_ID_LSCR;
    case 242:
      return AV_CODEC_ID_VP4;
    case 243:
      return AV_CODEC_ID_IMM5;
    case 244:
      return AV_CODEC_ID_MVDV;
    case 245:
      return AV_CODEC_ID_MVHA;
    case 246:
      return AV_CODEC_ID_CDTOONS;
    case 247:
      return AV_CODEC_ID_MV30;
    case 248:
      return AV_CODEC_ID_NOTCHLC;
    case 249:
      return AV_CODEC_ID_PFM;
    case 250:
      return AV_CODEC_ID_MOBICLIP;
    case 251:
      return AV_CODEC_ID_PHOTOCD;
    case 252:
      return AV_CODEC_ID_IPU;
    case 253:
      return AV_CODEC_ID_ARGO;
    case 254:
      return AV_CODEC_ID_CRI;
    case 255:
      return AV_CODEC_ID_SIMBIOSIS_IMX;
    case 256:
      return AV_CODEC_ID_SGA_VIDEO;
    case 257:
      return AV_CODEC_ID_GEM;
    case 258:
      return AV_CODEC_ID_VBN;
    case 259:
      return AV_CODEC_ID_JPEGXL;
    case 260:
      return AV_CODEC_ID_QOI;
    case 261:
      return AV_CODEC_ID_PHM;
    case 262:
      return AV_CODEC_ID_RADIANCE_HDR;
    case 263:
      return AV_CODEC_ID_WBMP;
    case 264:
      return AV_CODEC_ID_MEDIA100;
    case 265:
      return AV_CODEC_ID_VQC;
    case 65536:
      return AV_CODEC_ID_PCM_S16LE;
    case 65537:
      return AV_CODEC_ID_PCM_S16BE;
    case 65538:
      return AV_CODEC_ID_PCM_U16LE;
    case 65539:
      return AV_CODEC_ID_PCM_U16BE;
    case 65540:
      return AV_CODEC_ID_PCM_S8;
    case 65541:
      return AV_CODEC_ID_PCM_U8;
    case 65542:
      return AV_CODEC_ID_PCM_MULAW;
    case 65543:
      return AV_CODEC_ID_PCM_ALAW;
    case 65544:
      return AV_CODEC_ID_PCM_S32LE;
    case 65545:
      return AV_CODEC_ID_PCM_S32BE;
    case 65546:
      return AV_CODEC_ID_PCM_U32LE;
    case 65547:
      return AV_CODEC_ID_PCM_U32BE;
    case 65548:
      return AV_CODEC_ID_PCM_S24LE;
    case 65549:
      return AV_CODEC_ID_PCM_S24BE;
    case 65550:
      return AV_CODEC_ID_PCM_U24LE;
    case 65551:
      return AV_CODEC_ID_PCM_U24BE;
    case 65552:
      return AV_CODEC_ID_PCM_S24DAUD;
    case 65553:
      return AV_CODEC_ID_PCM_ZORK;
    case 65554:
      return AV_CODEC_ID_PCM_S16LE_PLANAR;
    case 65555:
      return AV_CODEC_ID_PCM_DVD;
    case 65556:
      return AV_CODEC_ID_PCM_F32BE;
    case 65557:
      return AV_CODEC_ID_PCM_F32LE;
    case 65558:
      return AV_CODEC_ID_PCM_F64BE;
    case 65559:
      return AV_CODEC_ID_PCM_F64LE;
    case 65560:
      return AV_CODEC_ID_PCM_BLURAY;
    case 65561:
      return AV_CODEC_ID_PCM_LXF;
    case 65562:
      return AV_CODEC_ID_S302M;
    case 65563:
      return AV_CODEC_ID_PCM_S8_PLANAR;
    case 65564:
      return AV_CODEC_ID_PCM_S24LE_PLANAR;
    case 65565:
      return AV_CODEC_ID_PCM_S32LE_PLANAR;
    case 65566:
      return AV_CODEC_ID_PCM_S16BE_PLANAR;
    case 65567:
      return AV_CODEC_ID_PCM_S64LE;
    case 65568:
      return AV_CODEC_ID_PCM_S64BE;
    case 65569:
      return AV_CODEC_ID_PCM_F16LE;
    case 65570:
      return AV_CODEC_ID_PCM_F24LE;
    case 65571:
      return AV_CODEC_ID_PCM_VIDC;
    case 65572:
      return AV_CODEC_ID_PCM_SGA;
    case 69632:
      return AV_CODEC_ID_ADPCM_IMA_QT;
    case 69633:
      return AV_CODEC_ID_ADPCM_IMA_WAV;
    case 69634:
      return AV_CODEC_ID_ADPCM_IMA_DK3;
    case 69635:
      return AV_CODEC_ID_ADPCM_IMA_DK4;
    case 69636:
      return AV_CODEC_ID_ADPCM_IMA_WS;
    case 69637:
      return AV_CODEC_ID_ADPCM_IMA_SMJPEG;
    case 69638:
      return AV_CODEC_ID_ADPCM_MS;
    case 69639:
      return AV_CODEC_ID_ADPCM_4XM;
    case 69640:
      return AV_CODEC_ID_ADPCM_XA;
    case 69641:
      return AV_CODEC_ID_ADPCM_ADX;
    case 69642:
      return AV_CODEC_ID_ADPCM_EA;
    case 69643:
      return AV_CODEC_ID_ADPCM_G726;
    case 69644:
      return AV_CODEC_ID_ADPCM_CT;
    case 69645:
      return AV_CODEC_ID_ADPCM_SWF;
    case 69646:
      return AV_CODEC_ID_ADPCM_YAMAHA;
    case 69647:
      return AV_CODEC_ID_ADPCM_SBPRO_4;
    case 69648:
      return AV_CODEC_ID_ADPCM_SBPRO_3;
    case 69649:
      return AV_CODEC_ID_ADPCM_SBPRO_2;
    case 69650:
      return AV_CODEC_ID_ADPCM_THP;
    case 69651:
      return AV_CODEC_ID_ADPCM_IMA_AMV;
    case 69652:
      return AV_CODEC_ID_ADPCM_EA_R1;
    case 69653:
      return AV_CODEC_ID_ADPCM_EA_R3;
    case 69654:
      return AV_CODEC_ID_ADPCM_EA_R2;
    case 69655:
      return AV_CODEC_ID_ADPCM_IMA_EA_SEAD;
    case 69656:
      return AV_CODEC_ID_ADPCM_IMA_EA_EACS;
    case 69657:
      return AV_CODEC_ID_ADPCM_EA_XAS;
    case 69658:
      return AV_CODEC_ID_ADPCM_EA_MAXIS_XA;
    case 69659:
      return AV_CODEC_ID_ADPCM_IMA_ISS;
    case 69660:
      return AV_CODEC_ID_ADPCM_G722;
    case 69661:
      return AV_CODEC_ID_ADPCM_IMA_APC;
    case 69662:
      return AV_CODEC_ID_ADPCM_VIMA;
    case 69663:
      return AV_CODEC_ID_ADPCM_AFC;
    case 69664:
      return AV_CODEC_ID_ADPCM_IMA_OKI;
    case 69665:
      return AV_CODEC_ID_ADPCM_DTK;
    case 69666:
      return AV_CODEC_ID_ADPCM_IMA_RAD;
    case 69667:
      return AV_CODEC_ID_ADPCM_G726LE;
    case 69668:
      return AV_CODEC_ID_ADPCM_THP_LE;
    case 69669:
      return AV_CODEC_ID_ADPCM_PSX;
    case 69670:
      return AV_CODEC_ID_ADPCM_AICA;
    case 69671:
      return AV_CODEC_ID_ADPCM_IMA_DAT4;
    case 69672:
      return AV_CODEC_ID_ADPCM_MTAF;
    case 69673:
      return AV_CODEC_ID_ADPCM_AGM;
    case 69674:
      return AV_CODEC_ID_ADPCM_ARGO;
    case 69675:
      return AV_CODEC_ID_ADPCM_IMA_SSI;
    case 69676:
      return AV_CODEC_ID_ADPCM_ZORK;
    case 69677:
      return AV_CODEC_ID_ADPCM_IMA_APM;
    case 69678:
      return AV_CODEC_ID_ADPCM_IMA_ALP;
    case 69679:
      return AV_CODEC_ID_ADPCM_IMA_MTF;
    case 69680:
      return AV_CODEC_ID_ADPCM_IMA_CUNNING;
    case 69681:
      return AV_CODEC_ID_ADPCM_IMA_MOFLEX;
    case 69682:
      return AV_CODEC_ID_ADPCM_IMA_ACORN;
    case 69683:
      return AV_CODEC_ID_ADPCM_XMD;
    case 73828:
      return AV_CODEC_ID_AMR_NB;
    case 73729:
      return AV_CODEC_ID_AMR_WB;
    case 77824:
      return AV_CODEC_ID_RA_144;
    case 77825:
      return AV_CODEC_ID_RA_288;
    case 81920:
      return AV_CODEC_ID_ROQ_DPCM;
    case 81921:
      return AV_CODEC_ID_INTERPLAY_DPCM;
    case 81922:
      return AV_CODEC_ID_XAN_DPCM;
    case 81923:
      return AV_CODEC_ID_SOL_DPCM;
    case 81924:
      return AV_CODEC_ID_SDX2_DPCM;
    case 81925:
      return AV_CODEC_ID_GREMLIN_DPCM;
    case 81926:
      return AV_CODEC_ID_DERF_DPCM;
    case 81927:
      return AV_CODEC_ID_WADY_DPCM;
    case 81929:
      return AV_CODEC_ID_CBD2_DPCM;
    case 86016:
      return AV_CODEC_ID_MP2;
    case 86017:
      return AV_CODEC_ID_MP3;
    case 86018:
      return AV_CODEC_ID_AAC;
    case 86019:
      return AV_CODEC_ID_AC3;
    case 86020:
      return AV_CODEC_ID_DTS;
    case 86021:
      return AV_CODEC_ID_VORBIS;
    case 86022:
      return AV_CODEC_ID_DVAUDIO;
    case 86023:
      return AV_CODEC_ID_WMAV1;
    case 86024:
      return AV_CODEC_ID_WMAV2;
    case 86025:
      return AV_CODEC_ID_MACE3;
    case 86026:
      return AV_CODEC_ID_MACE6;
    case 86027:
      return AV_CODEC_ID_VMDAUDIO;
    case 86028:
      return AV_CODEC_ID_FLAC;
    case 86029:
      return AV_CODEC_ID_MP3ADU;
    case 86030:
      return AV_CODEC_ID_MP3ON4;
    case 86031:
      return AV_CODEC_ID_SHORTEN;
    case 86032:
      return AV_CODEC_ID_ALAC;
    case 86033:
      return AV_CODEC_ID_WESTWOOD_SND1;
    case 86034:
      return AV_CODEC_ID_GSM;
    case 86035:
      return AV_CODEC_ID_QDM2;
    case 86036:
      return AV_CODEC_ID_COOK;
    case 86037:
      return AV_CODEC_ID_TRUESPEECH;
    case 86038:
      return AV_CODEC_ID_TTA;
    case 86039:
      return AV_CODEC_ID_SMACKAUDIO;
    case 86040:
      return AV_CODEC_ID_QCELP;
    case 86041:
      return AV_CODEC_ID_WAVPACK;
    case 86042:
      return AV_CODEC_ID_DSICINAUDIO;
    case 86043:
      return AV_CODEC_ID_IMC;
    case 86044:
      return AV_CODEC_ID_MUSEPACK7;
    case 86045:
      return AV_CODEC_ID_MLP;
    case 86046:
      return AV_CODEC_ID_GSM_MS;
    case 86047:
      return AV_CODEC_ID_ATRAC3;
    case 86048:
      return AV_CODEC_ID_APE;
    case 86049:
      return AV_CODEC_ID_NELLYMOSER;
    case 86050:
      return AV_CODEC_ID_MUSEPACK8;
    case 86051:
      return AV_CODEC_ID_SPEEX;
    case 86052:
      return AV_CODEC_ID_WMAVOICE;
    case 86053:
      return AV_CODEC_ID_WMAPRO;
    case 86054:
      return AV_CODEC_ID_WMALOSSLESS;
    case 86055:
      return AV_CODEC_ID_ATRAC3P;
    case 86056:
      return AV_CODEC_ID_EAC3;
    case 86057:
      return AV_CODEC_ID_SIPR;
    case 86058:
      return AV_CODEC_ID_MP1;
    case 86059:
      return AV_CODEC_ID_TWINVQ;
    case 86060:
      return AV_CODEC_ID_TRUEHD;
    case 86061:
      return AV_CODEC_ID_MP4ALS;
    case 86062:
      return AV_CODEC_ID_ATRAC1;
    case 86063:
      return AV_CODEC_ID_BINKAUDIO_RDFT;
    case 86064:
      return AV_CODEC_ID_BINKAUDIO_DCT;
    case 86065:
      return AV_CODEC_ID_AAC_LATM;
    case 86066:
      return AV_CODEC_ID_QDMC;
    case 86067:
      return AV_CODEC_ID_CELT;
    case 86068:
      return AV_CODEC_ID_G723_1;
    case 86069:
      return AV_CODEC_ID_G729;
    case 86070:
      return AV_CODEC_ID_8SVX_EXP;
    case 86071:
      return AV_CODEC_ID_8SVX_FIB;
    case 86072:
      return AV_CODEC_ID_BMV_AUDIO;
    case 86073:
      return AV_CODEC_ID_RALF;
    case 86074:
      return AV_CODEC_ID_IAC;
    case 86075:
      return AV_CODEC_ID_ILBC;
    case 86076:
      return AV_CODEC_ID_OPUS;
    case 86077:
      return AV_CODEC_ID_COMFORT_NOISE;
    case 86078:
      return AV_CODEC_ID_TAK;
    case 86079:
      return AV_CODEC_ID_METASOUND;
    case 86080:
      return AV_CODEC_ID_PAF_AUDIO;
    case 86081:
      return AV_CODEC_ID_ON2AVC;
    case 86082:
      return AV_CODEC_ID_DSS_SP;
    case 86083:
      return AV_CODEC_ID_CODEC2;
    case 86084:
      return AV_CODEC_ID_FFWAVESYNTH;
    case 86085:
      return AV_CODEC_ID_SONIC;
    case 86086:
      return AV_CODEC_ID_SONIC_LS;
    case 86087:
      return AV_CODEC_ID_EVRC;
    case 86088:
      return AV_CODEC_ID_SMV;
    case 86089:
      return AV_CODEC_ID_DSD_LSBF;
    case 86090:
      return AV_CODEC_ID_DSD_MSBF;
    case 86091:
      return AV_CODEC_ID_DSD_LSBF_PLANAR;
    case 86092:
      return AV_CODEC_ID_DSD_MSBF_PLANAR;
    case 86093:
      return AV_CODEC_ID_4GV;
    case 86094:
      return AV_CODEC_ID_INTERPLAY_ACM;
    case 86095:
      return AV_CODEC_ID_XMA1;
    case 86096:
      return AV_CODEC_ID_XMA2;
    case 86097:
      return AV_CODEC_ID_DST;
    case 86098:
      return AV_CODEC_ID_ATRAC3AL;
    case 86099:
      return AV_CODEC_ID_ATRAC3PAL;
    case 86100:
      return AV_CODEC_ID_DOLBY_E;
    case 86101:
      return AV_CODEC_ID_APTX;
    case 86102:
      return AV_CODEC_ID_APTX_HD;
    case 86103:
      return AV_CODEC_ID_SBC;
    case 86104:
      return AV_CODEC_ID_ATRAC9;
    case 86105:
      return AV_CODEC_ID_HCOM;
    case 86106:
      return AV_CODEC_ID_ACELP_KELVIN;
    case 86107:
      return AV_CODEC_ID_MPEGH_3D_AUDIO;
    case 86108:
      return AV_CODEC_ID_SIREN;
    case 86109:
      return AV_CODEC_ID_HCA;
    case 86110:
      return AV_CODEC_ID_FASTAUDIO;
    case 86111:
      return AV_CODEC_ID_MSNSIREN;
    case 86112:
      return AV_CODEC_ID_DFPWM;
    case 86113:
      return AV_CODEC_ID_BONK;
    case 86114:
      return AV_CODEC_ID_MISC4;
    case 86115:
      return AV_CODEC_ID_APAC;
    case 86116:
      return AV_CODEC_ID_FTR;
    case 86117:
      return AV_CODEC_ID_WAVARC;
    case 86118:
      return AV_CODEC_ID_RKA;
    case 94208:
      return AV_CODEC_ID_DVD_SUBTITLE;
    case 94209:
      return AV_CODEC_ID_DVB_SUBTITLE;
    case 94210:
      return AV_CODEC_ID_TEXT;
    case 94211:
      return AV_CODEC_ID_XSUB;
    case 94212:
      return AV_CODEC_ID_SSA;
    case 94213:
      return AV_CODEC_ID_MOV_TEXT;
    case 94214:
      return AV_CODEC_ID_HDMV_PGS_SUBTITLE;
    case 94215:
      return AV_CODEC_ID_DVB_TELETEXT;
    case 94216:
      return AV_CODEC_ID_SRT;
    case 94217:
      return AV_CODEC_ID_MICRODVD;
    case 94218:
      return AV_CODEC_ID_EIA_608;
    case 94219:
      return AV_CODEC_ID_JACOSUB;
    case 94220:
      return AV_CODEC_ID_SAMI;
    case 94221:
      return AV_CODEC_ID_REALTEXT;
    case 94222:
      return AV_CODEC_ID_STL;
    case 94223:
      return AV_CODEC_ID_SUBVIEWER1;
    case 94224:
      return AV_CODEC_ID_SUBVIEWER;
    case 94225:
      return AV_CODEC_ID_SUBRIP;
    case 94226:
      return AV_CODEC_ID_WEBVTT;
    case 94227:
      return AV_CODEC_ID_MPL2;
    case 94228:
      return AV_CODEC_ID_VPLAYER;
    case 94229:
      return AV_CODEC_ID_PJS;
    case 94230:
      return AV_CODEC_ID_ASS;
    case 94231:
      return AV_CODEC_ID_HDMV_TEXT_SUBTITLE;
    case 94232:
      return AV_CODEC_ID_TTML;
    case 94233:
      return AV_CODEC_ID_ARIB_CAPTION;
    case 98304:
      return AV_CODEC_ID_TTF;
    case 98305:
      return AV_CODEC_ID_SCTE_35;
    case 98306:
      return AV_CODEC_ID_EPG;
    case 98307:
      return AV_CODEC_ID_BINTEXT;
    case 98308:
      return AV_CODEC_ID_XBIN;
    case 98309:
      return AV_CODEC_ID_IDF;
    case 98310:
      return AV_CODEC_ID_OTF;
    case 98311:
      return AV_CODEC_ID_SMPTE_KLV;
    case 98312:
      return AV_CODEC_ID_DVD_NAV;
    case 98313:
      return AV_CODEC_ID_TIMED_ID3;
    case 98314:
      return AV_CODEC_ID_BIN_DATA;
    case 102400:
      return AV_CODEC_ID_PROBE;
    case 131072:
      return AV_CODEC_ID_MPEG2TS;
    case 131073:
      return AV_CODEC_ID_MPEG4SYSTEMS;
    case 135168:
      return AV_CODEC_ID_FFMETADATA;
    case 135169:
      return AV_CODEC_ID_WRAPPED_AVFRAME;
    case 135170:
      return AV_CODEC_ID_VNULL;
    case 135171:
      return AV_CODEC_ID_ANULL;
      // Returning NONE as default.
    default:
      return AV_CODEC_ID_NONE;
    };
  }

  // Convert AVCodecID to uint32_t for rust SDK.
  static uint32_t fromAVCodecID(AVCodecID avCodecId){
    switch (avCodecId) {
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
      case AV_CODEC_ID_KGV1:
        return 137;
      case AV_CODEC_ID_YOP:
        return 138;
      case AV_CODEC_ID_VP8:
        return 139;
      case AV_CODEC_ID_PICTOR:
        return 140;
      case AV_CODEC_ID_ANSI:
        return 141;
      case AV_CODEC_ID_A64_MULTI:
        return 142;
      case AV_CODEC_ID_A64_MULTI5:
        return 143;
      case AV_CODEC_ID_R10K:
        return 144;
      case AV_CODEC_ID_MXPEG:
        return 145;
      case AV_CODEC_ID_LAGARITH:
        return 146;
      case AV_CODEC_ID_PRORES:
        return 147;
      case AV_CODEC_ID_JV:
        return 148;
      case AV_CODEC_ID_DFA:
        return 149;
      case AV_CODEC_ID_WMV3IMAGE:
        return 150;
      case AV_CODEC_ID_VC1IMAGE:
        return 151;
      case AV_CODEC_ID_UTVIDEO:
        return 152;
      case AV_CODEC_ID_BMV_VIDEO:
        return 153;
      case AV_CODEC_ID_VBLE:
        return 154;
      case AV_CODEC_ID_DXTORY:
        return 155;
      case AV_CODEC_ID_V410:
        return 156;
      case AV_CODEC_ID_XWD:
        return 157;
      case AV_CODEC_ID_CDXL:
        return 158;
      case AV_CODEC_ID_XBM:
        return 159;
      case AV_CODEC_ID_ZEROCODEC:
        return 160;
      case AV_CODEC_ID_MSS1:
        return 161;
      case AV_CODEC_ID_MSA1:
        return 162;
      case AV_CODEC_ID_TSCC2:
        return 163;
      case AV_CODEC_ID_MTS2:
        return 164;
      case AV_CODEC_ID_CLLC:
        return 165;
      case AV_CODEC_ID_MSS2:
        return 166;
      case AV_CODEC_ID_VP9:
        return 167;
      case AV_CODEC_ID_AIC:
        return 168;
      case AV_CODEC_ID_ESCAPE130:
        return 169;
      case AV_CODEC_ID_G2M:
        return 170;
      case AV_CODEC_ID_WEBP:
        return 171;
      case AV_CODEC_ID_HNM4_VIDEO:
        return 172;
      case AV_CODEC_ID_HEVC:
        return 173;
      case AV_CODEC_ID_FIC:
        return 174;
      case AV_CODEC_ID_ALIAS_PIX:
        return 175;
      case AV_CODEC_ID_BRENDER_PIX:
        return 176;
      case AV_CODEC_ID_PAF_VIDEO:
        return 177;
      case AV_CODEC_ID_EXR:
        return 178;
      case AV_CODEC_ID_VP7:
        return 179;
      case AV_CODEC_ID_SANM:
        return 180;
      case AV_CODEC_ID_SGIRLE:
        return 181;
      case AV_CODEC_ID_MVC1:
        return 182;
      case AV_CODEC_ID_MVC2:
        return 183;
      case AV_CODEC_ID_HQX:
        return 184;
      case AV_CODEC_ID_TDSC:
        return 185;
      case AV_CODEC_ID_HQ_HQA:
        return 186;
      case AV_CODEC_ID_HAP:
        return 187;
      case AV_CODEC_ID_DDS:
        return 188;
      case AV_CODEC_ID_DXV:
        return 189;
      case AV_CODEC_ID_SCREENPRESSO:
        return 190;
      case AV_CODEC_ID_RSCC:
        return 191;
      case AV_CODEC_ID_AVS2:
        return 192;
      case AV_CODEC_ID_PGX:
        return 193;
      case AV_CODEC_ID_AVS3:
        return 194;
      case AV_CODEC_ID_MSP2:
        return 195;
      case AV_CODEC_ID_VVC:
        return 196;
      case AV_CODEC_ID_Y41P:
        return 197;
      case AV_CODEC_ID_AVRP:
        return 198;
      case AV_CODEC_ID_012V:
        return 199;
      case AV_CODEC_ID_AVUI:
        return 200;
      case AV_CODEC_ID_AYUV:
        return 201;
      case AV_CODEC_ID_TARGA_Y216:
        return 202;
      case AV_CODEC_ID_V308:
        return 203;
      case AV_CODEC_ID_V408:
        return 204;
      case AV_CODEC_ID_YUV4:
        return 205;
      case AV_CODEC_ID_AVRN:
        return 206;
      case AV_CODEC_ID_CPIA:
        return 207;
      case AV_CODEC_ID_XFACE:
        return 208;
      case AV_CODEC_ID_SNOW:
        return 209;
      case AV_CODEC_ID_SMVJPEG:
        return 210;
      case AV_CODEC_ID_APNG:
        return 211;
      case AV_CODEC_ID_DAALA:
        return 212;
      case AV_CODEC_ID_CFHD:
        return 213;
      case AV_CODEC_ID_TRUEMOTION2RT:
        return 214;
      case AV_CODEC_ID_M101:
        return 215;
      case AV_CODEC_ID_MAGICYUV:
        return 216;
      case AV_CODEC_ID_SHEERVIDEO:
        return 217;
      case AV_CODEC_ID_YLC:
        return 218;
      case AV_CODEC_ID_PSD:
        return 219;
      case AV_CODEC_ID_PIXLET:
        return 220;
      case AV_CODEC_ID_SPEEDHQ:
        return 221;
      case AV_CODEC_ID_FMVC:
        return 222;
      case AV_CODEC_ID_SCPR:
        return 223;
      case AV_CODEC_ID_CLEARVIDEO:
        return 224;
      case AV_CODEC_ID_XPM:
        return 225;
      case AV_CODEC_ID_AV1:
        return 226;
      case AV_CODEC_ID_BITPACKED:
        return 227;
      case AV_CODEC_ID_MSCC:
        return 228;
      case AV_CODEC_ID_SRGC:
        return 229;
      case AV_CODEC_ID_SVG:
        return 230;
      case AV_CODEC_ID_GDV:
        return 231;
      case AV_CODEC_ID_FITS:
        return 232;
      case AV_CODEC_ID_IMM4:
        return 233;
      case AV_CODEC_ID_PROSUMER:
        return 234;
      case AV_CODEC_ID_MWSC:
        return 235;
      case AV_CODEC_ID_WCMV:
        return 236;
      case AV_CODEC_ID_RASC:
        return 237;
      case AV_CODEC_ID_HYMT:
        return 238;
      case AV_CODEC_ID_ARBC:
        return 239;
      case AV_CODEC_ID_AGM:
        return 240;
      case AV_CODEC_ID_LSCR:
        return 241;
      case AV_CODEC_ID_VP4:
        return 242;
      case AV_CODEC_ID_IMM5:
        return 243;
      case AV_CODEC_ID_MVDV:
        return 244;
      case AV_CODEC_ID_MVHA:
        return 245;
      case AV_CODEC_ID_CDTOONS:
        return 246;
      case AV_CODEC_ID_MV30:
        return 247;
      case AV_CODEC_ID_NOTCHLC:
        return 248;
      case AV_CODEC_ID_PFM:
        return 249;
      case AV_CODEC_ID_MOBICLIP:
        return 250;
      case AV_CODEC_ID_PHOTOCD:
        return 251;
      case AV_CODEC_ID_IPU:
        return 252;
      case AV_CODEC_ID_ARGO:
        return 253;
      case AV_CODEC_ID_CRI:
        return 254;
      case AV_CODEC_ID_SIMBIOSIS_IMX:
        return 255;
      case AV_CODEC_ID_SGA_VIDEO:
        return 256;
      case AV_CODEC_ID_GEM:
        return 257;
      case AV_CODEC_ID_VBN:
        return 258;
      case AV_CODEC_ID_JPEGXL:
        return 259;
      case AV_CODEC_ID_QOI:
        return 260;
      case AV_CODEC_ID_PHM:
        return 261;
      case AV_CODEC_ID_RADIANCE_HDR:
        return 262;
      case AV_CODEC_ID_WBMP:
        return 263;
      case AV_CODEC_ID_MEDIA100:
        return 264;
      case AV_CODEC_ID_VQC:
        return 265;
      case AV_CODEC_ID_PCM_S16LE:
        return 65536;
      case AV_CODEC_ID_PCM_S16BE:
        return 65537;
      case AV_CODEC_ID_PCM_U16LE:
        return 65538;
      case AV_CODEC_ID_PCM_U16BE:
        return 65539;
      case AV_CODEC_ID_PCM_S8:
        return 65540;
      case AV_CODEC_ID_PCM_U8:
        return 65541;
      case AV_CODEC_ID_PCM_MULAW:
        return 65542;
      case AV_CODEC_ID_PCM_ALAW:
        return 65543;
      case AV_CODEC_ID_PCM_S32LE:
        return 65544;
      case AV_CODEC_ID_PCM_S32BE:
        return 65545;
      case AV_CODEC_ID_PCM_U32LE:
        return 65546;
      case AV_CODEC_ID_PCM_U32BE:
        return 65547;
      case AV_CODEC_ID_PCM_S24LE:
        return 65548;
      case AV_CODEC_ID_PCM_S24BE:
        return 65549;
      case AV_CODEC_ID_PCM_U24LE:
        return 65550;
      case AV_CODEC_ID_PCM_U24BE:
        return 65551;
      case AV_CODEC_ID_PCM_S24DAUD:
        return 65552;
      case AV_CODEC_ID_PCM_ZORK:
        return 65553;
      case AV_CODEC_ID_PCM_S16LE_PLANAR:
        return 65554;
      case AV_CODEC_ID_PCM_DVD:
        return 65555;
      case AV_CODEC_ID_PCM_F32BE:
        return 65556;
      case AV_CODEC_ID_PCM_F32LE:
        return 65557;
      case AV_CODEC_ID_PCM_F64BE:
        return 65558;
      case AV_CODEC_ID_PCM_F64LE:
        return 65559;
      case AV_CODEC_ID_PCM_BLURAY:
        return 65560;
      case AV_CODEC_ID_PCM_LXF:
        return 65561;
      case AV_CODEC_ID_S302M:
        return 65562;
      case AV_CODEC_ID_PCM_S8_PLANAR:
        return 65563;
      case AV_CODEC_ID_PCM_S24LE_PLANAR:
        return 65564;
      case AV_CODEC_ID_PCM_S32LE_PLANAR:
        return 65565;
      case AV_CODEC_ID_PCM_S16BE_PLANAR:
        return 65566;
      case AV_CODEC_ID_PCM_S64LE:
        return 65567;
      case AV_CODEC_ID_PCM_S64BE:
        return 65568;
      case AV_CODEC_ID_PCM_F16LE:
        return 65569;
      case AV_CODEC_ID_PCM_F24LE:
        return 65570;
      case AV_CODEC_ID_PCM_VIDC:
        return 65571;
      case AV_CODEC_ID_PCM_SGA:
        return 65572;
      case AV_CODEC_ID_ADPCM_IMA_QT:
        return 69632;
      case AV_CODEC_ID_ADPCM_IMA_WAV:
        return 69633;
      case AV_CODEC_ID_ADPCM_IMA_DK3:
        return 69634;
      case AV_CODEC_ID_ADPCM_IMA_DK4:
        return 69635;
      case AV_CODEC_ID_ADPCM_IMA_WS:
        return 69636;
      case AV_CODEC_ID_ADPCM_IMA_SMJPEG:
        return 69637;
      case AV_CODEC_ID_ADPCM_MS:
        return 69638;
      case AV_CODEC_ID_ADPCM_4XM:
        return 69639;
      case AV_CODEC_ID_ADPCM_XA:
        return 69640;
      case AV_CODEC_ID_ADPCM_ADX:
        return 69641;
      case AV_CODEC_ID_ADPCM_EA:
        return 69642;
      case AV_CODEC_ID_ADPCM_G726:
        return 69643;
      case AV_CODEC_ID_ADPCM_CT:
        return 69644;
      case AV_CODEC_ID_ADPCM_SWF:
        return 69645;
      case AV_CODEC_ID_ADPCM_YAMAHA:
        return 69646;
      case AV_CODEC_ID_ADPCM_SBPRO_4:
        return 69647;
      case AV_CODEC_ID_ADPCM_SBPRO_3:
        return 69648;
      case AV_CODEC_ID_ADPCM_SBPRO_2:
        return 69649;
      case AV_CODEC_ID_ADPCM_THP:
        return 69650;
      case AV_CODEC_ID_ADPCM_IMA_AMV:
        return 69651;
      case AV_CODEC_ID_ADPCM_EA_R1:
        return 69652;
      case AV_CODEC_ID_ADPCM_EA_R3:
        return 69653;
      case AV_CODEC_ID_ADPCM_EA_R2:
        return 69654;
      case AV_CODEC_ID_ADPCM_IMA_EA_SEAD:
        return 69655;
      case AV_CODEC_ID_ADPCM_IMA_EA_EACS:
        return 69656;
      case AV_CODEC_ID_ADPCM_EA_XAS:
        return 69657;
      case AV_CODEC_ID_ADPCM_EA_MAXIS_XA:
        return 69658;
      case AV_CODEC_ID_ADPCM_IMA_ISS:
        return 69659;
      case AV_CODEC_ID_ADPCM_G722:
        return 69660;
      case AV_CODEC_ID_ADPCM_IMA_APC:
        return 69661;
      case AV_CODEC_ID_ADPCM_VIMA:
        return 69662;
      case AV_CODEC_ID_ADPCM_AFC:
        return 69663;
      case AV_CODEC_ID_ADPCM_IMA_OKI:
        return 69664;
      case AV_CODEC_ID_ADPCM_DTK:
        return 69665;
      case AV_CODEC_ID_ADPCM_IMA_RAD:
        return 69666;
      case AV_CODEC_ID_ADPCM_G726LE:
        return 69667;
      case AV_CODEC_ID_ADPCM_THP_LE:
        return 69668;
      case AV_CODEC_ID_ADPCM_PSX:
        return 69669;
      case AV_CODEC_ID_ADPCM_AICA:
        return 69670;
      case AV_CODEC_ID_ADPCM_IMA_DAT4:
        return 69671;
      case AV_CODEC_ID_ADPCM_MTAF:
        return 69672;
      case AV_CODEC_ID_ADPCM_AGM:
        return 69673;
      case AV_CODEC_ID_ADPCM_ARGO:
        return 69674;
      case AV_CODEC_ID_ADPCM_IMA_SSI:
        return 69675;
      case AV_CODEC_ID_ADPCM_ZORK:
        return 69676;
      case AV_CODEC_ID_ADPCM_IMA_APM:
        return 69677;
      case AV_CODEC_ID_ADPCM_IMA_ALP:
        return 69678;
      case AV_CODEC_ID_ADPCM_IMA_MTF:
        return 69679;
      case AV_CODEC_ID_ADPCM_IMA_CUNNING:
        return 69680;
      case AV_CODEC_ID_ADPCM_IMA_MOFLEX:
        return 69681;
      case AV_CODEC_ID_ADPCM_IMA_ACORN:
        return 69682;
      case AV_CODEC_ID_ADPCM_XMD:
        return 69683;
      case AV_CODEC_ID_AMR_NB:
        return 73828;
      case AV_CODEC_ID_AMR_WB:
        return 73729;
      case AV_CODEC_ID_RA_144:
        return 77824;
      case AV_CODEC_ID_RA_288:
        return 77825;
      case AV_CODEC_ID_ROQ_DPCM:
        return 81920;
      case AV_CODEC_ID_INTERPLAY_DPCM:
        return 81921;
      case AV_CODEC_ID_XAN_DPCM:
        return 81922;
      case AV_CODEC_ID_SOL_DPCM:
        return 81923;
      case AV_CODEC_ID_SDX2_DPCM:
        return 81924;
      case AV_CODEC_ID_GREMLIN_DPCM:
        return 81925;
      case AV_CODEC_ID_DERF_DPCM:
        return 81926;
      case AV_CODEC_ID_WADY_DPCM:
        return 81927;
      case AV_CODEC_ID_CBD2_DPCM:
        return 81929;
      case AV_CODEC_ID_MP2:
        return 86016;
      case AV_CODEC_ID_MP3:
        return 86017;
      case AV_CODEC_ID_AAC:
        return 86018;
      case AV_CODEC_ID_AC3:
        return 86019;
      case AV_CODEC_ID_DTS:
        return 86020;
      case AV_CODEC_ID_VORBIS:
        return 86021;
      case AV_CODEC_ID_DVAUDIO:
        return 86022;
      case AV_CODEC_ID_WMAV1:
        return 86023;
      case AV_CODEC_ID_WMAV2:
        return 86024;
      case AV_CODEC_ID_MACE3:
        return 86025;
      case AV_CODEC_ID_MACE6:
        return 86026;
      case AV_CODEC_ID_VMDAUDIO:
        return 86027;
      case AV_CODEC_ID_FLAC:
        return 86028;
      case AV_CODEC_ID_MP3ADU:
        return 86029;
      case AV_CODEC_ID_MP3ON4:
        return 86030;
      case AV_CODEC_ID_SHORTEN:
        return 86031;
      case AV_CODEC_ID_ALAC:
        return 86032;
      case AV_CODEC_ID_WESTWOOD_SND1:
        return 86033;
      case AV_CODEC_ID_GSM:
        return 86034;
      case AV_CODEC_ID_QDM2:
        return 86035;
      case AV_CODEC_ID_COOK:
        return 86036;
      case AV_CODEC_ID_TRUESPEECH:
        return 86037;
      case AV_CODEC_ID_TTA:
        return 86038;
      case AV_CODEC_ID_SMACKAUDIO:
        return 86039;
      case AV_CODEC_ID_QCELP:
        return 86040;
      case AV_CODEC_ID_WAVPACK:
        return 86041;
      case AV_CODEC_ID_DSICINAUDIO:
        return 86042;
      case AV_CODEC_ID_IMC:
        return 86043;
      case AV_CODEC_ID_MUSEPACK7:
        return 86044;
      case AV_CODEC_ID_MLP:
        return 86045;
      case AV_CODEC_ID_GSM_MS:
        return 86046;
      case AV_CODEC_ID_ATRAC3:
        return 86047;
      case AV_CODEC_ID_APE:
        return 86048;
      case AV_CODEC_ID_NELLYMOSER:
        return 86049;
      case AV_CODEC_ID_MUSEPACK8:
        return 86050;
      case AV_CODEC_ID_SPEEX:
        return 86051;
      case AV_CODEC_ID_WMAVOICE:
        return 86052;
      case AV_CODEC_ID_WMAPRO:
        return 86053;
      case AV_CODEC_ID_WMALOSSLESS:
        return 86054;
      case AV_CODEC_ID_ATRAC3P:
        return 86055;
      case AV_CODEC_ID_EAC3:
        return 86056;
      case AV_CODEC_ID_SIPR:
        return 86057;
      case AV_CODEC_ID_MP1:
        return 86058;
      case AV_CODEC_ID_TWINVQ:
        return 86059;
      case AV_CODEC_ID_TRUEHD:
        return 86060;
      case AV_CODEC_ID_MP4ALS:
        return 86061;
      case AV_CODEC_ID_ATRAC1:
        return 86062;
      case AV_CODEC_ID_BINKAUDIO_RDFT:
        return 86063;
      case AV_CODEC_ID_BINKAUDIO_DCT:
        return 86064;
      case AV_CODEC_ID_AAC_LATM:
        return 86065;
      case AV_CODEC_ID_QDMC:
        return 86066;
      case AV_CODEC_ID_CELT:
        return 86067;
      case AV_CODEC_ID_G723_1:
        return 86068;
      case AV_CODEC_ID_G729:
        return 86069;
      case AV_CODEC_ID_8SVX_EXP:
        return 86070;
      case AV_CODEC_ID_8SVX_FIB:
        return 86071;
      case AV_CODEC_ID_BMV_AUDIO:
        return 86072;
      case AV_CODEC_ID_RALF:
        return 86073;
      case AV_CODEC_ID_IAC:
        return 86074;
      case AV_CODEC_ID_ILBC:
        return 86075;
      case AV_CODEC_ID_OPUS:
        return 86076;
      case AV_CODEC_ID_COMFORT_NOISE:
        return 86077;
      case AV_CODEC_ID_TAK:
        return 86078;
      case AV_CODEC_ID_METASOUND:
        return 86079;
      case AV_CODEC_ID_PAF_AUDIO:
        return 86080;
      case AV_CODEC_ID_ON2AVC:
        return 86081;
      case AV_CODEC_ID_DSS_SP:
        return 86082;
      case AV_CODEC_ID_CODEC2:
        return 86083;
      case AV_CODEC_ID_FFWAVESYNTH:
        return 86084;
      case AV_CODEC_ID_SONIC:
        return 86085;
      case AV_CODEC_ID_SONIC_LS:
        return 86086;
      case AV_CODEC_ID_EVRC:
        return 86087;
      case AV_CODEC_ID_SMV:
        return 86088;
      case AV_CODEC_ID_DSD_LSBF:
        return 86089;
      case AV_CODEC_ID_DSD_MSBF:
        return 86090;
      case AV_CODEC_ID_DSD_LSBF_PLANAR:
        return 86091;
      case AV_CODEC_ID_DSD_MSBF_PLANAR:
        return 86092;
      case AV_CODEC_ID_4GV:
        return 86093;
      case AV_CODEC_ID_INTERPLAY_ACM:
        return 86094;
      case AV_CODEC_ID_XMA1:
        return 86095;
      case AV_CODEC_ID_XMA2:
        return 86096;
      case AV_CODEC_ID_DST:
        return 86097;
      case AV_CODEC_ID_ATRAC3AL:
        return 86098;
      case AV_CODEC_ID_ATRAC3PAL:
        return 86099;
      case AV_CODEC_ID_DOLBY_E:
        return 86100;
      case AV_CODEC_ID_APTX:
        return 86101;
      case AV_CODEC_ID_APTX_HD:
        return 86102;
      case AV_CODEC_ID_SBC:
        return 86103;
      case AV_CODEC_ID_ATRAC9:
        return 86104;
      case AV_CODEC_ID_HCOM:
        return 86105;
      case AV_CODEC_ID_ACELP_KELVIN:
        return 86106;
      case AV_CODEC_ID_MPEGH_3D_AUDIO:
        return 86107;
      case AV_CODEC_ID_SIREN:
        return 86108;
      case AV_CODEC_ID_HCA:
        return 86109;
      case AV_CODEC_ID_FASTAUDIO:
        return 86110;
      case AV_CODEC_ID_MSNSIREN:
        return 86111;
      case AV_CODEC_ID_DFPWM:
        return 86112;
      case AV_CODEC_ID_BONK:
        return 86113;
      case AV_CODEC_ID_MISC4:
        return 86114;
      case AV_CODEC_ID_APAC:
        return 86115;
      case AV_CODEC_ID_FTR:
        return 86116;
      case AV_CODEC_ID_WAVARC:
        return 86117;
      case AV_CODEC_ID_RKA:
        return 86118;
      case AV_CODEC_ID_DVD_SUBTITLE:
        return 94208;
      case AV_CODEC_ID_DVB_SUBTITLE:
        return 94209;
      case AV_CODEC_ID_TEXT:
        return 94210;
      case AV_CODEC_ID_XSUB:
        return 94211;
      case AV_CODEC_ID_SSA:
        return 94212;
      case AV_CODEC_ID_MOV_TEXT:
        return 94213;
      case AV_CODEC_ID_HDMV_PGS_SUBTITLE:
        return 94214;
      case AV_CODEC_ID_DVB_TELETEXT:
        return 94215;
      case AV_CODEC_ID_SRT:
        return 94216;
      case AV_CODEC_ID_MICRODVD:
        return 94217;
      case AV_CODEC_ID_EIA_608:
        return 94218;
      case AV_CODEC_ID_JACOSUB:
        return 94219;
      case AV_CODEC_ID_SAMI:
        return 94220;
      case AV_CODEC_ID_REALTEXT:
        return 94221;
      case AV_CODEC_ID_STL:
        return 94222;
      case AV_CODEC_ID_SUBVIEWER1:
        return 94223;
      case AV_CODEC_ID_SUBVIEWER:
        return 94224;
      case AV_CODEC_ID_SUBRIP:
        return 94225;
      case AV_CODEC_ID_WEBVTT:
        return 94226;
      case AV_CODEC_ID_MPL2:
        return 94227;
      case AV_CODEC_ID_VPLAYER:
        return 94228;
      case AV_CODEC_ID_PJS:
        return 94229;
      case AV_CODEC_ID_ASS:
        return 94230;
      case AV_CODEC_ID_HDMV_TEXT_SUBTITLE:
        return 94231;
      case AV_CODEC_ID_TTML:
        return 94232;
      case AV_CODEC_ID_ARIB_CAPTION:
        return 94233;
      case AV_CODEC_ID_TTF:
        return 98304;
      case AV_CODEC_ID_SCTE_35:
        return 98305;
      case AV_CODEC_ID_EPG:
        return 98306;
      case AV_CODEC_ID_BINTEXT:
        return 98307;
      case AV_CODEC_ID_XBIN:
        return 98308;
      case AV_CODEC_ID_IDF:
        return 98309;
      case AV_CODEC_ID_OTF:
        return 98310;
      case AV_CODEC_ID_SMPTE_KLV:
        return 98311;
      case AV_CODEC_ID_DVD_NAV:
        return 98312;
      case AV_CODEC_ID_TIMED_ID3:
        return 98313;
      case AV_CODEC_ID_BIN_DATA:
        return 98314;
      case AV_CODEC_ID_PROBE:
        return 102400;
      case AV_CODEC_ID_MPEG2TS:
        return 131072;
      case AV_CODEC_ID_MPEG4SYSTEMS:
        return 131073;
      case AV_CODEC_ID_FFMETADATA:
        return 135168;
      case AV_CODEC_ID_WRAPPED_AVFRAME:
        return 135169;
      case AV_CODEC_ID_VNULL:
        return 135170;
      case AV_CODEC_ID_ANULL:
        return 135171;
    default:
      return AV_CODEC_ID_NONE;
    }
  }
};

class PixFmt {

public:
  static uint32_t fromAVPixFmt(AVPixelFormat avPixelFormat){
    switch (avPixelFormat) {
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
        case AV_PIX_FMT_ARGB:  // Big Endian
          return 29;
        case AV_PIX_FMT_RGBA:  // Big
          return 30;
        case AV_PIX_FMT_ABGR: // Big
          return 31;
        case AV_PIX_FMT_BGRA:  // little
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
        case AV_PIX_FMT_0RGB:  // big
          return 128;
        case AV_PIX_FMT_RGB0:
          return 129;
        case AV_PIX_FMT_0BGR:   // big
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
//        case AV_PIX_FMT_RGB32:  // IF format is this type, based on endianness, it resolves to big endian or small endian.
//          return 175;           // The Switch case contains both big and small endian, so No need to add these in switch case.
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
//        case AV_PIX_FMT_RGB555:      // Little Endian, Big Endian WIll Resolve on it's own.
//          return 298;
        default:
          return 0;
    }
  }

  static AVPixelFormat intoAVPixFmt(uint32_t AvPixFmtId){
    switch (AvPixFmtId) {
       case 0:
         return AV_PIX_FMT_NONE ;
       case 1:
         return AV_PIX_FMT_YUV420P ;
       case 2:
         return AV_PIX_FMT_YUYV422 ;
       case 3:
         return AV_PIX_FMT_RGB24 ;
       case 4:
         return AV_PIX_FMT_BGR24 ;
       case 5:
         return AV_PIX_FMT_YUV422P ;
       case 7:
         return AV_PIX_FMT_YUV444P ;
       case 8:
         return AV_PIX_FMT_YUV410P ;
       case 9:
         return AV_PIX_FMT_YUV411P ;
       case 10:
         return AV_PIX_FMT_GRAY8 ;
       case 11:
         return AV_PIX_FMT_MONOWHITE ;
       case 12:
         return AV_PIX_FMT_MONOBLACK ;
       case 13:
         return AV_PIX_FMT_PAL8 ;
       case 14:
         return AV_PIX_FMT_YUVJ420P ;
       case 15:
         return AV_PIX_FMT_YUVJ422P ;
       case 16:
         return AV_PIX_FMT_YUVJ444P ;
  //     case 17:
  //       return AV_PIX_FMT_XVMC_MPEG2_MC ;     // Lower FFmpeg Version
  //     case 18:
  //       return AV_PIX_FMT_XVMC_MPEG2_IDCT ;
       case 19:
         return AV_PIX_FMT_UYVY422 ;
       case 20:
         return AV_PIX_FMT_UYYVYY411 ;
       case 21:
         return AV_PIX_FMT_BGR8 ;
       case 22:
         return AV_PIX_FMT_BGR4 ;
       case 23:
         return AV_PIX_FMT_BGR4_BYTE ;
       case 24:
         return AV_PIX_FMT_RGB8 ;
       case 25:
         return AV_PIX_FMT_RGB4 ;
       case 26:
         return AV_PIX_FMT_RGB4_BYTE ;
       case 27:
         return AV_PIX_FMT_NV12 ;
       case 28:
         return AV_PIX_FMT_NV21 ;
       case 29:
         return AV_PIX_FMT_ARGB ;
       case 30:
         return AV_PIX_FMT_RGBA ;
       case 31:
         return AV_PIX_FMT_ABGR ;
       case 32:
         return AV_PIX_FMT_BGRA ;  // Little
       case 33:
         return AV_PIX_FMT_GRAY16BE ;
       case 34:
         return AV_PIX_FMT_GRAY16LE ;
       case 35:
         return AV_PIX_FMT_YUV440P ;
       case 36:
         return AV_PIX_FMT_YUVJ440P ;
       case 37:
         return AV_PIX_FMT_YUVA420P ;
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
         return AV_PIX_FMT_RGB48BE ;
       case 44:
         return AV_PIX_FMT_RGB48LE ;
       case 45:
         return AV_PIX_FMT_RGB565BE ;
       case 46:
         return AV_PIX_FMT_RGB565LE ;
       case 47:
         return AV_PIX_FMT_RGB555BE ;
       case 48:
         return AV_PIX_FMT_RGB555LE ;
       case 49:
         return AV_PIX_FMT_BGR565BE ;
       case 50:
         return AV_PIX_FMT_BGR565LE ;
       case 51:
         return AV_PIX_FMT_BGR555BE ;
       case 52:
         return AV_PIX_FMT_BGR555LE ;
  //     case 53:
  //       return AV_PIX_FMT_VAAPI_MOCO ;
  //     case 54:
  //       return AV_PIX_FMT_VAAPI_IDCT ;
  //     case 55:
  //       return AV_PIX_FMT_VAAPI_VLD ;
  //     case 56:
  //       return AV_PIX_FMT_VAAPI ;    // ff_api_vdpau is present
       case 57:
         return AV_PIX_FMT_YUV420P16LE ;
       case 58:
         return AV_PIX_FMT_YUV420P16BE ;
       case 59:
         return AV_PIX_FMT_YUV422P16LE ;
       case 60:
         return AV_PIX_FMT_YUV422P16BE ;
       case 61:
         return AV_PIX_FMT_YUV444P16LE ;
       case 62:
         return AV_PIX_FMT_YUV444P16BE ;
  //     case 63:
  //       return AV_PIX_FMT_VDPAU_MPEG4 ;   // ff_api_vdpau is present
       case 64:
         return AV_PIX_FMT_DXVA2_VLD ;
       case 65:
         return AV_PIX_FMT_RGB444LE ;
       case 66:
         return AV_PIX_FMT_RGB444BE ;
       case 67:
         return AV_PIX_FMT_BGR444LE ;
       case 68:
         return AV_PIX_FMT_BGR444BE ;
       case 69:
         return AV_PIX_FMT_YA8 ;
       case 70:
         return AV_PIX_FMT_BGR48BE ;
       case 71:
         return AV_PIX_FMT_BGR48LE ;
       case 72:
         return AV_PIX_FMT_YUV420P9BE ;
       case 73:
         return AV_PIX_FMT_YUV420P9LE ;
       case 74:
         return AV_PIX_FMT_YUV420P10BE ;
       case 75:
         return AV_PIX_FMT_YUV420P10LE ;
       case 76:
         return AV_PIX_FMT_YUV422P10BE ;
       case 77:
         return AV_PIX_FMT_YUV422P10LE ;
       case 78:
         return AV_PIX_FMT_YUV444P9BE ;
       case 79:
         return AV_PIX_FMT_YUV444P9LE ;
       case 80:
         return AV_PIX_FMT_YUV444P10BE ;
       case 81:
         return AV_PIX_FMT_YUV444P10LE ;
       case 82:
         return AV_PIX_FMT_YUV422P9BE ;
       case 83:
         return AV_PIX_FMT_YUV422P9LE ;
  //     case 84:
  //       return AV_PIX_FMT_VDA_VLD ;   // Lower than ffmpeg version 4
       case 85:
         return AV_PIX_FMT_GBRP ;
       case 86:
         return AV_PIX_FMT_GBRP9BE ;
       case 87:
         return AV_PIX_FMT_GBRP9LE ;
       case 88:
         return AV_PIX_FMT_GBRP10BE ;
       case 89:
         return AV_PIX_FMT_GBRP10LE ;
       case 90:
         return AV_PIX_FMT_GBRP16BE ;
       case 91:
         return AV_PIX_FMT_GBRP16LE ;
       case 92:
         return AV_PIX_FMT_YUVA420P9BE ;
       case 93:
         return AV_PIX_FMT_YUVA420P9LE ;
       case 94:
         return AV_PIX_FMT_YUVA422P9BE ;
       case 95:
         return AV_PIX_FMT_YUVA422P9LE ;
       case 96:
         return AV_PIX_FMT_YUVA444P9BE ;
       case 97:
         return AV_PIX_FMT_YUVA444P9LE ;
       case 98:
         return AV_PIX_FMT_YUVA420P10BE ;
       case 99:
         return AV_PIX_FMT_YUVA420P10LE ;
       case 100:
         return AV_PIX_FMT_YUVA422P10BE ;
       case 101:
         return AV_PIX_FMT_YUVA422P10LE ;
       case 102:
         return AV_PIX_FMT_YUVA444P10BE ;
       case 103:
         return AV_PIX_FMT_YUVA444P10LE ;
       case 104:
         return AV_PIX_FMT_YUVA420P16BE ;
       case 105:
         return AV_PIX_FMT_YUVA420P16LE ;
       case 106:
         return AV_PIX_FMT_YUVA422P16BE ;
       case 107:
         return AV_PIX_FMT_YUVA422P16LE ;
       case 108:
         return AV_PIX_FMT_YUVA444P16BE ;
       case 109:
         return AV_PIX_FMT_YUVA444P16LE ;
       case 110:
         return AV_PIX_FMT_VDPAU ;
       case 111:
         return AV_PIX_FMT_XYZ12LE ;
       case 112:
         return AV_PIX_FMT_XYZ12BE ;
       case 113:
         return AV_PIX_FMT_NV16 ;
       case 114:
         return AV_PIX_FMT_NV20LE ;
       case 115:
         return AV_PIX_FMT_NV20BE ;
       case 116:
         return AV_PIX_FMT_RGBA64BE ;
       case 117:
         return AV_PIX_FMT_RGBA64LE ;
       case 118:
         return AV_PIX_FMT_BGRA64BE ;
       case 119:
         return AV_PIX_FMT_BGRA64LE ;
       case 120:
         return AV_PIX_FMT_YVYU422 ;
  //     case 121:
  //       return AV_PIX_FMT_VDA ;   // Lower than ffmpeg version 4.
       case 122:
         return AV_PIX_FMT_YA16BE ;
       case 123:
         return AV_PIX_FMT_YA16LE ;
       case 124:
         return AV_PIX_FMT_QSV ;
       case 125:
         return AV_PIX_FMT_MMAL ;
       case 126:
         return AV_PIX_FMT_D3D11VA_VLD ;
       case 127:
         return AV_PIX_FMT_CUDA ;
       case 128:
         return AV_PIX_FMT_0RGB ;
       case 129:
         return AV_PIX_FMT_RGB0 ;
       case 130:
         return AV_PIX_FMT_0BGR ;
       case 131:
         return AV_PIX_FMT_BGR0 ;
       case 132:
         return AV_PIX_FMT_YUVA444P ;
       case 133:
         return AV_PIX_FMT_YUVA422P ;
       case 134:
         return AV_PIX_FMT_YUV420P12BE ;
       case 135:
         return AV_PIX_FMT_YUV420P12LE ;
       case 136:
         return AV_PIX_FMT_YUV420P14BE ;
       case 137:
         return AV_PIX_FMT_YUV420P14LE ;
       case 138:
         return AV_PIX_FMT_YUV422P12BE ;
       case 139:
         return AV_PIX_FMT_YUV422P12LE ;
       case 140:
         return AV_PIX_FMT_YUV422P14BE ;
       case 141:
         return AV_PIX_FMT_YUV422P14LE ;
       case 142:
         return AV_PIX_FMT_YUV444P12BE ;
       case 143:
         return AV_PIX_FMT_YUV444P12LE ;
       case 144:
         return AV_PIX_FMT_YUV444P14BE ;
       case 146:
         return AV_PIX_FMT_YUV444P14LE ;
       case 147:
         return AV_PIX_FMT_GBRP12BE ;
       case 148:
         return AV_PIX_FMT_GBRP12LE ;
       case 149:
         return AV_PIX_FMT_GBRP14BE ;
       case 150:
         return AV_PIX_FMT_GBRP14LE ;
       case 151:
         return AV_PIX_FMT_GBRAP ;
       case 152:
         return AV_PIX_FMT_GBRAP16BE ;
       case 153:
         return AV_PIX_FMT_GBRAP16LE ;
       case 154:
         return AV_PIX_FMT_YUVJ411P ;
       case 155:
         return AV_PIX_FMT_BAYER_BGGR8 ;
       case 156:
         return AV_PIX_FMT_BAYER_RGGB8 ;
       case 157:
         return AV_PIX_FMT_BAYER_GBRG8 ;
       case 158:
         return AV_PIX_FMT_BAYER_GRBG8 ;
       case 159:
         return AV_PIX_FMT_BAYER_BGGR16LE ;
       case 160:
         return AV_PIX_FMT_BAYER_BGGR16BE ;
       case 161:
         return AV_PIX_FMT_BAYER_RGGB16LE ;
       case 162:
         return AV_PIX_FMT_BAYER_RGGB16BE ;
       case 163:
         return AV_PIX_FMT_BAYER_GBRG16LE ;
       case 164:
         return AV_PIX_FMT_BAYER_GBRG16BE ;
       case 165:
         return AV_PIX_FMT_BAYER_GRBG16LE ;
       case 166:
         return AV_PIX_FMT_BAYER_GRBG16BE ;
       case 167:
         return AV_PIX_FMT_YUV440P10LE ;
       case 168:
         return AV_PIX_FMT_YUV440P10BE ;
       case 169:
         return AV_PIX_FMT_YUV440P12LE ;
       case  170:
         return AV_PIX_FMT_YUV440P12BE ;
       case 171:
         return AV_PIX_FMT_AYUV64LE ;
       case 172:
         return AV_PIX_FMT_AYUV64BE ;
       case 173:
         return AV_PIX_FMT_VIDEOTOOLBOX ;
       case 174:
         return AV_PIX_FMT_XVMC ;
       case 175:
         return AV_PIX_FMT_RGB32 ;
       case 176:
         return AV_PIX_FMT_RGB32_1 ;
       case 177:
         return AV_PIX_FMT_BGR32 ;
       case 178:
         return AV_PIX_FMT_BGR32_1 ;
       case 179:
         return AV_PIX_FMT_0RGB32 ;
       case 180:
         return AV_PIX_FMT_0BGR32 ;
       case 181:
         return AV_PIX_FMT_GRAY16 ;
       case 182:
         return AV_PIX_FMT_YA16 ;
       case 183:
         return AV_PIX_FMT_RGB48 ;
       case 184:
         return AV_PIX_FMT_RGB565 ;
       case 185:
         return AV_PIX_FMT_RGB444 ;
       case 186:
         return AV_PIX_FMT_BGR48 ;
       case 187:
         return AV_PIX_FMT_BGR565 ;
       case 188:
         return AV_PIX_FMT_BGR555 ;
       case 189:
         return AV_PIX_FMT_BGR444 ;
       case 190:
         return AV_PIX_FMT_YUV420P9 ;
       case 191:
         return AV_PIX_FMT_YUV422P9 ;
       case 192:
         return AV_PIX_FMT_YUV444P9 ;
       case 193:
         return AV_PIX_FMT_YUV420P10 ;
       case 194:
         return AV_PIX_FMT_YUV422P10 ;
       case 195:
         return AV_PIX_FMT_YUV440P10 ;
       case 196:
         return AV_PIX_FMT_YUV444P10 ;
       case 197:
         return AV_PIX_FMT_YUV420P12 ;
       case 198:
         return AV_PIX_FMT_YUV422P12 ;
       case 199:
         return AV_PIX_FMT_YUV440P12 ;
       case 200:
         return AV_PIX_FMT_YUV444P12 ;
       case 201:
         return AV_PIX_FMT_YUV420P14 ;
       case 202:
         return AV_PIX_FMT_YUV422P14 ;
       case 203:
         return AV_PIX_FMT_YUV444P14 ;
       case 204:
         return AV_PIX_FMT_YUV420P16 ;
       case 205:
         return AV_PIX_FMT_YUV422P16 ;
       case 206:
         return AV_PIX_FMT_YUV444P16 ;
       case 207:
         return AV_PIX_FMT_GBRP9 ;
       case 208:
         return AV_PIX_FMT_GBRP10 ;
       case 209:
         return AV_PIX_FMT_GBRP12 ;
       case 210:
         return AV_PIX_FMT_GBRP14 ;
       case 211:
         return AV_PIX_FMT_GBRP16 ;
       case 212:
         return AV_PIX_FMT_GBRAP16 ;
       case 213:
         return AV_PIX_FMT_BAYER_BGGR16 ;
       case 214:
         return AV_PIX_FMT_BAYER_RGGB16 ;
       case 215:
         return AV_PIX_FMT_BAYER_GBRG16 ;
       case 216:
         return AV_PIX_FMT_BAYER_GRBG16 ;
       case 217:
         return AV_PIX_FMT_YUVA420P9 ;
       case 218:
         return AV_PIX_FMT_YUVA422P9 ;
       case 219:
         return AV_PIX_FMT_YUVA444P9 ;
       case 220:
         return AV_PIX_FMT_YUVA420P10 ;
       case 221:
         return AV_PIX_FMT_YUVA422P10 ;
       case 222:
         return AV_PIX_FMT_YUVA444P10 ;
       case 223:
         return AV_PIX_FMT_YUVA420P16 ;
       case 224:
         return AV_PIX_FMT_YUVA422P16 ;
       case 225:
         return AV_PIX_FMT_YUVA444P16 ;
       case 226:
         return AV_PIX_FMT_XYZ12 ;
       case 227:
         return AV_PIX_FMT_NV20 ;
       case 228:
         return AV_PIX_FMT_AYUV64 ;
       case 229:
         return AV_PIX_FMT_P010LE ;
       case 230:
         return AV_PIX_FMT_P010BE ;
       case 231:
         return AV_PIX_FMT_GBRAP12BE ;
       case 232:
         return AV_PIX_FMT_GBRAP12LE ;
       case 233:
         return AV_PIX_FMT_GBRAP10LE ;
       case 234:
         return AV_PIX_FMT_GBRAP10BE ;
       case 235:
         return AV_PIX_FMT_MEDIACODEC ;
       case 236:
         return AV_PIX_FMT_GRAY12BE ;
       case 237:
         return AV_PIX_FMT_GRAY12LE ;
       case 238:
         return AV_PIX_FMT_GRAY10BE ;
       case 239:
         return AV_PIX_FMT_GRAY10LE ;
       case 240:
         return AV_PIX_FMT_P016LE ;
       case 241:
         return AV_PIX_FMT_P016BE ;
       case 242:
         return AV_PIX_FMT_D3D11 ;
       case 243:
         return AV_PIX_FMT_GRAY9BE ;
       case 244:
         return AV_PIX_FMT_GRAY9LE ;
       case 245:
         return AV_PIX_FMT_GBRPF32BE ;
       case 246:
         return AV_PIX_FMT_GBRPF32LE ;
       case 247:
         return AV_PIX_FMT_GBRAPF32BE ;
       case 248:
         return AV_PIX_FMT_GBRAPF32LE ;
       case 249:
         return AV_PIX_FMT_DRM_PRIME ;

         // Above ffmpeg 4.0  Need to add versions.
       case 250:
         return AV_PIX_FMT_OPENCL ;
       case 251:
         return AV_PIX_FMT_GRAY14BE ;
       case 252:
         return AV_PIX_FMT_GRAY14LE ;
       case 253:
         return AV_PIX_FMT_GRAYF32BE ;
       case 254:
         return AV_PIX_FMT_GRAYF32LE ;
       case 255:
         return AV_PIX_FMT_YUVA422P12BE ;
       case 256:
         return AV_PIX_FMT_YUVA422P12LE ;
       case 257:
         return AV_PIX_FMT_YUVA444P12BE ;
       case 258:
         return AV_PIX_FMT_YUVA444P12LE ;
       case 259:
         return AV_PIX_FMT_NV24 ;
       case 260:
         return AV_PIX_FMT_NV42 ;
       case 261:
         return AV_PIX_FMT_VULKAN ;
       case 262:
         return AV_PIX_FMT_Y210BE ;
       case 263:
         return AV_PIX_FMT_Y210LE ;
       case 264:
         return AV_PIX_FMT_X2RGB10LE ;
       case 265:
         return AV_PIX_FMT_X2RGB10BE ;
       case 266:
         return AV_PIX_FMT_X2BGR10LE ;
       case 267:
         return AV_PIX_FMT_X2BGR10BE ;
       case 268:
         return AV_PIX_FMT_P210BE ;
       case 269:
         return AV_PIX_FMT_P210LE ;
       case 270:
         return AV_PIX_FMT_P410BE ;
       case 271:
         return AV_PIX_FMT_P410LE ;
       case 272:
         return AV_PIX_FMT_P216BE ;
       case 273:
         return AV_PIX_FMT_P216LE ;
       case 274:
         return AV_PIX_FMT_P416BE ;
       case 275:
         return AV_PIX_FMT_P416LE ;
       case 276:
         return AV_PIX_FMT_VUYA ;
       case 277:
         return AV_PIX_FMT_RGBAF16BE ;
       case 278:
         return AV_PIX_FMT_RGBAF16LE ;
       case 279:
         return AV_PIX_FMT_VUYX ;
       case 280:
         return AV_PIX_FMT_P012LE ;
       case 281:
         return AV_PIX_FMT_P012BE ;
       case 282:
         return AV_PIX_FMT_Y212BE ;
       case 283:
         return AV_PIX_FMT_Y212LE ;
       case 284:
         return AV_PIX_FMT_XV30BE ;
       case 285:
         return AV_PIX_FMT_XV30LE ;
       case 286:
         return AV_PIX_FMT_XV36BE ;
       case 287:
         return AV_PIX_FMT_XV36LE ;
       case 288:
         return AV_PIX_FMT_RGBF32BE ;
       case 289:
         return AV_PIX_FMT_RGBF32LE ;
       case 290:
         return AV_PIX_FMT_RGBAF32BE ;
       case 291:
         return AV_PIX_FMT_RGBAF32LE ;
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
        return AV_PIX_FMT_RGB555 ;
       default:
        return AV_PIX_FMT_NONE;
     }
  }
};

class SampleFmt{
public:
  static AVSampleFormat fromSampleID(uint32_t SampleID){
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

  static uint32_t toSampleID(AVSampleFormat AvSampleFormat){
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

// Could have avoided, but Did this to support older version of FFMPEG (V5,V4,V3)
// Version 6 FFmpeg uses AVChannelLayout Struct;
class ChannelLayout {
private:
  const static uint64_t FRONT_LEFT            = 1;
  const static uint64_t FRONT_RIGHT           = 1ULL << 1;
  const static uint64_t FRONT_CENTER          = 1ULL << 2;
  const static uint64_t LOW_FREQUENCY         = 1ULL << 3;
  const static uint64_t BACK_LEFT             = 1ULL << 4;
  const static uint64_t BACK_RIGHT            = 1ULL << 5;
  const static uint64_t FRONT_LEFT_OF_CENTER  = 1ULL << 6;
  const static uint64_t FRONT_RIGHT_OF_CENTER = 1ULL << 7;
  const static uint64_t BACK_CENTER           = 1ULL << 8;
  const static uint64_t SIDE_LEFT             = 1ULL << 9;
  const static uint64_t SIDE_RIGHT            = 1ULL << 10;
  const static uint64_t TOP_CENTER            = 1ULL << 11;
  const static uint64_t TOP_FRONT_LEFT        = 1ULL << 12;
  const static uint64_t TOP_FRONT_CENTER      = 1ULL << 13;
  const static uint64_t TOP_FRONT_RIGHT       = 1ULL << 14;
  const static uint64_t TOP_BACK_LEFT         = 1ULL << 15;
  const static uint64_t TOP_BACK_CENTER       = 1ULL << 16;
  const static uint64_t TOP_BACK_RIGHT        = 1ULL << 17;
  const static uint64_t STEREO_LEFT           = 1ULL << 18;
  const static uint64_t STEREO_RIGHT          = 1ULL << 19;
  const static uint64_t WIDE_LEFT             = 1ULL << 20;
  const static uint64_t WIDE_RIGHT            = 1ULL << 21;
  const static uint64_t SURROUND_DIRECT_LEFT  = 1ULL << 22;
  const static uint64_t SURROUND_DIRECT_RIGHT = 1ULL << 23;
  const static uint64_t LOW_FREQUENCY_2       = 1ULL << 24;
  const static uint64_t NATIVE                = 1ULL << 25;

  const static uint64_t MONO               = 1ULL << 26;
  const static uint64_t STEREO             = 1ULL << 27;
  const static uint64_t _2POINT1           = 1ULL << 28;
  const static uint64_t _2_1               = 1ULL << 29;
  const static uint64_t SURROUND           = 1ULL << 30;
  const static uint64_t _3POINT1           = 1ULL << 31;
  const static uint64_t _4POINT0           = 1ULL << 32;
  const static uint64_t _4POINT1           = 1ULL << 33;
  const static uint64_t _2_2               = 1ULL << 34;
  const static uint64_t QUAD               = 1ULL << 35;
  const static uint64_t _5POINT0           = 1ULL << 36;
  const static uint64_t _5POINT1           = 1ULL << 37;
  const static uint64_t _5POINT0_BACK      = 1ULL << 38;
  const static uint64_t _5POINT1_BACK      = 1ULL << 39;
  const static uint64_t _6POINT0           = 1ULL << 40;
  const static uint64_t _6POINT0_FRONT     = 1ULL << 41;
  const static uint64_t HEXAGONAL          = 1ULL << 42;
  const static uint64_t _6POINT1           = 1ULL << 43;
  const static uint64_t _6POINT1_BACK      = 1ULL << 44;
  const static uint64_t _6POINT1_FRONT     = 1ULL << 45;
  const static uint64_t _7POINT0           = 1ULL << 46;
  const static uint64_t _7POINT0_FRONT     = 1ULL << 47;
  const static uint64_t _7POINT1           = 1ULL << 48;
  const static uint64_t _7POINT1_WIDE      = 1ULL << 49;
  const static uint64_t _7POINT1_WIDE_BACK = 1ULL << 50;
  const static uint64_t OCTAGONAL          = 1ULL << 51;
  const static uint64_t HEXADECAGONAL      = 1ULL << 52;
  const static uint64_t STEREO_DOWNMIX     = 1ULL << 53;

public:

  // Check This function. (Looks good, test it)
  static uint64_t fromChannelLayoutID(uint64_t ChannelLayout) {
     uint64_t Channel = 0;
     if(ChannelLayout & FRONT_LEFT)
       Channel |= AV_CH_FRONT_LEFT;
     if(ChannelLayout & FRONT_RIGHT)
       Channel |= AV_CH_FRONT_RIGHT;
     if(ChannelLayout & FRONT_CENTER)
       Channel |= AV_CH_FRONT_CENTER;
     if(ChannelLayout & LOW_FREQUENCY)
       Channel |= AV_CH_LOW_FREQUENCY;
     if(ChannelLayout & BACK_LEFT)
       Channel |= AV_CH_BACK_LEFT;
     if(ChannelLayout & BACK_RIGHT)
       Channel |= AV_CH_BACK_RIGHT;
     if(ChannelLayout & FRONT_LEFT_OF_CENTER)
       Channel |= AV_CH_FRONT_LEFT_OF_CENTER;
     if(ChannelLayout & FRONT_RIGHT_OF_CENTER)
       Channel |= AV_CH_FRONT_RIGHT_OF_CENTER;
     if(ChannelLayout & BACK_CENTER)
       Channel |= AV_CH_BACK_CENTER;
     if(ChannelLayout & SIDE_LEFT)
       Channel |= AV_CH_SIDE_LEFT;
     if(ChannelLayout & SIDE_RIGHT)
       Channel |= AV_CH_SIDE_RIGHT;
     if(ChannelLayout & TOP_CENTER)
       Channel |= AV_CH_TOP_CENTER;
     if(ChannelLayout & TOP_FRONT_LEFT)
       Channel |= AV_CH_TOP_FRONT_LEFT;
     if(ChannelLayout & TOP_FRONT_CENTER)
       Channel |= AV_CH_TOP_FRONT_CENTER;
     if(ChannelLayout & TOP_FRONT_RIGHT)
       Channel |= AV_CH_TOP_FRONT_RIGHT;
     if(ChannelLayout & TOP_BACK_LEFT)
       Channel |= AV_CH_TOP_BACK_LEFT;
     if(ChannelLayout & TOP_BACK_CENTER)
       Channel |= AV_CH_TOP_FRONT_CENTER;
     if(ChannelLayout & TOP_BACK_RIGHT)
       Channel |= AV_CH_TOP_BACK_RIGHT;
     if(ChannelLayout & STEREO_LEFT)
       Channel |= AV_CH_STEREO_LEFT;
     if(ChannelLayout & STEREO_RIGHT)
       Channel |= AV_CH_STEREO_RIGHT;
     if(ChannelLayout & WIDE_LEFT)
       Channel |= AV_CH_WIDE_LEFT;
     if(ChannelLayout & WIDE_RIGHT)
       Channel |= AV_CH_WIDE_RIGHT;
     if(ChannelLayout & SURROUND_DIRECT_LEFT)
       Channel |= AV_CH_SURROUND_DIRECT_LEFT;
     if(ChannelLayout & SURROUND_DIRECT_RIGHT)
       Channel |= AV_CH_SURROUND_DIRECT_RIGHT;
     if(ChannelLayout & LOW_FREQUENCY_2)
       Channel |= AV_CH_LOW_FREQUENCY_2;
     if(ChannelLayout & NATIVE)
       Channel |= AV_CH_LAYOUT_NATIVE;
     if(ChannelLayout & MONO)
       Channel |= AV_CH_LAYOUT_MONO;
     if(ChannelLayout & STEREO)
       Channel |= AV_CH_LAYOUT_STEREO;
     if(ChannelLayout & _2POINT1)
       Channel |= AV_CH_LAYOUT_2POINT1;
     if(ChannelLayout & _2_1)
       Channel |= AV_CH_LAYOUT_2_1;
     if(ChannelLayout & SURROUND)
       Channel |= AV_CH_LAYOUT_SURROUND;
     if(ChannelLayout & _3POINT1)
       Channel |= AV_CH_LAYOUT_3POINT1;
     if(ChannelLayout & _4POINT0)
       Channel |= AV_CH_LAYOUT_4POINT0;
     if(ChannelLayout & _4POINT1)
       Channel |= AV_CH_LAYOUT_4POINT1;
     if(ChannelLayout & _2_2)
       Channel |= AV_CH_LAYOUT_2_2;
     if(ChannelLayout & QUAD)
       Channel |= AV_CH_LAYOUT_QUAD;
     if(ChannelLayout & _5POINT0)
       Channel |= AV_CH_LAYOUT_5POINT0;
     if(ChannelLayout & _5POINT1)
       Channel |= AV_CH_LAYOUT_5POINT1;
     if(ChannelLayout & _5POINT0_BACK)
       Channel |= AV_CH_LAYOUT_5POINT0_BACK;
     if(ChannelLayout & _5POINT1_BACK)
       Channel |= AV_CH_LAYOUT_5POINT1_BACK;
     if(ChannelLayout & _6POINT0)
       Channel |= AV_CH_LAYOUT_6POINT0;
     if(ChannelLayout & _6POINT0_FRONT)
       Channel |= AV_CH_LAYOUT_6POINT0_FRONT;
     if(ChannelLayout & HEXAGONAL)
       Channel |= AV_CH_LAYOUT_HEXAGONAL;
     if(ChannelLayout & _6POINT1)
       Channel |= AV_CH_LAYOUT_6POINT1;
     if(ChannelLayout & _6POINT1_BACK)
       Channel |= AV_CH_LAYOUT_6POINT1_BACK;
     if(ChannelLayout & _6POINT1_FRONT)
       Channel |= AV_CH_LAYOUT_6POINT1_FRONT;
     if(ChannelLayout & _7POINT0)
       Channel |= AV_CH_LAYOUT_7POINT0;
     if(ChannelLayout & _7POINT0_FRONT)
       Channel |= AV_CH_LAYOUT_7POINT0_FRONT;
     if(ChannelLayout & _7POINT1)
       Channel |= AV_CH_LAYOUT_7POINT1;
     if(ChannelLayout & _7POINT1_WIDE)
       Channel |= AV_CH_LAYOUT_7POINT1_WIDE;
     if(ChannelLayout & _7POINT1_WIDE_BACK)
       Channel |= AV_CH_LAYOUT_7POINT1_WIDE_BACK;
     if(ChannelLayout & OCTAGONAL)
       Channel |= AV_CH_LAYOUT_OCTAGONAL;
     if(ChannelLayout & HEXADECAGONAL)
       Channel |= AV_CH_LAYOUT_HEXADECAGONAL;
     if(ChannelLayout & STEREO_DOWNMIX)
       Channel |= AV_CH_LAYOUT_STEREO_DOWNMIX;
     return Channel;
  }

    // Perfect Logic :)
    static uint64_t intoAVChannelID(uint64_t ChannelLayout) {
     uint64_t Channel = 0;
     if((ChannelLayout &  AV_CH_FRONT_LEFT) == AV_CH_FRONT_LEFT)
       Channel |= FRONT_LEFT;
     if((ChannelLayout &  AV_CH_FRONT_RIGHT) == AV_CH_FRONT_RIGHT)
       Channel |= FRONT_RIGHT;
     if((ChannelLayout &  AV_CH_FRONT_CENTER) == AV_CH_FRONT_CENTER)
       Channel |= FRONT_CENTER;
     if((ChannelLayout &  AV_CH_LOW_FREQUENCY) == AV_CH_LOW_FREQUENCY)
       Channel |= LOW_FREQUENCY;
     if((ChannelLayout &  AV_CH_BACK_LEFT) == AV_CH_BACK_LEFT)
       Channel |= BACK_LEFT;
     if((ChannelLayout &  AV_CH_BACK_RIGHT) == AV_CH_BACK_RIGHT)
       Channel |= BACK_RIGHT;
     if((ChannelLayout &  AV_CH_FRONT_LEFT_OF_CENTER) == AV_CH_FRONT_LEFT_OF_CENTER)
       Channel |= FRONT_LEFT_OF_CENTER;
     if((ChannelLayout &  AV_CH_FRONT_RIGHT_OF_CENTER) == AV_CH_FRONT_RIGHT_OF_CENTER)
       Channel |= FRONT_RIGHT_OF_CENTER;
     if((ChannelLayout &  AV_CH_BACK_CENTER) == AV_CH_BACK_CENTER)
       Channel |= BACK_CENTER;
     if((ChannelLayout &  AV_CH_SIDE_LEFT) == AV_CH_SIDE_LEFT)
       Channel |= SIDE_LEFT;
     if((ChannelLayout &  AV_CH_SIDE_RIGHT) == AV_CH_SIDE_RIGHT)
       Channel |= SIDE_RIGHT;
     if((ChannelLayout &  AV_CH_TOP_CENTER) == AV_CH_TOP_CENTER)
       Channel |= TOP_CENTER;
     if((ChannelLayout &  AV_CH_TOP_FRONT_LEFT) == AV_CH_TOP_FRONT_LEFT)
       Channel |= TOP_FRONT_LEFT;
     if((ChannelLayout &  AV_CH_TOP_FRONT_CENTER) == AV_CH_TOP_FRONT_CENTER)
       Channel |= TOP_FRONT_CENTER;
     if((ChannelLayout &  AV_CH_TOP_FRONT_RIGHT) == AV_CH_TOP_FRONT_RIGHT)
       Channel |= TOP_FRONT_RIGHT;
     if((ChannelLayout &  AV_CH_TOP_BACK_LEFT) == AV_CH_TOP_BACK_LEFT)
       Channel |= TOP_BACK_LEFT;
     if((ChannelLayout &  AV_CH_TOP_BACK_CENTER) == AV_CH_TOP_BACK_CENTER)
       Channel |= TOP_FRONT_CENTER;
     if((ChannelLayout &  AV_CH_TOP_BACK_RIGHT) == AV_CH_TOP_BACK_RIGHT)
       Channel |= TOP_BACK_RIGHT;
     if((ChannelLayout &  AV_CH_STEREO_LEFT) == AV_CH_STEREO_LEFT)
       Channel |= STEREO_LEFT;
     if((ChannelLayout &  AV_CH_STEREO_RIGHT) == AV_CH_STEREO_RIGHT)
       Channel |= STEREO_RIGHT;
     if((ChannelLayout &  AV_CH_WIDE_LEFT) == AV_CH_WIDE_LEFT)
       Channel |= WIDE_LEFT;
     if((ChannelLayout &  AV_CH_WIDE_RIGHT) == AV_CH_WIDE_RIGHT)
       Channel |= WIDE_RIGHT;
     if((ChannelLayout &  AV_CH_SURROUND_DIRECT_LEFT) == AV_CH_SURROUND_DIRECT_LEFT)
       Channel |= SURROUND_DIRECT_LEFT;
     if((ChannelLayout &  AV_CH_SURROUND_DIRECT_RIGHT) == AV_CH_SURROUND_DIRECT_RIGHT)
       Channel |= SURROUND_DIRECT_RIGHT;
     if((ChannelLayout &  AV_CH_LOW_FREQUENCY_2) == AV_CH_LOW_FREQUENCY_2)
       Channel |= LOW_FREQUENCY_2;

     // Channel Mask C;
     if((ChannelLayout &  AV_CH_LAYOUT_NATIVE) == AV_CH_LAYOUT_NATIVE)
       Channel |=NATIVE;
     if((ChannelLayout & AV_CH_LAYOUT_MONO) == AV_CH_LAYOUT_MONO)
       Channel |=MONO;
     if((ChannelLayout &  AV_CH_LAYOUT_STEREO) == AV_CH_LAYOUT_STEREO)
       Channel |=STEREO;
     if((ChannelLayout &  AV_CH_LAYOUT_2POINT1) == AV_CH_LAYOUT_2POINT1)
       Channel |= _2POINT1;
     if((ChannelLayout & AV_CH_LAYOUT_2_1) == AV_CH_LAYOUT_2_1)
       Channel |= _2_1;
     if((ChannelLayout &  AV_CH_LAYOUT_SURROUND) == AV_CH_LAYOUT_SURROUND)
       Channel |= SURROUND;
     if((ChannelLayout &  AV_CH_LAYOUT_3POINT1) == AV_CH_LAYOUT_3POINT1)
       Channel |= _3POINT1;
     if((ChannelLayout &  AV_CH_LAYOUT_4POINT0) == AV_CH_LAYOUT_4POINT0)
       Channel |= _4POINT0;
     if((ChannelLayout &  AV_CH_LAYOUT_4POINT1) == AV_CH_LAYOUT_4POINT1)
       Channel |= _4POINT1;
     if((ChannelLayout &  AV_CH_LAYOUT_2_2) == AV_CH_LAYOUT_2_2)
       Channel |= _2_2;
     if((ChannelLayout &  AV_CH_LAYOUT_QUAD) == AV_CH_LAYOUT_QUAD)
       Channel |= QUAD;
     if((ChannelLayout &  AV_CH_LAYOUT_5POINT0) == AV_CH_LAYOUT_5POINT0)
       Channel |= _5POINT0;
     if((ChannelLayout &  AV_CH_LAYOUT_5POINT1) == AV_CH_LAYOUT_5POINT1)
       Channel |= _5POINT1;
     if((ChannelLayout &  AV_CH_LAYOUT_5POINT0_BACK) == AV_CH_LAYOUT_5POINT0_BACK)
       Channel |= _5POINT0_BACK;
     if((ChannelLayout &  AV_CH_LAYOUT_5POINT1_BACK) == AV_CH_LAYOUT_5POINT1_BACK)
       Channel |= _5POINT1_BACK;
     if((ChannelLayout &  AV_CH_LAYOUT_6POINT0) == AV_CH_LAYOUT_6POINT0)
       Channel |= _6POINT0;
     if((ChannelLayout &  AV_CH_LAYOUT_6POINT0_FRONT) == AV_CH_LAYOUT_6POINT0_FRONT)
       Channel |= _6POINT0_FRONT;
     if((ChannelLayout &  AV_CH_LAYOUT_HEXAGONAL) == AV_CH_LAYOUT_HEXAGONAL)
       Channel |= HEXAGONAL;
     if((ChannelLayout &  AV_CH_LAYOUT_6POINT1) == AV_CH_LAYOUT_6POINT1)
       Channel |= _6POINT1;
     if((ChannelLayout &  AV_CH_LAYOUT_6POINT1_BACK) == AV_CH_LAYOUT_6POINT1_BACK)
       Channel |= _6POINT1_BACK;
     if((ChannelLayout &  AV_CH_LAYOUT_6POINT1_FRONT) == AV_CH_LAYOUT_6POINT1_FRONT)
       Channel |= _6POINT1_FRONT;
     if((ChannelLayout &  AV_CH_LAYOUT_7POINT0) == AV_CH_LAYOUT_7POINT0)
       Channel |= _7POINT0;
     if((ChannelLayout &  AV_CH_LAYOUT_7POINT0_FRONT) == AV_CH_LAYOUT_7POINT0_FRONT)
       Channel |= _7POINT0_FRONT;
     if((ChannelLayout &  AV_CH_LAYOUT_7POINT1) == AV_CH_LAYOUT_7POINT1)
       Channel |= _7POINT1;
     if((ChannelLayout &  AV_CH_LAYOUT_7POINT1_WIDE) == AV_CH_LAYOUT_7POINT1_WIDE)
       Channel |= _7POINT1_WIDE;
     if((ChannelLayout &  AV_CH_LAYOUT_7POINT1_WIDE_BACK) == AV_CH_LAYOUT_7POINT1_WIDE_BACK)
       Channel |= _7POINT1_WIDE_BACK;
     if((ChannelLayout &  AV_CH_LAYOUT_OCTAGONAL) == AV_CH_LAYOUT_OCTAGONAL)
       Channel |= OCTAGONAL;
     if((ChannelLayout &  AV_CH_LAYOUT_HEXADECAGONAL) == AV_CH_LAYOUT_HEXADECAGONAL)
       Channel |= HEXADECAGONAL;
     if((ChannelLayout &  AV_CH_LAYOUT_STEREO_DOWNMIX) == AV_CH_LAYOUT_STEREO_DOWNMIX)
       Channel |= STEREO_DOWNMIX;
     return Channel;
    }
};

} // namespace FFmpegUtils
} // namespace WasmEdgeFFmpeg
} // namespace Host
} // namespace WasmEdge