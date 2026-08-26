#ifndef STRAWBERRY_DIALOGHELPERS_H
#define STRAWBERRY_DIALOGHELPERS_H

#include "core/application.h"
#include "core/song.h"

#include <gtk/gtk.h>

#include <string>
#include <vector>

namespace DialogHelpers {

Song SongForDialog(Application *app);
void SetImageFromBytes(GtkWidget *image, const std::vector<unsigned char> &data, int pixel_size);
std::string PrettyBytes(int64_t bytes);
std::string PrettyUnixTime(int64_t ts);
std::string SafeFolderName(std::string name);
GtkWidget *DropDownFromNames(const std::vector<std::string> &names);
bool ApplyCover(Application *app, Song *song, const std::string &image);

}  // namespace DialogHelpers

#endif
