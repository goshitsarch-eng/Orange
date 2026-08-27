#ifndef STRAWBERRY_TRANSCODEROPTIONSLABELS_H
#define STRAWBERRY_TRANSCODEROPTIONSLABELS_H

namespace TranscoderOptionsLabels {

inline const char *Title() { return "Transcoding options"; }
inline const char *Bitrate() { return "Bitrate"; }
inline const char *Kbps() { return " kbps"; }
inline const char *Quality() { return "Optimize for quality"; }
inline const char *OptimizeBitrate() { return "Optimize for bitrate"; }
inline const char *ConstantBitrate() { return "Constant bitrate"; }
inline const char *EngineQuality() { return "Encoding engine quality"; }
inline const char *Fast() { return "Fast"; }
inline const char *Standard() { return "Standard"; }
inline const char *High() { return "High"; }
inline const char *Best() { return "Best"; }
inline const char *ForceMono() { return "Force mono encoding"; }
inline const char *Profile() { return "Profile"; }
inline const char *Main() { return "Main profile (MAIN)"; }
inline const char *Lc() { return "Low complexity profile (LC)"; }
inline const char *Ssr() { return "Scalable sampling rate profile (SSR)"; }
inline const char *Ltp() { return "Long term prediction profile (LTP)"; }
inline const char *Tns() { return "Use temporal noise shaping"; }
inline const char *Midside() { return "Allow mid/side encoding"; }
inline const char *BlockType() { return "Block type"; }
inline const char *NormalBlock() { return "Normal block type"; }
inline const char *NoShort() { return "No short blocks"; }
inline const char *NoLong() { return "No long blocks"; }
inline const char *Managed() { return "Use bitrate management engine"; }
inline const char *TargetBitrate() { return "Target bitrate"; }
inline const char *MinBitrate() { return "Minimum bitrate"; }
inline const char *MaxBitrate() { return "Maximum bitrate"; }
inline const char *Disabled() { return "disabled"; }
inline const char *Automatic() { return "automatic"; }
inline const char *AverageBitrate() { return "Average bitrate"; }
inline const char *EncodingMode() { return "Encoding mode"; }
inline const char *Auto() { return "Auto"; }
inline const char *Uwb() { return "Ultra wide band (UWB)"; }
inline const char *Wb() { return "Wide band (WB)"; }
inline const char *Nb() { return "Narrow band (NB)"; }
inline const char *Vbr() { return "Variable bit rate"; }
inline const char *Vad() { return "Voice activity detection"; }
inline const char *Dtx() { return "Discontinuous transmission"; }
inline const char *Complexity() { return "Encoding complexity"; }
inline const char *Nframes() { return "Frames per buffer"; }

inline const char *EngineName(int index) {
  if (index <= 0) {
    return Fast();
  }
  if (index == 1) {
    return Standard();
  }
  return High();
}

}  // namespace TranscoderOptionsLabels

#endif
