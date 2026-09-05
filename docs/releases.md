# Building and installing Orange releases

## Automatic publishing

`cmake/Version.cmake` is the version source. Update its major/minor/patch fields together with README, Changelog, AppStream metadata and `tests/release_identity_test.cmake`, then push to `master`.

[Orange CI](../.github/workflows/orange-ci.yml) reads that version, compiles Orange and runs the offline test suite on native x86_64 and ARM64 runners. If the version has no published GitHub release, it also builds and verifies both Flatpak bundles. Only after all four build jobs pass does it create `v<VERSION>` at the tested commit, upload the complete asset set to a draft, and publish it. No personal access token is required: the release job uses GitHub's scoped `GITHUB_TOKEN` with `contents: write`.

A normal push with an already published version runs the native builds/tests without replacing that release. Feature branches and pull requests can build packages but cannot publish. Re-running an interrupted unpublished release resumes the draft; an existing version tag is never moved to another commit. Use the workflow's manual Run workflow action on `master` to retry if needed. Increment the version again for subsequent published fixes.

| Download | Contents |
| --- | --- |
| `orange-<VERSION>-linux-x86_64.tar.gz` | Linux x64 executable, application data, translations and desktop integration |
| `orange-<VERSION>-linux-aarch64.tar.gz` | Linux ARM64 executable, application data, translations and desktop integration |
| `orange-<VERSION>-x86_64.flatpak` | x64 Flatpak bundle |
| `orange-<VERSION>-aarch64.flatpak` | ARM64 Flatpak bundle |
| `orange-<VERSION>-source.tar.gz` | Source from the exact release commit |
| `SHA256SUMS` | SHA-256 checksums for all five downloads |

This publishes bundles on GitHub. It does not submit to Flathub or maintain an automatic-update Flatpak repository; install a newer downloaded bundle to update.

## Flatpak installation

Choose the bundle for your CPU (`uname -m` reports `x86_64` or `aarch64`). With Flatpak installed:

```sh
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user ./orange-2.1.6-x86_64.flatpak
flatpak run com.goshapps.Orange
```

The bundle uses the KDE 6.11 runtime. Flatpak downloads required runtime components from Flathub. For ARM64, substitute `aarch64` in the filename. Music-folder, removable-media and mounted-share access is declared in the [manifest](../dist/flatpak/com.goshapps.Orange.yml); grant access to another music location in your Flatpak permissions manager if required. The app ID remains `com.goshapps.Orange`.

The manifest builds the checked-out Orange source, with archive dependencies pinned by checksum. Optional iPod support is disabled in the Flatpak build; native archives include it when the build dependencies are available. The native builds run the full offline suite, while Flatpak verifies the exported application starts and reports the correct version.

## Linux tar.gz installation

These are **native install archives, not self-contained portable bundles**. They are built on Ubuntu 24.04 and require compatible Qt, GStreamer and other system libraries. Use Flatpak if your distribution has different library versions. KDSingleApplication is linked statically because Ubuntu 24.04 ships an older version. `DEPENDENCIES.txt` in each archive records the build machine's resolved shared libraries.

On Ubuntu 24.04, install the runtime dependencies:

```sh
sudo apt-get install libqt6widgets6t64 libqt6concurrent6t64 libqt6network6t64 libqt6sql6-sqlite \
  libqt6dbus6t64 libtag1v5 libicu74 libasound2t64 \
  libpulse0 libchromaprint1 libfftw3-double3 libebur128-1 libcdio19t64 \
  libmtp9 libgpod4t64 gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav
```

Extract the archive, inspect its contents, then copy its installation tree into `/usr/local`:

```sh
tar -xzf orange-2.1.6-linux-x86_64.tar.gz
sudo cp -a orange-2.1.6-linux-x86_64/bin orange-2.1.6-linux-x86_64/share /usr/local/
orange --version
orange
```

For ARM64, substitute `aarch64`. Keep the archive's file list if you need to remove this manual installation later. Existing user settings and music are not part of the archive.

## Building a Flatpak locally

```sh
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.kde.Platform//6.11 org.kde.Sdk//6.11
flatpak-builder --user --force-clean --repo=flatpak-repo --default-branch=stable \
  flatpak-build dist/flatpak/com.goshapps.Orange.yml
flatpak build-bundle --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo \
  flatpak-repo orange-local.flatpak com.goshapps.Orange stable
```

Build on the target CPU architecture. GitHub uses `ubuntu-24.04` for x86_64 and `ubuntu-24.04-arm` for ARM64; see [GitHub's runner reference](https://docs.github.com/en/actions/reference/runners/github-hosted-runners). Bundle creation follows the [Flatpak builder reference](https://docs.flatpak.org/en/latest/flatpak-builder-command-reference.html).
