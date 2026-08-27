#include "device/mtpconnection.h"

#include "core/logging.h"
#include "device/devicecopysupported.h"

#include <cstdlib>

bool MtpConnection::initialized_ = false;

void MtpConnection::InitLibMtp() {
#ifdef HAVE_MTP
  if (!initialized_) {
    LIBMTP_Init();
    initialized_ = true;
  }
#endif
}

MtpConnection::MtpConnection() { InitLibMtp(); }

MtpConnection::~MtpConnection() { Close(); }

bool MtpConnection::is_valid() const {
#ifdef HAVE_MTP
  return device_ != nullptr;
#else
  return false;
#endif
}

void MtpConnection::Close() {
#ifdef HAVE_MTP
  if (device_) {
    LIBMTP_Release_Device(device_);
    device_ = nullptr;
  }
#endif
}

#ifdef HAVE_MTP
std::string MtpConnection::ErrorString(LIBMTP_error_number_t error_number) {
  switch (error_number) {
    case LIBMTP_ERROR_NO_DEVICE_ATTACHED:
      return "No Devices have been found.";
    case LIBMTP_ERROR_CONNECTING:
      return "There has been an error connecting.";
    case LIBMTP_ERROR_MEMORY_ALLOCATION:
      return "Memory Allocation Error.";
    case LIBMTP_ERROR_NONE:
      return "Successfully connected.";
    case LIBMTP_ERROR_GENERAL:
    default:
      return "Unknown error, please report this to the libmtp developers.";
  }
}

Song::FileType MtpConnection::FileTypeFromMtp(LIBMTP_filetype_t type) {
  switch (type) {
    case LIBMTP_FILETYPE_WAV:
      return Song::FileType::WAV;
    case LIBMTP_FILETYPE_MP2:
    case LIBMTP_FILETYPE_MP3:
      return Song::FileType::MPEG;
    case LIBMTP_FILETYPE_WMA:
      return Song::FileType::ASF;
    case LIBMTP_FILETYPE_MP4:
    case LIBMTP_FILETYPE_M4A:
    case LIBMTP_FILETYPE_AAC:
      return Song::FileType::MP4;
    case LIBMTP_FILETYPE_FLAC:
      return Song::FileType::OggFlac;
    case LIBMTP_FILETYPE_OGG:
      return Song::FileType::OggVorbis;
    default:
      return Song::FileType::Unknown;
  }
}

LIBMTP_filetype_t MtpConnection::MtpFileTypeFromSong(Song::FileType type) {
  switch (type) {
    case Song::FileType::ASF:
      return LIBMTP_FILETYPE_ASF;
    case Song::FileType::MP4:
      return LIBMTP_FILETYPE_MP4;
    case Song::FileType::MPEG:
      return LIBMTP_FILETYPE_MP3;
    case Song::FileType::FLAC:
    case Song::FileType::OggFlac:
      return LIBMTP_FILETYPE_FLAC;
    case Song::FileType::OggSpeex:
    case Song::FileType::OggVorbis:
      return LIBMTP_FILETYPE_OGG;
    case Song::FileType::WAV:
      return LIBMTP_FILETYPE_WAV;
    default:
      return LIBMTP_FILETYPE_UNDEF_AUDIO;
  }
}
#endif

bool MtpConnection::OpenBySerial(const std::string &serial) {
#ifdef HAVE_MTP
  Close();
  LIBMTP_raw_device_t *raw = nullptr;
  int count = 0;
  const LIBMTP_error_number_t error = LIBMTP_Detect_Raw_Devices(&raw, &count);
  if (error != LIBMTP_ERROR_NONE || !raw) {
    error_text_ = ErrorString(error);
    return false;
  }
  bool opened = false;
  for (int i = 0; i < count; ++i) {
    LIBMTP_mtpdevice_t *device = LIBMTP_Open_Raw_Device_Uncached(&raw[i]);
    if (!device) {
      continue;
    }
    char *value = LIBMTP_Get_Serialnumber(device);
    const std::string id = value ? value : "";
    free(value);
    if (id == serial || (serial.empty() && count == 1)) {
      device_ = device;
      opened = true;
      break;
    }
    LIBMTP_Release_Device(device);
  }
  free(raw);
  if (!opened) {
    error_text_ = "MTP device not found.";
  }
  return opened;
#else
  (void)serial;
  error_text_ = "MTP support is not enabled.";
  return false;
#endif
}

bool MtpConnection::OpenFirst() { return OpenBySerial({}); }

std::string MtpConnection::serial() const {
#ifdef HAVE_MTP
  if (!device_) {
    return {};
  }
  char *value = LIBMTP_Get_Serialnumber(device_);
  std::string result = value ? value : "";
  free(value);
  return result;
#else
  return {};
#endif
}

std::string MtpConnection::friendly_name() const {
#ifdef HAVE_MTP
  if (!device_) {
    return {};
  }
  char *value = LIBMTP_Get_Friendlyname(device_);
  std::string result = value && *value ? value : "MTP device";
  free(value);
  return result;
#else
  return {};
#endif
}

std::vector<Song::FileType> MtpConnection::SupportedFiletypes() const {
  std::vector<Song::FileType> types;
#ifdef HAVE_MTP
  if (!device_) {
    return types;
  }
  uint16_t *list = nullptr;
  uint16_t length = 0;
  if (LIBMTP_Get_Supported_Filetypes(device_, &list, &length) != 0 || !list || !length) {
    return types;
  }
  for (uint16_t i = 0; i < length; ++i) {
    DeviceCopySupported::AppendFromMtp(static_cast<LIBMTP_filetype_t>(list[i]), &types);
  }
  free(list);
#endif
  return types;
}
