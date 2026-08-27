#ifndef STRAWBERRY_ORGANIZESYNTAXHIGHLIGHTER_H
#define STRAWBERRY_ORGANIZESYNTAXHIGHLIGHTER_H

#include <gtk/gtk.h>
#include <string>

class OrganizeSyntaxHighlighter {
 public:
  void Apply(GtkTextBuffer *buffer, const std::string &format);
  void Highlight(GtkTextBuffer *buffer, const std::string &format);
};

#endif
