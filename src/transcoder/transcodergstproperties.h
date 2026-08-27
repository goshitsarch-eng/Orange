#ifndef STRAWBERRY_TRANSCODERGSTPROPERTIES_H
#define STRAWBERRY_TRANSCODERGSTPROPERTIES_H

#include "core/settings.h"
#include "transcoder/transcoder.h"
#include "transcoder/transcoderoptionsfields.h"

#include <glib-object.h>
#include <gst/gst.h>

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

namespace TranscoderGstProperties {

inline std::string GroupForFactory(const std::string &factory) { return factory.empty() ? std::string() : "Transcoder/" + factory; }

inline bool ShouldApplyFactory(const std::string &factory) {
  return !factory.empty() && factory != "filesrc" && factory != "filesink" && factory != "decodebin" &&
         factory != "audioconvert" && factory != "audioresample";
}

inline std::vector<std::string> GroupsToApply(Transcoder::Format format, const std::string &factory) {
  std::vector<std::string> groups;
  const auto add = [&groups](const std::string &group) {
    if (group.empty()) {
      return;
    }
    if (std::find(groups.begin(), groups.end(), group) == groups.end()) {
      groups.push_back(group);
    }
  };
  add(GroupForFactory(factory));
  add(TranscoderOptionsFields::GroupFor(format));
  if (const char *legacy = TranscoderOptionsFields::LegacyGroupFor(format)) {
    add(legacy);
  }
  return groups;
}

inline bool ParseBool(const std::string &value) {
  return value == "true" || value == "TRUE" || value == "1" || value == "yes";
}

inline bool ApplyStored(GObject *object, GParamSpec *property, Settings *settings) {
  if (!object || !property || !settings || !settings->Contains(property->name)) {
    return false;
  }
  switch (property->value_type) {
    case G_TYPE_FLOAT:
      g_object_set(object, property->name, static_cast<gfloat>(settings->DoubleValue(property->name)), nullptr);
      return true;
    case G_TYPE_DOUBLE:
      g_object_set(object, property->name, settings->DoubleValue(property->name), nullptr);
      return true;
    case G_TYPE_BOOLEAN:
      g_object_set(object, property->name, settings->BoolValue(property->name, ParseBool(settings->Value(property->name))),
                   nullptr);
      return true;
    case G_TYPE_INT:
    case G_TYPE_ENUM:
      g_object_set(object, property->name, settings->IntValue(property->name), nullptr);
      return true;
    case G_TYPE_UINT:
      g_object_set(object, property->name, static_cast<guint>(settings->IntValue(property->name)), nullptr);
      return true;
    case G_TYPE_INT64:
    case G_TYPE_LONG:
      g_object_set(object, property->name, static_cast<gint64>(settings->Int64Value(property->name)), nullptr);
      return true;
    case G_TYPE_UINT64:
    case G_TYPE_ULONG:
      g_object_set(object, property->name, static_cast<guint64>(settings->Int64Value(property->name)), nullptr);
      return true;
    case G_TYPE_STRING:
      g_object_set(object, property->name, settings->Value(property->name).c_str(), nullptr);
      return true;
    default:
      g_object_set(object, property->name, settings->IntValue(property->name), nullptr);
      return true;
  }
}

inline int ApplyGroup(GObject *object, const std::string &group) {
  if (!object || group.empty()) {
    return 0;
  }
  Settings settings;
  settings.BeginGroup(group);
  guint count = 0;
  GParamSpec **properties = g_object_class_list_properties(G_OBJECT_GET_CLASS(object), &count);
  int applied = 0;
  for (guint i = 0; i < count; ++i) {
    if (ApplyStored(object, properties[i], &settings)) {
      ++applied;
    }
  }
  g_free(properties);
  return applied;
}

inline std::string FactoryName(GstElement *element) {
  if (!element) {
    return {};
  }
  GstElementFactory *factory = gst_element_get_factory(element);
  const gchar *name = factory ? GST_OBJECT_NAME(factory) : nullptr;
  return name ? name : "";
}

inline int ApplyStoredProperties(GstElement *pipeline, Transcoder::Format format) {
  if (!pipeline || !GST_IS_BIN(pipeline)) {
    return 0;
  }
  int applied = 0;
  GstIterator *it = gst_bin_iterate_recurse(GST_BIN(pipeline));
  GValue item = G_VALUE_INIT;
  while (gst_iterator_next(it, &item) == GST_ITERATOR_OK) {
    GstElement *element = GST_ELEMENT(g_value_get_object(&item));
    const std::string factory = FactoryName(element);
    if (ShouldApplyFactory(factory)) {
      for (const std::string &group : GroupsToApply(format, factory)) {
        applied += ApplyGroup(G_OBJECT(element), group);
      }
    }
    g_value_unset(&item);
  }
  gst_iterator_free(it);
  return applied;
}

}  // namespace TranscoderGstProperties

#endif  // STRAWBERRY_TRANSCODERGSTPROPERTIES_H
