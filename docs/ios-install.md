# Installing on an iPhone or iPad

Snap and Spell is distributed to iPhones and iPads through **TestFlight**. The release workflow builds the app, signs it
for the App Store, has Apple check it and uploads it to App Store Connect, and the people the owner has invited as testers
install it from the TestFlight app. No device is registered with Apple and nothing in the build identifies a device, which is
what a public repository needs: an ad-hoc IPA carries the identifiers of the devices it is for, and neither a release asset
nor a workflow artifact is private here, so this repository does not make one. How the build is made is in
[CI](ci.md#ios), and how the signing pieces come from Apple is in `docs/ios-signing.md` of `icl-common-infra`.

!!! warning "This has not been tried on a device"
    The unsigned compile of the iOS application has passed on a hosted runner (`ios-build.yml`), and so has the earlier
    ad-hoc signing and export of this repository (a rehearsal on 2026-10-26). The App Store signing, Apple's validation, the
    upload to TestFlight and running the app on an iPhone or an iPad have not been done here until the first TestFlight build
    is made, and nobody has opened the app on a device. The build's checks (`verify_ipa.sh --signed --store`) look inside
    the IPA; they do not install it. [What is unverified](ci.md#what-ios-has-and-has-not-shown).

## For a tester

1. Install **TestFlight** from the App Store on the iPhone or iPad.
2. Be invited as an internal tester by the owner (App Store Connect, the app, TestFlight, Internal Testing). The Apple ID you
   sign in to TestFlight with must be the one that was invited.
3. Open TestFlight, choose **ICL Snap and Spell** and tap Install (or Update). The app is called Snap and Spell on
   the Home Screen.

It needs **iOS 18 or later**, which is the minimum Qt 6.12 supports and the build's deployment target (`CMakePresets.json`,
`ios-base`). The app asks for the camera only on the real-board screens, and says why; it keeps everything on the device.

## For the owner

Set up once, in the Apple developer portal and App Store Connect: the identifier `com.iainchesworthlabs.snapandspell`
(Explicit, no capabilities), an **App Store Connect** distribution profile for it named `Snap and Spell App Store`
(certificate `Apple Distribution`, no devices), the app record (the store name `ICL Snap and Spell` (the ICL only makes it unique across the App Store; the name under the icon is Snap and Spell), any SKU)
and an Internal Testing group in its TestFlight tab. `docs/ios-signing.md` in `icl-common-infra` says where each comes from.

In this repository the pieces are:

| What | Where | Name |
|---|---|---|
| The App Store provisioning profile (base64) | repository secret | `IOS_PROVISIONING_PROFILE_BASE64` |
| The Apple Distribution certificate and its password | organisation secrets | `IOS_DIST_CERT_P12_BASE64`, `IOS_DIST_CERT_PASSWORD` |
| The App Store Connect API key | organisation secrets | `APPSTORE_API_KEY_ID`, `APPSTORE_API_ISSUER_ID`, `APPSTORE_API_PRIVATE_KEY` |
| The switch | repository variable | `SIGN_IOS_UPLOAD` = `true` |

```sh
base64 -w0 Snap_and_Spell_App_Store.mobileprovision | gh secret set IOS_PROVISIONING_PROFILE_BASE64 -R iainchesworthlabs/snapandspell
gh variable set SIGN_IOS_UPLOAD --body true -R iainchesworthlabs/snapandspell
```

Without `SIGN_IOS_UPLOAD` the release has no iOS build and says nothing about it.

### Uploading a build

```sh
gh workflow run release.yml --ref develop -f desktop=false -f android=false                          # builds, signs, has Apple validate it; uploads nothing
gh workflow run release.yml --ref develop -f desktop=false -f android=false -f upload_testflight=true  # and uploads it to TestFlight
```

A pushed `v*` tag uploads too. Read the job's log, not its green tick: the validation step should end with
`VERIFY SUCCEEDED with no errors`, and the upload with `UPLOAD SUCCEEDED with no errors` and a Delivery UUID. The build
appears in TestFlight a few minutes after the upload, once Apple has processed it.

Each upload gets a **build number** that is the number of the workflow run (`SNS_IOS_BUILD_NUMBER`, written to
`CFBundleVersion`), which only grows, as App Store Connect requires; the version people see is the project's. A build cannot
be taken back from App Store Connect once uploaded, so validate first.

### When the signing runs out

- **The profile lasts a year.** The distribution certificate expires on 2027-07-16 and a profile cannot outlive it. Make the
  profile again in the portal (editing it and saving it makes a fresh one) and set `IOS_PROVISIONING_PROFILE_BASE64` again.
  The job's first step refuses an expired profile and names its date.
- **The certificate is the organisation's**, shared by every app of the team: renewing it is done once, in `icl-common-infra`
  (`docs/ios-signing.md`), and then every app's profile must be made again against the new certificate.
- **The secrets are never printed** by the workflow. `gh secret set` reads the value from standard input and prints none of
  it; keep the `.p12`, its password, the `.p8` key and the profile somewhere safe, outside the repository.

## What has not been tried

- The signed App Store build, Apple's validation and the upload (a first run is expected to meet something: Apple's checks
  name the icon, the property list and the privacy declarations of the statically linked Qt libraries, and
  `ITSAppUsesNonExemptEncryption` and the icon without alpha are already dealt with).
- Installing from TestFlight and running the app on any iPhone or iPad: the camera permission, the shake with the
  accelerometer, speech, the store in the app's container, rotation and the layouts on an iPad.
- External testers. They need Beta App Review and the store listing's metadata, which are the owner's to decide; this
  page describes internal testing only. A public App Store release needs more again (an age rating, the privacy answers,
  and, for an app made for young children, the Kids Category's rules).
