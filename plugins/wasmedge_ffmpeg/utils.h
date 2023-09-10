extern "C" {
  #include "libavutil/avutil.h"
}

namespace WasmEdge{
namespace Host{
namespace WasmEdgeFFmpeg {
namespace FFmpegUtils {

class MediaType {
public:
  static AVMediaType getMediaType(int32_t mediaTypeId){
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
};

}
}
}
}