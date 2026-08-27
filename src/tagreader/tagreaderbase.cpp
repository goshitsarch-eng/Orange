#include "tagreader/tagreaderbase.h"

float TagReaderBase::ConvertPOPMRating(int popm_rating) {
  if (popm_rating < 0x01) {
    return 0.0f;
  }
  if (popm_rating < 0x40) {
    return 0.20f;
  }
  if (popm_rating < 0x80) {
    return 0.40f;
  }
  if (popm_rating < 0xC0) {
    return 0.60f;
  }
  if (popm_rating < 0xFC) {
    return 0.80f;
  }
  return 1.0f;
}

int TagReaderBase::ConvertToPOPMRating(float rating) {
  if (rating < 0.20f) {
    return 0x00;
  }
  if (rating < 0.40f) {
    return 0x01;
  }
  if (rating < 0.60f) {
    return 0x40;
  }
  if (rating < 0.80f) {
    return 0x80;
  }
  if (rating < 1.0f) {
    return 0xC0;
  }
  return 0xFF;
}
