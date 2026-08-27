#include "systemtrayicon/macsystemtrayicon.h"

#ifdef __APPLE__
#include "core/mac_startup.h"
#include "core/macstartupactions.h"
#include "systemtrayicon/systemtrayicon.h"

#import <AppKit/AppKit.h>

@interface StrawberryDockTarget : NSObject {
  SystemTrayIcon *tray_;
}
- (void)setTray:(SystemTrayIcon *)tray;
- (void)clicked:(id)sender;
@end

@implementation StrawberryDockTarget
- (void)setTray:(SystemTrayIcon *)tray {
  tray_ = tray;
}
- (void)clicked:(id)sender {
  if (!tray_) {
    return;
  }
  NSMenuItem *item = (NSMenuItem *)sender;
  SystemTrayIcon::ActivateMenuId(static_cast<int>([item tag]), &tray_->PlayPause, &tray_->Stop, &tray_->Next, &tray_->Previous,
                                 &tray_->ShowHide, &tray_->Quit, &tray_->Mute, &tray_->StopAfter, &tray_->Love);
}
@end

MacSystemTrayIcon::MacSystemTrayIcon() {
  StrawberryDockTarget *target = [[StrawberryDockTarget alloc] init];
  target_ = target;
  NSMenu *menu = [[NSMenu alloc] initWithTitle:@"DockMenu"];
  menu_ = menu;
  now_playing_ = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:MacStartupActions::NowPlayingLabel()] action:nil
                                     keyEquivalent:@""];
  [(NSMenuItem *)now_playing_ setEnabled:NO];
  artist_ = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
  [(NSMenuItem *)artist_ setEnabled:NO];
  title_ = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
  [(NSMenuItem *)title_ setEnabled:NO];
}

MacSystemTrayIcon::~MacSystemTrayIcon() {
  MacSetDockMenu(nullptr);
  [(id)now_playing_ release];
  [(id)artist_ release];
  [(id)title_ release];
  [(id)menu_ release];
  [(id)target_ release];
}

void MacSystemTrayIcon::Setup(SystemTrayIcon *tray) {
  tray_ = tray;
  [(StrawberryDockTarget *)target_ setTray:tray];
  Rebuild();
  MacSetDockMenu(menu_);
}

void MacSystemTrayIcon::Rebuild() {
  NSMenu *menu = (NSMenu *)menu_;
  [menu removeAllItems];
  if (title_ && [[(NSMenuItem *)title_ title] length] > 0) {
    [menu addItem:(NSMenuItem *)now_playing_];
    [menu addItem:(NSMenuItem *)artist_];
    [menu addItem:(NSMenuItem *)title_];
    [menu addItem:[NSMenuItem separatorItem]];
  }
  const bool show_love = tray_ ? tray_->love_visible() : true;
  for (int id : MacStartupActions::DockMenuIds(show_love)) {
    if (MacStartupActions::DockItemIsSeparator(id)) {
      [menu addItem:[NSMenuItem separatorItem]];
      continue;
    }
    const bool playing = tray_ && tray_->playing();
    NSString *label = [NSString stringWithUTF8String:SystemTrayIcon::MenuLabel(id, playing)];
    NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:label action:@selector(clicked:) keyEquivalent:@""];
    [item setTag:id];
    [item setTarget:(StrawberryDockTarget *)target_];
    [menu addItem:item];
    [item release];
  }
}

void MacSystemTrayIcon::SetNowPlaying(const Song &song) {
  [(NSMenuItem *)artist_ setTitle:[NSString stringWithUTF8String:song.artist().c_str()]];
  [(NSMenuItem *)title_ setTitle:[NSString stringWithUTF8String:song.title().c_str()]];
  Rebuild();
}

void MacSystemTrayIcon::ClearNowPlaying() {
  [(NSMenuItem *)artist_ setTitle:@""];
  [(NSMenuItem *)title_ setTitle:@""];
  Rebuild();
}
#endif
