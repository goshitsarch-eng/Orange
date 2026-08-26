#include "core/jsonbaserequest.h"

#include "utilities/jsonutils.h"

#include <json-glib/json-glib.h>

bool JsonBaseRequest::IsObject(const std::string &json) {
  JsonParser *parser = json_parser_new();
  const bool ok = json_parser_load_from_data(parser, json.c_str(), static_cast<gssize>(json.size()), nullptr);
  JsonNode *root = ok ? json_parser_get_root(parser) : nullptr;
  const bool object = root && JSON_NODE_HOLDS_OBJECT(root);
  g_object_unref(parser);
  return object;
}

std::string JsonBaseRequest::StringValue(const std::string &json, const std::string &key) {
  return JsonUtils::GetString(json, {key});
}
