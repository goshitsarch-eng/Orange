#ifndef STRAWBERRY_ABOUTCREDITS_H
#define STRAWBERRY_ABOUTCREDITS_H

#include <cstddef>

namespace AboutCredits {

inline const char *WindowTitle() { return "About Strawberry"; }
inline const char *AuthorName() { return "Jonas Kvinge"; }
inline const char *AuthorSection() { return "Author and maintainer"; }
inline const char *ContributorsSection() { return "Contributors"; }
inline const char *ClementineAuthorsSection() { return "Clementine authors"; }
inline const char *ClementineContributorsSection() { return "Clementine contributors"; }
inline const char *AmarokSection() { return "Amarok 1.4 authors"; }
inline const char *ThanksSection() { return "Thanks to"; }
inline const char *ThirdPartySection() { return "Built with"; }
inline const char *ThanksOthers() { return "Thanks to all the other Amarok and Clementine contributors."; }
inline const char *Description() { return "Strawberry is a music player and music collection organizer."; }
inline const char *ForkNote() { return "It is a fork of Clementine released in 2018 aimed at music collectors and audiophiles."; }
inline const char *LineageNote() {
  return "Clementine itself began in 2010 as a port of Amarok 1.4, so parts of this codebase trace back to Amarok.";
}
inline const char *Copyright() { return "© 2018-2026 Jonas Kvinge and the Strawberry contributors"; }
inline const char *GplNote() {
  return "Strawberry is free software released under GPL. The source code is available on GitHub.";
}
inline const char *LicenseNote() {
  return "You should have received a copy of the GNU General Public License along with this program.  If not, see http://www.gnu.org/licenses/";
}
inline const char *SponsorNote() { return "If you like Strawberry and can make use of it, consider sponsoring or donating."; }
inline const char *SponsorLinks() {
  return "You can sponsor the author on Patreon or GitHub. You can also make a one-time payment through paypal.me/jonaskvinge.";
}
inline const char *Website() { return "https://www.strawberrymusicplayer.org"; }
inline const char *SourceUrl() { return "https://github.com/strawberrymusicplayer/strawberry"; }
inline const char *LicenseUrl() { return "http://www.gnu.org/licenses/"; }
inline const char *PatreonUrl() { return "https://www.patreon.com/jonaskvinge"; }
inline const char *GitHubSponsorsUrl() { return "https://github.com/sponsors/jonaski"; }
inline const char *PayPalUrl() { return "https://paypal.me/jonaskvinge"; }

inline const char *Comments() {
  return "Strawberry is a music player and music collection organizer.\n"
         "It is a fork of Clementine released in 2018 aimed at music collectors and audiophiles.\n"
         "Clementine itself began in 2010 as a port of Amarok 1.4, so parts of this codebase trace back to Amarok.\n\n"
         "Strawberry is free software released under GPL. The source code is available on GitHub.\n"
         "You should have received a copy of the GNU General Public License along with this program.  If not, see http://www.gnu.org/licenses/\n\n"
         "If you like Strawberry and can make use of it, consider sponsoring or donating.\n"
         "You can sponsor the author on Patreon or GitHub. You can also make a one-time payment through paypal.me/jonaskvinge.";
}

inline size_t Count(const char **names) {
  size_t n = 0;
  while (names && names[n]) {
    ++n;
  }
  return n;
}

inline const char **Developers() {
  static const char *names[] = {"Jonas Kvinge", nullptr};
  return names;
}

inline const char **StrawberryContributors() {
  static const char *names[] = {"Gavin D. Howard",
                                "Martin Delille",
                                "Roman Lebedev",
                                "Daniel Ostertag",
                                "Gustavo L Conte",
                                "Adam Hill",
                                "Alexey Sokolov",
                                "Alexey Vazhnov",
                                "Andrei Stepanov",
                                "Andrew Tribick",
                                "Benji Hartman",
                                "Célestin Matte",
                                "Cesar Enrique Garcia Dabo",
                                "Chongo Bong",
                                "Christian Kr",
                                "Claudiu Mn",
                                "Daniel Kolesa",
                                "Edgar Salgado",
                                "Eoin O'Neill",
                                "Felipe Bugno",
                                "Fletcher Dostie",
                                "Gaganpreet Arora",
                                "Gregor Santner",
                                "Ike Devolder",
                                "Jacob Henner",
                                "Jiří Pinkava",
                                "Kientz Arnaud",
                                "Kyle Hopkins",
                                "Lars Wendler",
                                "Leandro Matheus",
                                "Madeline Schreiber",
                                "Malte Zilinski",
                                "Marcus Müller",
                                "Matteo Lo Potro",
                                "Maxime Haselbauer",
                                "Michał Walenciak",
                                "Mikalai Daronin",
                                "Mikel Pérez",
                                "Nicholas Bissell",
                                "Nicolas Toussaint",
                                "Octavio Calleya Garcia",
                                "Olivier Humbert",
                                "Ondrej Mosnáček",
                                "Pascal Below",
                                "Piper McCorkle",
                                "Robert Gingras",
                                "Robert Marshall",
                                "Rob Stanfield",
                                "Sami Boukortt",
                                "Sebastian Thomas",
                                "Sungrak Choi",
                                "Tom Kranz",
                                "William Andrea",
                                "Yaroslav Chvanov",
                                "Alex Bikadorov",
                                nullptr};
  return names;
}

inline const char **ClementineAuthors() {
  static const char *names[] = {"David Sansome", "John Maguire", "Paweł Bara", "Arnaud Bienner", nullptr};
  return names;
}

inline const char **ClementineContributors() {
  static const char *names[] = {"Jakub Stachowski",
                                "Paul Cifarelli",
                                "Felipe Rivera",
                                "Alexander Peitz",
                                "Andreas Muttscheller",
                                "Mark Furneaux",
                                "Florian Bigard",
                                "Mattias Andersson",
                                "Alan Briolat",
                                "Arun Narayanankutty",
                                "Bartłomiej Burdukiewicz",
                                "Andre Siviero",
                                "Santiago Gil",
                                "Tyler Rhodes",
                                "Vikram Ambrose",
                                "David Guillen",
                                "Krzysztof Sobiecki",
                                "Valeriy Malov",
                                "Nick Lanham",
                                nullptr};
  return names;
}

// Strawberry is a fork of Clementine, and Clementine started life as a port of Amarok 1.4.
// Amarok's original authors are credited here because code and design from Amarok 1.4 survives in this
// codebase by way of Clementine.
inline const char **AmarokAuthors() {
  static const char *names[] = {"Mark Kretschmann", "Max Howell", "Ian Monroe", "Alexandre Oliveira",
                                "Christian Muehlhaeuser", "Seb Ruiz", "Gábor Lehel", "Nikolaj Hald Nielsen",
                                "Dan Meltzer", "Jeff Mitchell", nullptr};
  return names;
}

// Third-party projects Strawberry is built on.
// This list is kept in sync with the dependencies declared in CMakeLists.txt.
inline const char **ThirdParty() {
  static const char *names[] = {"GTK — https://www.gtk.org/",
                                "libadwaita — https://gnome.pages.gitlab.gnome.org/libadwaita/",
                                "GLib / GObject / GIO — https://gitlab.gnome.org/GNOME/glib",
                                "GStreamer — https://gstreamer.freedesktop.org/",
                                "TagLib — https://taglib.org/",
                                "SQLite — https://www.sqlite.org/",
                                "libsoup — https://libsoup.gnome.org/",
                                "json-glib — https://gitlab.gnome.org/GNOME/json-glib",
                                "ICU — https://icu.unicode.org/",
                                "Boost — https://www.boost.org/",
                                "libsecret — https://gitlab.gnome.org/GNOME/libsecret",
                                "ALSA — https://www.alsa-project.org/",
                                "PulseAudio — https://www.freedesktop.org/wiki/Software/PulseAudio/",
                                "Chromaprint / AcoustID — https://acoustid.org/chromaprint",
                                "MusicBrainz — https://musicbrainz.org/",
                                "libebur128 — https://github.com/jiixyj/libebur128",
                                "FFTW — https://www.fftw.org/",
                                "libcdio — https://www.gnu.org/software/libcdio/",
                                "libmtp — https://libmtp.sourceforge.net/",
                                "libgpod — https://www.gtkpod.org/libgpod/",
                                nullptr};
  return names;
}

inline const char **ThanksTo() {
  static const char *names[] = {"Mark Kretschmann", "Max Howell", "Artur Rona", "Robert-André Mauchin", "Thomas Pierson", "Fabio Loli",
                                nullptr};
  return names;
}

}  // namespace AboutCredits

#endif
