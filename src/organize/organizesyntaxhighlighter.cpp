#include "organize/organizesyntaxhighlighter.h"

#include "organize/organizeformat.h"

void OrganizeSyntaxHighlighter::Apply(GtkTextBuffer *buffer, const std::string &format) {
  if (!buffer) {
    return;
  }
  gtk_text_buffer_set_text(buffer, format.c_str(), static_cast<int>(format.size()));
  Highlight(buffer, format);
}

void OrganizeSyntaxHighlighter::Highlight(GtkTextBuffer *buffer, const std::string &format) {
  if (!buffer) {
    return;
  }
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
  GtkTextTag *tag = gtk_text_tag_table_lookup(table, "token");
  if (!tag) {
    tag = gtk_text_buffer_create_tag(buffer, "token", "foreground", "#3584e4", "weight", 700, nullptr);
  }
  GtkTextIter start_all;
  GtkTextIter end_all;
  gtk_text_buffer_get_bounds(buffer, &start_all, &end_all);
  gtk_text_buffer_remove_tag(buffer, tag, &start_all, &end_all);
  for (int i = 0; OrganizeFormat::kKnownTags[i]; ++i) {
    const std::string token = OrganizeFormat::kKnownTags[i];
    size_t pos = 0;
    while ((pos = format.find(token, pos)) != std::string::npos) {
      GtkTextIter start;
      GtkTextIter end;
      gtk_text_buffer_get_iter_at_offset(buffer, &start, static_cast<int>(pos));
      gtk_text_buffer_get_iter_at_offset(buffer, &end, static_cast<int>(pos + token.size()));
      gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
      pos += token.size();
    }
  }
}
