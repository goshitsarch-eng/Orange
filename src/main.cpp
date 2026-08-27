#include "config.h"
#include "version.h"

#include "core/application.h"
#include "core/commandlineoptions.h"
#include "core/logging.h"
#include "engine/gststartup.h"
#include "translations/translations.h"
#include "ui/mainwindow.h"

#include <adwaita.h>
#include <gst/gst.h>

#include <memory>

namespace {

struct Runtime {
  std::unique_ptr<Application> app;
  std::unique_ptr<MainWindow> window;
};

void Activate(AdwApplication *gtk_app, gpointer user_data) {
  auto *runtime = static_cast<Runtime *>(user_data);
  if (runtime->window) {
    runtime->window->Present();
    return;
  }
  runtime->app = std::make_unique<Application>();
  runtime->app->Init();
  CommandlineOptions empty;
  runtime->window = std::make_unique<MainWindow>(gtk_app, runtime->app.get(), empty);
  runtime->window->Present();
}

int CommandLine(GApplication *gapp, GApplicationCommandLine *cmdline, gpointer user_data) {
  auto *runtime = static_cast<Runtime *>(user_data);
  int argc = 0;
  char **argv = g_application_command_line_get_arguments(cmdline, &argc);
  CommandlineOptions options;
  options.Parse(argc, argv);
  g_strfreev(argv);
  if (options.version()) {
    g_application_command_line_print(cmdline, "Strawberry %s\n", STRAWBERRY_VERSION_DISPLAY);
    return 0;
  }
  if (options.debug() || options.log_levels().find("*:4") != std::string::npos) {
    logging::SetDebugEnabled(true);
  }
  if (!options.log_levels().empty()) {
    logging::SetLevels(options.log_levels());
  }
  Activate(ADW_APPLICATION(gapp), user_data);
  if (runtime->window) {
    runtime->window->CommandlineReceived(options);
  }
  return 0;
}

void Open(GApplication *gapp, gpointer files, gint n_files, const gchar *, gpointer user_data) {
  auto *runtime = static_cast<Runtime *>(user_data);
  Activate(ADW_APPLICATION(gapp), user_data);
  CommandlineOptions options;
  std::vector<std::string> urls;
  auto **gfiles = static_cast<GFile **>(files);
  for (int i = 0; i < n_files; ++i) {
    gchar *uri = g_file_get_uri(gfiles[i]);
    urls.emplace_back(uri);
    g_free(uri);
  }
  options.set_urls(urls);
  runtime->window->CommandlineReceived(options);
}

}  // namespace

int main(int argc, char **argv) {
  logging::Init();

  CommandlineOptions options;
  if (!options.Parse(argc, argv)) {
    return 1;
  }
  if (options.version()) {
    g_print("Strawberry %s\n", STRAWBERRY_VERSION_DISPLAY);
    return 0;
  }
  if (options.debug()) {
    logging::SetDebugEnabled(true);
  }
  if (!options.log_levels().empty()) {
    logging::SetLevels(options.log_levels());
  }

  GstStartup::Initialize();
  Translations::ApplySavedLanguage();
  Translations::Init();
  adw_init();

  Runtime runtime;
  AdwApplication *gtk_app =
      adw_application_new(STRAWBERRY_APPLICATION_ID,
                          static_cast<GApplicationFlags>(G_APPLICATION_HANDLES_OPEN | G_APPLICATION_HANDLES_COMMAND_LINE));
  g_application_set_application_id(G_APPLICATION(gtk_app), STRAWBERRY_APPLICATION_ID);
  g_signal_connect(gtk_app, "activate", G_CALLBACK(Activate), &runtime);
  g_signal_connect(gtk_app, "open", G_CALLBACK(Open), &runtime);
  g_signal_connect(gtk_app, "command-line", G_CALLBACK(CommandLine), &runtime);

  const int status = g_application_run(G_APPLICATION(gtk_app), argc, argv);
  if (runtime.app) {
    runtime.app->Exit();
  }
  g_object_unref(gtk_app);
  return status;
}
