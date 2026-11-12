// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "tts_core.h"
#include "host/wasi/vfs_io.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <regex>
#endif

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
namespace {

struct TTSSpeakerProfile {
  std::string Text;
  std::string Data;
};

// TTS function to process the prompt text.
// clang-format off
const TTSSpeakerProfile TTSDefaultSpeakerProfile = {
  // Speaker profile from edwko/OuteTTS (en_female_1.json).
  "<|text_start|>uhm<|text_sep|>now<|text_sep|>being<|text_sep|>the<|text_sep|>one<|text_sep|>to<|text_sep|>say<|text_sep|>i<|text_sep|>know<|text_sep|>the<|text_sep|>worst<|text_sep|>of<|text_sep|>you<|text_sep|>and<|text_sep|>ive<|text_sep|>been<|text_sep|>directly<|text_sep|>affected<|text_sep|>by<|text_sep|>people<|text_sep|>like<|text_sep|>you<|text_sep|>but<|text_sep|>its<|text_sep|>a<|text_sep|>clean<|text_sep|>slate<|text_sep|>with<|text_sep|>me<|text_sep|>buddy<|text_sep|>you<|text_sep|>know<|text_sep|>like<|text_sep|>thats<|text_sep|>really<|text_sep|>powerful<|text_sep|>in<|text_sep|>and<|text_sep|>of<|text_sep|>itself<|text_sep|>",
  "<|audio_start|>\nuhm<|t_0.36|><|code_start|><|447|><|223|><|967|><|301|><|965|><|827|><|393|><|908|><|764|><|1167|><|711|><|1222|><|324|><|1318|><|806|><|498|><|1198|><|1127|><|1178|><|916|><|1234|><|1411|><|1428|><|706|><|427|><|1605|><|1578|><|code_end|>\nnow<|t_0.36|><|code_start|><|1049|><|327|><|385|><|1070|><|732|><|1480|><|450|><|1025|><|1469|><|174|><|1013|><|1710|><|1674|><|775|><|771|><|251|><|778|><|1400|><|897|><|1487|><|366|><|441|><|1000|><|393|><|271|><|1000|><|768|><|code_end|>\nbeing<|t_0.27|><|code_start|><|926|><|406|><|1457|><|437|><|1231|><|672|><|1785|><|521|><|1179|><|1559|><|198|><|1086|><|733|><|122|><|1344|><|845|><|348|><|1389|><|470|><|1773|><|code_end|>\nthe<|t_0.08|><|code_start|><|1775|><|562|><|768|><|1222|><|768|><|963|><|code_end|>\none<|t_0.21|><|code_start|><|1757|><|744|><|144|><|1610|><|655|><|616|><|1317|><|225|><|1325|><|913|><|1342|><|992|><|1018|><|80|><|1777|><|883|><|code_end|>\nto<|t_0.08|><|code_start|><|487|><|1363|><|1682|><|1426|><|655|><|1483|><|code_end|>\nsay<|t_0.27|><|code_start|><|1644|><|1804|><|731|><|273|><|1592|><|731|><|1523|><|1404|><|984|><|1207|><|430|><|1132|><|1123|><|768|><|1116|><|829|><|1082|><|1095|><|440|><|1162|><|code_end|>\ni<|t_0.33|><|code_start|><|1330|><|335|><|1162|><|1155|><|308|><|1162|><|1150|><|1481|><|612|><|674|><|712|><|1745|><|1188|><|1787|><|1135|><|1275|><|1237|><|1143|><|408|><|1063|><|393|><|927|><|1298|><|132|><|1686|><|code_end|>\nknow<|t_0.27|><|code_start|><|983|><|1677|><|586|><|1528|><|1435|><|835|><|1396|><|706|><|987|><|22|><|1172|><|218|><|1404|><|1001|><|521|><|1389|><|775|><|1416|><|877|><|120|><|code_end|>\nthe<|t_0.16|><|code_start|><|916|><|1756|><|513|><|1245|><|1392|><|89|><|1266|><|12|><|1045|><|1075|><|904|><|35|><|code_end|>\nworst<|t_0.32|><|code_start|><|1607|><|174|><|1231|><|144|><|932|><|490|><|771|><|1504|><|798|><|674|><|364|><|80|><|1314|><|1636|><|449|><|1704|><|713|><|1795|><|968|><|1527|><|1302|><|1529|><|1176|><|795|><|code_end|>\nof<|t_0.12|><|code_start|><|1193|><|1205|><|390|><|1128|><|1091|><|883|><|322|><|377|><|1070|><|code_end|>\nyou<|t_0.17|><|code_start|><|1016|><|1332|><|926|><|281|><|927|><|1368|><|1687|><|918|><|67|><|1638|><|1317|><|1265|><|1770|><|code_end|>\nand<|t_0.28|><|code_start|><|1129|><|1633|><|1373|><|1207|><|405|><|879|><|1030|><|1253|><|1071|><|612|><|724|><|1770|><|665|><|1046|><|1351|><|1450|><|1541|><|1384|><|111|><|1477|><|284|><|code_end|>\nive<|t_0.35|><|code_start|><|674|><|266|><|89|><|1333|><|1183|><|1526|><|1143|><|883|><|1135|><|732|><|827|><|1119|><|594|><|1261|><|1024|><|1347|><|92|><|1392|><|825|><|1710|><|1289|><|1598|><|1070|><|1525|><|1442|><|555|><|code_end|>\nbeen<|t_0.17|><|code_start|><|1461|><|194|><|337|><|1128|><|188|><|892|><|848|><|1280|><|959|><|754|><|231|><|649|><|1304|><|code_end|>\ndirectly<|t_0.87|><|code_start|><|1030|><|353|><|570|><|1331|><|470|><|1832|><|1362|><|1809|><|1383|><|101|><|325|><|1557|><|1242|><|1512|><|180|><|227|><|1242|><|643|><|209|><|464|><|171|><|1219|><|174|><|1723|><|734|><|118|><|1269|><|643|><|209|><|187|><|612|><|1231|><|68|><|567|><|1242|><|505|><|319|><|1268|><|794|><|678|><|40|><|1286|><|470|><|1454|><|199|><|965|><|188|><|300|><|1234|><|1125|><|794|><|1289|><|1224|><|257|><|469|><|1121|><|101|><|823|><|1769|><|1683|><|95|><|255|><|59|><|67|><|832|><|code_end|>\naffected<|t_0.44|><|code_start|><|510|><|873|><|787|><|1228|><|771|><|1428|><|501|><|751|><|696|><|258|><|845|><|1818|><|1112|><|498|><|1111|><|985|><|1073|><|832|><|1427|><|168|><|163|><|447|><|119|><|567|><|1626|><|1820|><|903|><|635|><|1060|><|10|><|1632|><|35|><|1635|><|code_end|>\nby<|t_0.19|><|code_start|><|144|><|144|><|460|><|185|><|1112|><|1044|><|498|><|1192|><|656|><|1333|><|1001|><|1186|><|1186|><|454|><|code_end|>\npeople<|t_0.48|><|code_start|><|1260|><|747|><|351|><|526|><|612|><|1151|><|1262|><|1791|><|344|><|1752|><|1547|><|930|><|1302|><|1703|><|1289|><|92|><|1407|><|1482|><|508|><|1431|><|355|><|1696|><|337|><|199|><|1157|><|223|><|464|><|568|><|845|><|411|><|826|><|718|><|1786|><|545|><|712|><|580|><|code_end|>\nlike<|t_0.32|><|code_start|><|630|><|532|><|526|><|607|><|526|><|839|><|1305|><|660|><|459|><|339|><|717|><|1178|><|1148|><|687|><|149|><|1390|><|229|><|199|><|513|><|712|><|1451|><|731|><|582|><|1551|><|code_end|>\nyou<|t_0.21|><|code_start|><|1389|><|954|><|1781|><|1047|><|1236|><|930|><|809|><|1621|><|1268|><|384|><|242|><|587|><|869|><|816|><|1680|><|405|><|code_end|>\nbut<|t_0.59|><|code_start|><|1089|><|1590|><|908|><|80|><|594|><|1046|><|1706|><|1025|><|1150|><|405|><|548|><|893|><|1285|><|464|><|301|><|939|><|643|><|23|><|285|><|161|><|209|><|453|><|72|><|167|><|417|><|244|><|151|><|643|><|391|><|199|><|651|><|1023|><|337|><|1010|><|54|><|331|><|1167|><|756|><|388|><|934|><|1060|><|18|><|1624|><|1060|><|code_end|>\nits<|t_0.16|><|code_start|><|1102|><|183|><|1199|><|1258|><|1285|><|35|><|659|><|180|><|426|><|1587|><|1733|><|942|><|code_end|>\na<|t_0.04|><|code_start|><|791|><|1012|><|818|><|code_end|>\nclean<|t_0.61|><|code_start|><|1819|><|976|><|163|><|447|><|316|><|223|><|763|><|457|><|1208|><|1808|><|1697|><|1162|><|1660|><|1833|><|1054|><|1734|><|1121|><|1309|><|1643|><|924|><|1677|><|1548|><|869|><|1268|><|223|><|674|><|111|><|792|><|1670|><|912|><|174|><|1554|><|90|><|80|><|1563|><|1621|><|1698|><|1544|><|992|><|988|><|175|><|793|><|1661|><|1026|><|80|><|1761|><|code_end|>\nslate<|t_0.40|><|code_start|><|1802|><|322|><|1689|><|1577|><|1302|><|1552|><|1529|><|1722|><|1580|><|582|><|1642|><|1529|><|1020|><|582|><|1538|><|970|><|437|><|1141|><|1477|><|988|><|335|><|1611|><|922|><|1558|><|1120|><|1189|><|423|><|188|><|171|><|562|><|code_end|>\nwith<|t_0.15|><|code_start|><|963|><|1347|><|1274|><|747|><|1230|><|712|><|1408|><|1290|><|957|><|1279|><|258|><|code_end|>\nme<|t_0.09|><|code_start|><|638|><|1058|><|174|><|1452|><|1038|><|894|><|1571|><|code_end|>\nbuddy<|t_0.32|><|code_start|><|1003|><|130|><|1341|><|938|><|40|><|804|><|167|><|89|><|1456|><|1189|><|1155|><|1171|><|1434|><|1077|><|1029|><|1455|><|1622|><|1037|><|163|><|1411|><|1165|><|1463|><|837|><|1202|><|code_end|>\nyou<|t_0.36|><|code_start|><|1354|><|1165|><|615|><|1588|><|1192|><|1445|><|1033|><|982|><|401|><|1079|><|684|><|1570|><|266|><|31|><|420|><|163|><|893|><|845|><|905|><|1827|><|1804|><|153|><|627|><|243|><|1179|><|298|><|1147|><|code_end|>\nknow<|t_0.19|><|code_start|><|163|><|1542|><|1366|><|698|><|1753|><|206|><|916|><|1499|><|245|><|665|><|600|><|894|><|587|><|1741|><|code_end|>\nlike<|t_0.24|><|code_start|><|1106|><|1280|><|1062|><|1304|><|945|><|809|><|598|><|104|><|1001|><|822|><|965|><|189|><|693|><|1810|><|1293|><|199|><|1277|><|44|><|code_end|>\nthats<|t_0.24|><|code_start|><|121|><|1789|><|1443|><|370|><|1154|><|393|><|1178|><|1200|><|1264|><|424|><|1391|><|381|><|978|><|1346|><|704|><|1808|><|1579|><|1492|><|code_end|>\nreally<|t_0.56|><|code_start|><|1177|><|1761|><|1723|><|1360|><|1413|><|830|><|551|><|193|><|59|><|332|><|598|><|734|><|1684|><|1802|><|60|><|1590|><|353|><|89|><|1636|><|1396|><|893|><|143|><|455|><|1501|><|435|><|1082|><|621|><|1593|><|677|><|474|><|971|><|1513|><|913|><|828|><|1381|><|1148|><|1798|><|1186|><|1443|><|38|><|335|><|883|><|code_end|>\npowerful<|t_0.63|><|code_start|><|1773|><|458|><|1070|><|964|><|826|><|1220|><|1012|><|1738|><|1125|><|669|><|490|><|1169|><|922|><|958|><|1204|><|489|><|1001|><|886|><|1045|><|675|><|1471|><|1652|><|732|><|698|><|1124|><|480|><|897|><|1484|><|1028|><|35|><|594|><|1465|><|505|><|1669|><|436|><|851|><|1288|><|31|><|1501|><|1187|><|394|><|909|><|1541|><|1793|><|1720|><|922|><|840|><|code_end|>\nin<|t_0.16|><|code_start|><|1317|><|523|><|630|><|1343|><|1187|><|719|><|907|><|636|><|111|><|1524|><|188|><|1382|><|code_end|>\nand<|t_0.13|><|code_start|><|1074|><|922|><|1280|><|1496|><|1050|><|832|><|133|><|1435|><|1049|><|1774|><|code_end|>\nof<|t_0.12|><|code_start|><|960|><|1052|><|1192|><|1303|><|1112|><|970|><|417|><|60|><|1155|><|code_end|>\nitself<|t_0.47|><|code_start|><|1682|><|1209|><|1410|><|513|><|1222|><|861|><|167|><|406|><|1551|><|582|><|634|><|1529|><|786|><|1363|><|1578|><|1739|><|873|><|424|><|1041|><|1328|><|955|><|1110|><|1490|><|1424|><|1199|><|988|><|1162|><|1133|><|1193|><|978|><|470|><|832|><|963|><|1251|><|733|><|code_end|>",
};
// clang-format on

const std::map<int, std::string> Ones = {
    {0, "zero"},     {1, "one"},        {2, "two"},       {3, "three"},
    {4, "four"},     {5, "five"},       {6, "six"},       {7, "seven"},
    {8, "eight"},    {9, "nine"},       {10, "ten"},      {11, "eleven"},
    {12, "twelve"},  {13, "thirteen"},  {14, "fourteen"}, {15, "fifteen"},
    {16, "sixteen"}, {17, "seventeen"}, {18, "eighteen"}, {19, "nineteen"}};

const std::map<int, std::string> Tens = {
    {2, "twenty"}, {3, "thirty"},  {4, "forty"},  {5, "fifty"},
    {6, "sixty"},  {7, "seventy"}, {8, "eighty"}, {9, "ninety"}};

// TTS related functions.
void fillHannWindow(int Length, bool Periodic, float *Output) {
  int Offset = -1;
  float Pi = static_cast<float>(std::acos(-1));
  if (Periodic) {
    Offset = 0;
  }
  for (int I = 0; I < Length; I++) {
    double Value =
        0.5 *
        (1.0 - cosf(static_cast<float>((2.0 * Pi * I) / (Length + Offset))));
    Output[I] = static_cast<float>(Value);
  }
}

void twiddle(float *Real, float *Imag, int K, int N) {
  float Pi = static_cast<float>(std::acos(-1));
  float Angle = 2 * Pi * K / N;
  *Real = cos(Angle);
  *Imag = sin(Angle);
}

void irfft(int N, const float *InpCplx, float *OutReal) {
  int NN = N / 2 + 1;

  std::vector<float> RealInput(NN);
  std::vector<float> ImagInput(NN);
  for (int I = 0; I < NN; ++I) {
    RealInput[I] = InpCplx[2 * I];
    ImagInput[I] = InpCplx[2 * I + 1];
  }

  std::vector<float> RealOutput(N);
  std::vector<float> ImagOutput(N);

  for (int K = 0; K < N; ++K) {
    RealOutput[K] = 0.0f;
    ImagOutput[K] = 0.0f;
    for (int M = 0; M < NN; ++M) {
      float TwiddleReal;
      float TwiddleImag;

      twiddle(&TwiddleReal, &TwiddleImag, K * M, N);

      RealOutput[K] += RealInput[M] * TwiddleReal - ImagInput[M] * TwiddleImag;
      ImagOutput[K] += RealInput[M] * TwiddleImag + ImagInput[M] * TwiddleReal;
    }
  }

  for (int I = 0; I < N; ++I) {
    OutReal[I] = RealOutput[I] / NN;
  }
}

void fold(const std::vector<float> &Data, int64_t NOut, int64_t NWin,
          int64_t NHop, int64_t NPad, std::vector<float> &Output) {
  int64_t OutputHeight = NOut;
  int64_t KernelW = NWin;
  int64_t StrideW = NHop;
  int64_t Width = NOut;

  Output.resize(Width, 0.0f);

  int64_t ColIdx = 0;
  for (int64_t WCol = 0; WCol < Width; ++WCol) {
    int64_t Start = WCol * StrideW - NPad;
    int64_t End = Start + KernelW;

    for (int64_t WIm = Start; WIm < End; ++WIm) {
      if (WIm >= 0 && WIm < OutputHeight &&
          ColIdx < static_cast<int64_t>(Data.size())) {
        Output[WIm] += Data[ColIdx];
      }
      ColIdx++;
    }
  }

  Output.resize(NOut - 2 * NPad);
}

std::vector<float> embdToAudio(const float *Embd, const int NCodes,
                               const int NEmbd, const int NThread) {
  const int NFft = 1280;
  const int NHop = 320;
  const int NWin = 1280;
  const int NPad = (NWin - NHop) / 2;
  const int NOut = (NCodes - 1) * NHop + NWin;

  std::vector<float> Hann(NFft);

  fillHannWindow(static_cast<int>(Hann.size()), true, Hann.data());

  int NSpec = NEmbd * NCodes;

  std::vector<float> E(NSpec);
  std::vector<float> S(NSpec);
  std::vector<float> ST(NSpec);

  for (int L = 0; L < NCodes; ++L) {
    for (int K = 0; K < NEmbd; ++K) {
      E[K * NCodes + L] = Embd[L * NEmbd + K];
    }
  }

  for (int K = 0; K < NEmbd / 2; ++K) {
    for (int L = 0; L < NCodes; ++L) {
      float Mag = E[(K)*NCodes + L];
      float Phi = E[(K + NEmbd / 2) * NCodes + L];

      Mag = exp(Mag);

      if (Mag > 1e2) {
        Mag = 1e2;
      }
      S[2 * (K * NCodes + L) + 0] = Mag * cosf(Phi);
      S[2 * (K * NCodes + L) + 1] = Mag * sinf(Phi);
    }
  }

  for (int L = 0; L < NCodes; ++L) {
    for (int K = 0; K < NEmbd / 2; ++K) {
      ST[L * NEmbd + 2 * K + 0] = S[2 * (K * NCodes + L) + 0];
      ST[L * NEmbd + 2 * K + 1] = S[2 * (K * NCodes + L) + 1];
    }
  }

  std::vector<float> Res(NCodes * NFft);
  std::vector<float> Hann2(NCodes * NFft);

  std::vector<std::thread> Workers(NThread);
  for (int I = 0; I < NThread; ++I) {
    Workers[I] = std::thread([&, I]() {
      for (int L = I; L < NCodes; L += NThread) {
        irfft(NFft, ST.data() + L * NEmbd, Res.data() + L * NFft);
        for (int J = 0; J < NFft; ++J) {
          Res[L * NFft + J] *= Hann[J];
          Hann2[L * NFft + J] = Hann[J] * Hann[J];
        }
      }
    });
  }
  for (int I = 0; I < NThread; ++I) {
    Workers[I].join();
  }

  std::vector<float> Audio;
  std::vector<float> Env;

  fold(Res, NOut, NWin, NHop, NPad, Audio);
  fold(Hann2, NOut, NWin, NHop, NPad, Env); // TODO: can be done once

  for (size_t I = 0; I < Audio.size(); ++I) {
    Audio[I] /= Env[I];
  }

  return Audio;
}

struct WavHeader {
  char Riff[4] = {'R', 'I', 'F', 'F'};
  uint32_t ChunkSize;
  char Wave[4] = {'W', 'A', 'V', 'E'};
  char Fmt[4] = {'f', 'm', 't', ' '};
  uint32_t FmtChunkSize = 16;
  uint16_t AudioFormat = 1; // PCM
  uint16_t NumChannels = 1; // Mono
  uint32_t SampleRate;
  uint32_t ByteRate;
  uint16_t BlockAlign;
  uint16_t BitsPerSample = 16;
  char Data[4] = {'d', 'a', 't', 'a'};
  uint32_t DataSize;
};

std::vector<uint8_t> audioDataToWav(const std::vector<float> &Data,
                                    int SampleRate) {
  std::vector<uint8_t> WavData;
  WavHeader Header;
  Header.SampleRate = SampleRate;
  Header.ByteRate =
      Header.SampleRate * Header.NumChannels * (Header.BitsPerSample / 8);
  Header.BlockAlign = Header.NumChannels * (Header.BitsPerSample / 8);
  Header.DataSize =
      static_cast<uint32_t>(Data.size() * (Header.BitsPerSample / 8));
  Header.ChunkSize = 36 + Header.DataSize;

  WavData.insert(WavData.end(), reinterpret_cast<uint8_t *>(&Header),
                 reinterpret_cast<uint8_t *>(&Header) + sizeof(Header));

  for (const auto &Sample : Data) {
    int16_t PCMSample =
        static_cast<int16_t>(std::clamp(Sample * 32767.0, -32768.0, 32767.0));
    WavData.insert(WavData.end(), reinterpret_cast<uint8_t *>(&PCMSample),
                   reinterpret_cast<uint8_t *>(&PCMSample) + sizeof(PCMSample));
  }

  return WavData;
}

// Convert a number less than 1000 to words
std::string convertLessThanThousand(int Num) {
  std::string Result;

  if (Num >= 100) {
    Result += Ones.at(Num / 100) + " hundred ";
    Num %= 100;
  }

  if (Num >= 20) {
    Result += Tens.at(Num / 10);
    if (Num % 10 > 0) {
      Result += "-" + Ones.at(Num % 10);
    }
  } else if (Num > 0) {
    Result += Ones.at(Num);
  }

  return Result;
}

std::string numberToWords(const std::string &NumberStr) {
  try {
    size_t DecimalPos = NumberStr.find('.');
    std::string IntegerPart = NumberStr.substr(0, DecimalPos);

    int IntNumber = std::stoi(IntegerPart);
    std::string Result;

    if (IntNumber == 0) {
      Result = "zero";
    } else {
      if (IntNumber >= 1000000000) {
        int Billions = IntNumber / 1000000000;
        Result += convertLessThanThousand(Billions) + " billion ";
        IntNumber %= 1000000000;
      }

      if (IntNumber >= 1000000) {
        int Millions = IntNumber / 1000000;
        Result += convertLessThanThousand(Millions) + " million ";
        IntNumber %= 1000000;
      }

      if (IntNumber >= 1000) {
        int Thousands = IntNumber / 1000;
        Result += convertLessThanThousand(Thousands) + " thousand ";
        IntNumber %= 1000;
      }

      if (IntNumber > 0) {
        Result += convertLessThanThousand(IntNumber);
      }
    }

    // Handle decimal part
    if (DecimalPos != std::string::npos) {
      Result += " point";
      std::string DecimalPart = NumberStr.substr(DecimalPos + 1);
      for (char Digit : DecimalPart) {
        Result += " " + Ones.at(Digit - '0');
      }
    }

    return Result;
  } catch (const std::exception &) {
    // Skip if fails
    return " ";
  }
}

std::string replaceNumbersWithWords(const std::string &InputText) {
  std::regex NumberPattern(R"(\d+(\.\d+)?)");
  std::string Result;
  auto It =
      std::sregex_iterator(InputText.begin(), InputText.end(), NumberPattern);
  auto End = std::sregex_iterator();

  size_t LastPos = 0;
  for (std::sregex_iterator I = It; I != End; ++I) {
    const std::smatch &Match = *I;
    Result.append(InputText, LastPos, Match.position() - LastPos);
    Result.append(numberToWords(Match.str()));
    LastPos = Match.position() + Match.length();
  }
  Result.append(InputText, LastPos);

  return Result;
}

// Based on:
// https://github.com/edwko/OuteTTS/blob/a613e79c489d8256dd657ea9168d78de75895d82/outetts/version/v1/prompt_processor.py#L39
// https://github.com/ggerganov/llama.cpp/blob/b4488/examples/tts/tts.cpp#L374
std::string processTTSPromptText(const std::string &Text) {
  std::string ProcessedText = replaceNumbersWithWords(Text);

  std::transform(
      ProcessedText.begin(), ProcessedText.end(), ProcessedText.begin(),
      [](unsigned char C) { return static_cast<char>(::tolower(C)); });

  std::regex SpecialChars(R"([-_/,\.\\])");
  ProcessedText = std::regex_replace(ProcessedText, SpecialChars, " ");

  std::regex NonAlpha(R"([^a-z\s])");
  ProcessedText = std::regex_replace(ProcessedText, NonAlpha, "");

  std::regex MultipleSpaces(R"(\s+)");
  ProcessedText = std::regex_replace(ProcessedText, MultipleSpaces, " ");

  ProcessedText =
      std::regex_replace(ProcessedText, std::regex(R"(^\s+|\s+$)"), "");

  ProcessedText =
      std::regex_replace(ProcessedText, std::regex(R"(\s)"), "<|text_sep|>");

  return ProcessedText;
}

std::optional<TTSSpeakerProfile>
getSpeakerProfileFromFile(const std::string &FilePath, WasiNNEnvironment &Env) {
  WasmEdge::FStream::IFStream JsonFile(FilePath, Env.getEnv());
  if (!JsonFile.is_open()) {
    return std::nullopt;
  }
  try {
    nlohmann::json JsonData;
    JsonFile >> JsonData;
    JsonFile.close();

    // Initialize the outputs.
    std::string AudioOutputText = "<|audio_start|>\n";
    std::string TextOutput = "<|text_start|>";

    // Iterate through each word in the JSON data
    for (const auto &WordData : JsonData.at("words")) {
      std::string Word = WordData.at("word");
      double Duration = WordData.at("duration");
      std::vector<int> Codes = WordData.at("codes");

      // Create the audio output entry
      std::ostringstream WordEntry;
      WordEntry << Word << "<|t_" << std::fixed << std::setprecision(2)
                << Duration << "|><|code_start|>";
      for (const auto &Code : Codes) {
        WordEntry << "<|" << Code << "|>";
      }
      WordEntry << "<|code_end|>\n";
      AudioOutputText += WordEntry.str();

      // Create the text output entry
      TextOutput += Word + "<|text_sep|>";
    }

    return TTSSpeakerProfile{TextOutput, AudioOutputText};
  } catch (const std::exception &E) {
    LOG_ERROR("getSpeakerProfileFromFile: invalid speaker profile: {}"sv,
              E.what())
    return std::nullopt;
  }
}

} // namespace

std::vector<llama_token> processTTSPrompt(WasiNNEnvironment &Env,
                                          Graph &GraphRef,
                                          std::string &Prompt) noexcept {
  // Use the custom speaker profile if available.
  TTSSpeakerProfile SpeakerProfile = TTSDefaultSpeakerProfile;
  if (!GraphRef.TTSSpeakerFilePath.empty()) {
    std::optional<TTSSpeakerProfile> SpeakerProfileOpt =
        getSpeakerProfileFromFile(GraphRef.TTSSpeakerFilePath, Env);
    if (SpeakerProfileOpt.has_value()) {
      SpeakerProfile = *SpeakerProfileOpt;
    } else {
      RET_ERROR(
          {},
          "processTTSPrompt: Failed to load speaker profile from file: {}"sv,
          GraphRef.TTSSpeakerFilePath);
    }
  }
  std::string ProcessedPrompt = processTTSPromptText(Prompt);
  std::vector<llama_token> Result, TmpTokens;
  try {
    Result = common_tokenize(GraphRef.LlamaContext.get(), "<|im_start|>\n",
                             /* add_special */ true,
                             /* parse_special */ true);
    TmpTokens =
        common_tokenize(GraphRef.LlamaContext.get(), SpeakerProfile.Text,
                        /* add_special */ false,
                        /* parse_special */ true);
    Result.insert(Result.end(), TmpTokens.begin(), TmpTokens.end());
    TmpTokens = common_tokenize(GraphRef.LlamaContext.get(), ProcessedPrompt,
                                /* add_special */ false,
                                /* parse_special */ true);
    Result.insert(Result.end(), TmpTokens.begin(), TmpTokens.end());
    TmpTokens = common_tokenize(GraphRef.LlamaContext.get(), "<|text_end|>\n",
                                /* add_special */ false,
                                /* parse_special */ true);
    Result.insert(Result.end(), TmpTokens.begin(), TmpTokens.end());
    TmpTokens =
        common_tokenize(GraphRef.LlamaContext.get(), SpeakerProfile.Data,
                        /* add_special */ false,
                        /* parse_special */ true);
    Result.insert(Result.end(), TmpTokens.begin(), TmpTokens.end());
  } catch (const std::exception &E) {
    RET_ERROR({}, "processTTSPrompt: unable to tokenize the prompt: {}"sv,
              E.what())
  }

  return Result;
}

// TextToSpeech function that generates voice data from codes.
ErrNo codesToSpeech(WasiNNEnvironment &Env, Graph &GraphRef,
                    Context &CxtRef) noexcept {
  // Remove all non-audio tokens.
  CxtRef.LlamaOutputTokens.erase(
      std::remove_if(CxtRef.LlamaOutputTokens.begin(),
                     CxtRef.LlamaOutputTokens.end(),
                     [](llama_token T) { return T < 151672 || T > 155772; }),
      CxtRef.LlamaOutputTokens.end());

  // Adjust the token values for audio data.
  for (llama_token &Token : CxtRef.LlamaOutputTokens) {
    Token -= 151672;
  }

  // Put codes into batch.
  const uint32_t NCodes =
      static_cast<uint32_t>(CxtRef.LlamaOutputTokens.size());
  llama_batch TTSBatch =
      llama_batch_init(NCodes, /* embd */ 0, /* n_seq_max */ 1);
  for (uint32_t I = 0; I < NCodes; ++I) {
    common_batch_add(TTSBatch, CxtRef.LlamaOutputTokens[I], I,
                     /* seq_ids */ {0}, /* logits */ true);
  }
  if (llama_decode(GraphRef.TTSContext.get(), TTSBatch) != 0) {
    RET_ERROR(ErrNo::RuntimeError, "codesToSpeech: fail to eval."sv)
  }
  llama_batch_free(TTSBatch);

  // Get embeddings.
  const int NEmbd = llama_model_n_embd(GraphRef.TTSModel.get());
  const float *Embd = llama_get_embeddings(GraphRef.TTSContext.get());

  // Embeddings to audio.
  std::vector<float> AudioData =
      embdToAudio(Embd, NCodes, NEmbd,
                  static_cast<int>(GraphRef.Params.cpuparams.n_threads));

  // Zero out first 0.25 seconds of audio.
  const uint32_t SamplingRate = 24000;
  for (uint32_t I = 0; I < SamplingRate / 4; ++I) {
    AudioData[I] = 0.0f;
  }

  // Convert audio data to WAV and put it into the output buffer.
  CxtRef.LlamaOutputs = audioDataToWav(AudioData, SamplingRate);

  // Save .wav file if path is provided.
  if (!GraphRef.TTSOutputFilePath.empty()) {
    WasmEdge::FStream::OFStream File(GraphRef.TTSOutputFilePath,
                                     std::ios_base::out | std::ios_base::binary,
                                     Env.getEnv());
    if (!File) {
      RET_ERROR(ErrNo::RuntimeError,
                "codesToSpeech: Failed to open file '{}' for writing"sv,
                GraphRef.TTSOutputFilePath);
    }
    File.write(reinterpret_cast<const char *>(CxtRef.LlamaOutputs.data()),
               CxtRef.LlamaOutputs.size());
    File.close();
  }

  return ErrNo::Success;
}

#endif
} // namespace WasmEdge::Host::WASINN::GGML
