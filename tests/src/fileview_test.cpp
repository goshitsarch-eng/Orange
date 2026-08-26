#include "fileview/fileviewtreemodel.h"
#include "utilities/fileutils.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <unistd.h>

#include <string>

namespace {

std::string TempDir() {
  char path[] = "/tmp/strawberry-fileview-XXXXXX";
  return mkdtemp(path);
}

}  // namespace

TEST(FileViewTreeModel, RootPathsAndFilesIn) {
  const std::string dir = TempDir();
  const std::string audio = FileUtils::Join(dir, "song.flac");
  const std::string other = FileUtils::Join(dir, "notes.txt");
  const std::string hidden = FileUtils::Join(dir, ".hidden.flac");
  ASSERT_TRUE(FileUtils::WriteFile(audio, "a"));
  ASSERT_TRUE(FileUtils::WriteFile(other, "b"));
  ASSERT_TRUE(FileUtils::WriteFile(hidden, "c"));

  FileViewTreeModel model;
  model.SetRootPaths({dir});
  EXPECT_EQ(1, model.DirectoryCount());
  ASSERT_TRUE(model.root());
  EXPECT_EQ(FileViewTreeItem::Type::Root, model.root()->type);

  const std::vector<std::string> files = model.FilesIn(dir);
  EXPECT_NE(std::find(files.begin(), files.end(), audio), files.end());
  EXPECT_EQ(std::find(files.begin(), files.end(), other), files.end());
  EXPECT_EQ(std::find(files.begin(), files.end(), hidden), files.end());

  FileUtils::Remove(audio);
  FileUtils::Remove(other);
  FileUtils::Remove(hidden);
  rmdir(dir.c_str());
}

TEST(FileViewTreeModel, NameFiltersIncludeExtraExtensions) {
  const std::string dir = TempDir();
  const std::string cue = FileUtils::Join(dir, "album.cue");
  const std::string txt = FileUtils::Join(dir, "readme.txt");
  ASSERT_TRUE(FileUtils::WriteFile(cue, "PERFORMER \"A\""));
  ASSERT_TRUE(FileUtils::WriteFile(txt, "hi"));

  FileViewTreeModel model;
  model.SetNameFilters({"txt"});
  const std::vector<std::string> files = model.FilesIn(dir);
  EXPECT_NE(std::find(files.begin(), files.end(), cue), files.end());
  EXPECT_NE(std::find(files.begin(), files.end(), txt), files.end());

  FileUtils::Remove(cue);
  FileUtils::Remove(txt);
  rmdir(dir.c_str());
}

TEST(FileViewTreeModel, MissingRootIsIgnored) {
  FileViewTreeModel model;
  model.SetRootPaths({"/tmp/does-not-exist-strawberry-fileview"});
  EXPECT_EQ(0, model.DirectoryCount());
}
