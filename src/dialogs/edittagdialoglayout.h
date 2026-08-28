#ifndef STRAWBERRY_EDITTAGDIALOGLAYOUT_H
#define STRAWBERRY_EDITTAGDIALOGLAYOUT_H

namespace EditTagDialogLayout {

// Floor for each tab's scrolled page.
// Without one the scrollers report almost no minimum and the dialog collapses around the action bar; with
// it the dialog opens at a usable size and still grows to fit taller pages.
inline constexpr int kMinPageHeight = 440;

}  // namespace EditTagDialogLayout

#endif
