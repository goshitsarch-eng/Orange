#include "radios/radiobrowserservice.h"

#include "radios/radiobrowsersearchopts.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <json-glib/json-glib.h>

const std::vector<std::string> RadioBrowserService::kServers = {"de1.api.radio-browser.info", "de2.api.radio-browser.info"};

std::string RadioBrowserService::Homepage() { return "https://www.radio-browser.info/"; }

std::string RadioBrowserService::Donate() { return "https://www.radio-browser.info/"; }

std::string RadioBrowserService::DefaultServer() { return "https://" + kServers.front(); }

namespace {

std::string HostFromServer(std::string host) {
  if (StrUtils::StartsWith(host, "https://")) {
    host = host.substr(8);
  } else if (StrUtils::StartsWith(host, "http://")) {
    host = host.substr(7);
  }
  if (!host.empty() && host.back() == '/') {
    host.pop_back();
  }
  if (host.empty()) {
    host = RadioBrowserService::kServers.front();
  }
  return host;
}

}  // namespace

std::string RadioBrowserService::SearchUrl(const std::string &server, const std::string &query, const std::string &country,
                                           bool hide_broken, int limit, int offset, const std::string &order) {
  const std::string host = HostFromServer(server);
  std::string url = "https://" + host + "/json/stations/search?limit=" + std::to_string(limit) + "&offset=" + std::to_string(offset);
  if (!query.empty()) {
    url += "&name=" + StrUtils::UriEscape(query);
  }
  if (!country.empty()) {
    url += "&countrycode=" + StrUtils::UriEscape(country);
  }
  if (hide_broken) {
    url += "&hidebroken=true";
  }
  const std::string sort = order.empty() ? RadioBrowserSearchOpts::DefaultSort() : order;
  url += "&order=" + StrUtils::UriEscape(sort);
  if (RadioBrowserSearchOpts::ReverseOrder(sort)) {
    url += "&reverse=true";
  }
  return url;
}

std::string RadioBrowserService::CountriesUrl(const std::string &server) {
  return "https://" + HostFromServer(server) + "/json/countrycodes";
}

std::vector<RadioChannel> RadioBrowserService::ParseStations(const std::string &json) { return ParseStationPage(json).channels; }

RadioBrowserService::StationPage RadioBrowserService::ParseStationPage(const std::string &json) {
  StationPage page;
  if (json.empty()) {
    return page;
  }
  JsonParser *parser = json_parser_new();
  if (!json_parser_load_from_data(parser, json.data(), static_cast<gssize>(json.size()), nullptr)) {
    g_object_unref(parser);
    return page;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_ARRAY(root)) {
    g_object_unref(parser);
    return page;
  }
  JsonArray *array = json_node_get_array(root);
  const guint n = json_array_get_length(array);
  page.raw_count = static_cast<int>(n);
  std::vector<RadioChannel> &channels = page.channels;
  for (guint i = 0; i < n; ++i) {
    JsonNode *item = json_array_get_element(array, i);
    if (!item || !JSON_NODE_HOLDS_OBJECT(item)) {
      continue;
    }
    JsonObject *object = json_node_get_object(item);
    const char *name_raw = json_object_has_member(object, "name") ? json_object_get_string_member(object, "name") : nullptr;
    const std::string name = StrUtils::Trim(name_raw ? name_raw : "");
    if (name.empty()) {
      continue;
    }
    const char *resolved = json_object_has_member(object, "url_resolved") ? json_object_get_string_member(object, "url_resolved") : nullptr;
    const char *url = json_object_has_member(object, "url") ? json_object_get_string_member(object, "url") : nullptr;
    std::string stream = resolved && *resolved ? resolved : (url ? url : "");
    if (stream.empty()) {
      continue;
    }
    RadioChannel channel;
    channel.source = Song::Source::RadioBrowser;
    channel.name = name;
    channel.url = stream;
    if (json_object_has_member(object, "favicon")) {
      const char *favicon = json_object_get_string_member(object, "favicon");
      if (favicon) {
        channel.thumbnail_url = favicon;
      }
    }
    if (json_object_has_member(object, "country")) {
      const char *country = json_object_get_string_member(object, "country");
      if (country) {
        channel.country = StrUtils::Trim(country);
      }
    }
    if (json_object_has_member(object, "tags")) {
      const char *tags = json_object_get_string_member(object, "tags");
      if (tags) {
        channel.tags = StrUtils::Trim(tags);
      }
    }
    if (json_object_has_member(object, "codec")) {
      const char *codec = json_object_get_string_member(object, "codec");
      if (codec) {
        channel.codec = StrUtils::Trim(codec);
      }
    }
    channels.push_back(channel);
  }
  g_object_unref(parser);
  return page;
}

std::vector<RadioBrowserService::Country> RadioBrowserService::ParseCountries(const std::string &json) {
  std::vector<Country> countries;
  if (json.empty()) {
    return countries;
  }
  JsonParser *parser = json_parser_new();
  if (!json_parser_load_from_data(parser, json.data(), static_cast<gssize>(json.size()), nullptr)) {
    g_object_unref(parser);
    return countries;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_ARRAY(root)) {
    g_object_unref(parser);
    return countries;
  }
  JsonArray *array = json_node_get_array(root);
  const guint n = json_array_get_length(array);
  for (guint i = 0; i < n; ++i) {
    JsonNode *item = json_array_get_element(array, i);
    if (!item || !JSON_NODE_HOLDS_OBJECT(item)) {
      continue;
    }
    JsonObject *object = json_node_get_object(item);
    const char *name_raw = json_object_has_member(object, "name") ? json_object_get_string_member(object, "name") : nullptr;
    std::string code = StrUtils::ToUpper(StrUtils::Trim(name_raw ? name_raw : ""));
    if (code.empty()) {
      continue;
    }
    int count = 0;
    if (json_object_has_member(object, "stationcount")) {
      count = static_cast<int>(json_object_get_int_member(object, "stationcount"));
    }
    if (count <= 0) {
      continue;
    }
    Country country;
    country.code = code;
    country.name = code;
    country.stationcount = count;
    countries.push_back(country);
  }
  g_object_unref(parser);
  std::sort(countries.begin(), countries.end(), [](const Country &a, const Country &b) { return a.code < b.code; });
  return countries;
}
