#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Check that an iOS application (a built .app, or an exported .ipa) is complete.
#
#   libs/platforms/ios/packaging/verify_ipa.sh <WordJewelsSolver.app | package.ipa>
#       [--version <x.y.z>] [--build <n>] [--signed] [--store]
#
# What it looks for:
#
#   * Info.plist is valid, names the identifier, the executable, the version, the
#     icon set and the camera usage text (a program that asks for the camera
#     without it is ended by iOS), and says it is an iPhone/iPad program;
#   * the compiled asset catalogue (the icon) is in the bundle;
#   * the dictionary and the dice are beside the executable, which is the
#     bundle's root;
#   * the executable is for arm64 and links nothing outside the system: Qt is
#     static on iOS, so a link to a Qt framework or to a path on the build
#     machine means the wrong kit was used;
#   * with --build: CFBundleVersion is that number (TestFlight refuses a build whose
#     number is not higher than every earlier one, and the release workflow gives it
#     the number of the run);
#   * with --signed: the code signature verifies, the profile is embedded, it does not
#     allow a debugger to attach and it belongs to this bundle identifier;
#   * with --store (an App Store IPA, for TestFlight): the profile names no devices (an
#     IPA that lists devices would publish their identifiers), and Info.plist says
#     ITSAppUsesNonExemptEncryption is false (otherwise App Store Connect asks about
#     encryption for every build).
#
# macOS only (plutil, otool, codesign, security). Unverified until a hosted macOS
# run has built the application (docs/ci.md, "What iOS has and has not shown").
set -euo pipefail

target=""
version=""
build=""
signed=false
store=false
while [ $# -gt 0 ]; do
  case "$1" in
    --version) version="${2:?}"; shift 2 ;;
    --build) build="${2:?}"; shift 2 ;;
    --signed) signed=true; shift ;;
    --store) store=true; shift ;;
    -h|--help) sed -n '2,32p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    -*) echo "verify_ipa.sh: unknown argument '$1'" >&2; exit 2 ;;
    *) target="$1"; shift ;;
  esac
done
[ -e "$target" ] || { echo "usage: verify_ipa.sh <app|ipa> [--version v] [--build n] [--signed] [--store]" >&2; exit 2; }
fail() { echo "FAILED: $*" >&2; exit 1; }

bundle_id="com.iainchesworthlabs.snapandspell"

work=""
cleanup() { if [ -n "$work" ]; then rm -rf "$work"; fi; }
trap cleanup EXIT

app="$target"
case "$target" in
  *.ipa)
    work="$(mktemp -d)"
    unzip -q "$target" -d "$work"
    app="$(find "$work/Payload" -maxdepth 1 -name '*.app' -type d | head -n 1)"
    [ -n "$app" ] || fail "no Payload/*.app in $target"
    ;;
esac
echo "==> $app"

plist="$app/Info.plist"
[ -f "$plist" ] || fail "no Info.plist (an iOS bundle is flat: it is beside the executable)"
plutil -lint "$plist" >/dev/null || fail "Info.plist is not a valid property list"
key() { /usr/libexec/PlistBuddy -c "Print :$1" "$plist" 2>/dev/null || true; }

[ "$(key CFBundleIdentifier)" = "$bundle_id" ] || fail "CFBundleIdentifier is '$(key CFBundleIdentifier)'"
[ "$(key CFBundleExecutable)" = "WordJewelsSolver" ] || fail "CFBundleExecutable is '$(key CFBundleExecutable)'"
[ -n "$(key NSCameraUsageDescription)" ] || fail "Info.plist has no NSCameraUsageDescription"
[ "$(key CFBundleIconName)" = "AppIcon" ] || fail "CFBundleIconName is '$(key CFBundleIconName)'"
[ -n "$(key UILaunchScreen)" ] || fail "Info.plist has no UILaunchScreen (iOS would run the program in a legacy window)"
[ "$(key DTPlatformName)" = "iphoneos" ] || fail "DTPlatformName is '$(key DTPlatformName)'; this is not a device build"
[ -n "$(key MinimumOSVersion)" ] || fail "Info.plist has no MinimumOSVersion"
if [ -n "$version" ]; then
  [ "$(key CFBundleShortVersionString)" = "$version" ] || fail "CFBundleShortVersionString is '$(key CFBundleShortVersionString)', expected $version"
fi
# TestFlight needs a build number that grows; without --build any non-empty one will do here.
[ -n "$(key CFBundleVersion)" ] || fail "Info.plist has no CFBundleVersion"
if [ -n "$build" ]; then
  [ "$(key CFBundleVersion)" = "$build" ] || fail "CFBundleVersion is '$(key CFBundleVersion)', expected the build number $build"
fi
if $store; then
  [ "$(key ITSAppUsesNonExemptEncryption)" = "false" ] || fail "ITSAppUsesNonExemptEncryption is '$(key ITSAppUsesNonExemptEncryption)', expected false (the program uses no encryption of its own)"
fi
echo "    Info.plist: identifier, executable, version $(key CFBundleShortVersionString) build $(key CFBundleVersion), icon, camera text, launch screen, iOS $(key MinimumOSVersion) or later"

for required in \
    "$app/WordJewelsSolver" \
    "$app/Assets.car" \
    "$app/dictionaries/en.txt" \
    "$app/dice/3x3.txt" \
    "$app/dice/6x6.txt"; do
  [ -e "$required" ] || fail "missing ${required#"$app"/}"
done
echo "    executable, asset catalogue, dictionary and dice"

archs="$(lipo -archs "$app/WordJewelsSolver" 2>/dev/null || true)"
[ "$archs" = "arm64" ] || fail "the executable is for '$archs', expected arm64"
bad="$(otool -L "$app/WordJewelsSolver" | tail -n +2 | awk '{print $1}' | grep -Ev '^(/usr/lib/|/System/Library/)' || true)"
[ -z "$bad" ] || fail "the executable links outside the system: $bad"
echo "    executable: arm64, links only the system"

if $signed; then
  codesign --verify --strict --verbose=2 "$app" || fail "the code signature does not verify"
  profile="$app/embedded.mobileprovision"
  [ -f "$profile" ] || fail "no embedded.mobileprovision: the build was not signed with a profile"
  decoded="$(mktemp)"
  security cms -D -i "$profile" > "$decoded" 2>/dev/null || fail "cannot decode embedded.mobileprovision"
  pkey() { /usr/libexec/PlistBuddy -c "Print :$1" "$decoded" 2>/dev/null || true; }
  case "$(pkey Entitlements:application-identifier)" in
    *."$bundle_id") ;;
    *) fail "the profile is for '$(pkey Entitlements:application-identifier)', not $bundle_id" ;;
  esac
  devices="$(/usr/libexec/PlistBuddy -c "Print :ProvisionedDevices" "$decoded" 2>/dev/null | grep -c '^    ' || true)"
  if $store; then
    [ "$devices" -eq 0 ] || fail "the profile names $devices device(s): it is not an App Store profile, and an IPA that lists devices must not be kept in a public repository"
  fi
  [ "$(pkey Entitlements:get-task-allow)" = "false" ] || fail "the profile allows a debugger to attach (get-task-allow): it is a development profile"
  echo "    signed: the signature verifies; profile '$(pkey Name)' for $bundle_id, $devices device(s), expires $(pkey ExpirationDate)"
  rm -f "$decoded"
fi
echo "OK: $target"
