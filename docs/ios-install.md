# Installing on my iPhone/iPad

WordJewelsSolver is not in the App Store or TestFlight. A tagged release carries an
**ad-hoc** build, an `.ipa` signed for the devices you have registered with Apple,
and a link that installs it from Safari. This page is for the author's own devices:
the install, adding a device, and what to do when the signing runs out. How the
build is made is in [CI](ci.md#ios).

!!! warning "This has not run yet"
    The unsigned compile of the iOS application has passed on a hosted runner
    (`ios-build.yml`, since 2026-10-21), but the signing, the archive, the export,
    the install and running it on a device have never been done. The first run is
    expected to need fixes
    ([what is unverified](ci.md#what-ios-has-and-has-not-shown)). Rehearse it
    before relying on it, as below.

## What you need

- An iPhone or iPad with **iOS 18 or later**. That is the minimum Qt 6.12 supports,
  and the build's deployment target is set to it (`CMakePresets.json`,
  `ios-base`). An older device cannot run it.
- The device registered in the **ad-hoc provisioning profile** the build was
  signed with. Ad hoc means exactly that: a device that is not in the profile
  cannot install the app, and nothing here can make it.
- The documentation site published ([why](#why-there-is-an-install-page)).

## Install

1. On the device, open the **install page** in **Safari** (not Chrome, not an app's
   browser, not a link inside the GitHub app; `itms-services://` links only work from
   Safari):

   `https://iainchesworthlabs.github.io/wordjewelssolver/install-ios.html`

   With no tag it offers the newest release that has an iOS build. A release's own
   notes link to it with that release's tag (`install-ios.html?tag=v0.1.0`). Bookmark
   it or add it to the Home Screen.

2. Tap **Install on iPhone or iPad**, and confirm iOS's question. The icon appears on
   the Home Screen and fills in as it downloads.
3. Open it. The first time, iOS asks whether the app may use the camera (Mode A, the
   scanner, reads a board from the camera's picture; the picture stays on the
   device).
4. If iOS says the developer is **untrusted**, go to *Settings, General, VPN & Device
   Management*, choose the developer app and tap **Trust**. A distribution
   certificate for an ad-hoc build normally does not need this; it is here in case
   it does.

An update is the same again with a newer release: iOS replaces the app and keeps
its data.

## Rehearse a build before tagging

A manual run builds and signs the IPA exactly as a tag would, and publishes nothing.
GitHub lists a manual run only for a workflow that is on the default branch, so this
works once the workflow is on the default branch, `develop` ([Releasing](releasing.md#rehearsal)):

```sh
gh workflow run release.yml --ref main -f signing=ephemeral -f ios=true
```

Read the `Build the iOS ad-hoc IPA` job. It stops at the first step if one of the three
secrets is missing and says which, and then checks the provisioning profile before
building (the App ID, that it lists devices, that it has not expired). The IPA of a
rehearsal is in the `ios-adhoc` artifact (or `package-ios` when `IOS_IPA_ON_RELEASE` is
`true`), and its install link would point at a release that does not exist, so a
rehearsal proves the build and the signing, not the install. The install is proved by the first
tag, a `-rc` one.

## Add a device

A device can install only the builds signed with a profile that lists it, so a new
device needs a **new profile and a new build**: a release made before the device was
registered will never install on it.

1. Find the device's UDID: connect it to a Mac and open it in Finder (click the line
   under its name until the UDID shows, then copy it), or in Xcode's *Devices and
   Simulators* window.
2. In the Apple developer portal, *Certificates, Identifiers & Profiles*:
   *Devices*, add it. Then *Profiles*, open the ad-hoc profile for
   `org.iainchesworth.wordjewelssolver`, edit it, tick the new device, save, and
   download the `.mobileprovision`.
3. Replace the secret (a profile is a file, so the value is its base64):

   ```sh
   base64 -i WordJewelsSolver_AdHoc.mobileprovision | gh secret set IOS_PROVISIONING_PROFILE_BASE64 --repo iainchesworthlabs/wordjewelssolver
   ```

   On Windows PowerShell: `[Convert]::ToBase64String([IO.File]::ReadAllBytes("WordJewelsSolver_AdHoc.mobileprovision")) | gh secret set IOS_PROVISIONING_PROFILE_BASE64 --repo iainchesworthlabs/wordjewelssolver`.

   The profile's name and team are read out of the file, so a new profile under
   another name needs nothing else set.
4. Cut a release (a patch tag, [Releasing](releasing.md#cutting-a-release)) or rehearse as above to
   check the new profile. Install from the new release's page.

## When the signing runs out

- **The profile expires on 2027-07-16.** After that date an installed copy stops
  opening, and no new build can be signed with it. Regenerate the profile in the
  portal (editing it and saving it makes a fresh one), upload it as above and cut a
  release; then reinstall. The build's own check fails with the expiry date when the secret holds an
  expired profile.
- **The distribution certificate has its own expiry** (the portal shows it). It is the
  organisation's, shared by every app of the team: renewing it is done once, in
  `icl-common-infra` (`docs/ios-signing.md`), and every profile must then be regenerated
  against the new certificate.
- **The secrets** are `IOS_DIST_CERT_P12_BASE64` and `IOS_DIST_CERT_PASSWORD`, the
  organisation's, and `IOS_PROVISIONING_PROFILE_BASE64`, this repository's. The team ID
  and the profile name are no longer secrets. They are set with `gh secret set`, which
  reads the value from standard input and prints none of it.
  The workflow never prints them either; keep the `.p12`, its password and the
  profile somewhere safe, outside the repository.

## Why there is an install page

Safari installs an ad-hoc app from a link of the form
`itms-services://?action=download-manifest&url=<an https URL of a manifest.plist>`.
The manifest names the app's identifier and version and where its `.ipa` is. A bare
link to an `.ipa` does not install. A release's notes cannot hold the link, because
GitHub's markdown renderer removes any link whose scheme is not on its short list
(`itms-services` is not). So the notes link to a page that makes it:
`docs/install-ios.html`, a static file of the documentation site.

That is why the site matters here. MkDocs copies a file that is not Markdown into
the site unchanged, so `docs/install-ios.html` is published with the rest of the
docs by `docs.yml` whenever `main` changes the docs ([CI](ci.md#documentation-site)):
merging `develop` into `main` for the first release publishes the site, and the page
with it, before the tag is cut. If the repository's Pages source has not been set
to GitHub Actions the link in a release's notes is a 404; the manifest and the `.ipa`
are still release files, and the page is the only part that is missing. The release
workflow does not wait for it.

The manifest's `.ipa` URL is the release asset's own
(`https://github.com/iainchesworthlabs/wordjewelssolver/releases/download/<tag>/<file>`).
The repository is public, so nothing about it needs a login.

## If it does not install

| What happens | Likely cause |
| --- | --- |
| The page shows no button, or says no release with an iOS build was found | No published release has a `manifest.plist` yet (a rehearsal publishes nothing), or GitHub's API refused the page a lookup; open it with `?tag=<tag>` |
| Tapping the button does nothing | Not in Safari. Open the page in Safari |
| "Unable to install", or the app is dimmed and stuck on "Waiting" | The device is not in the profile this release was signed with: [add it](#add-a-device) and cut a new release |
| iOS asks to trust the developer | Step 4 of [Install](#install) |
| The app opened before and now closes at once | The profile expired ([above](#when-the-signing-runs-out)) |
| It installs but is a white or dark screen | A build problem, not a signing one: this has never been run. Say what the device is and what iOS it has |
