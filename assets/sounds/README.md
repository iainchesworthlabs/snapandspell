# The feedback sounds

These eight WAV files are **placeholders**, made for the application and owned
by the project (not third-party audio; `notices/fragments/assets.txt` says so).
They are synthesised short tones and noise bursts: a rattle, a die landing, a
letter joining, a tick, a cheer, a chime, the three cleared notes and the tap of
a control. They exist so the cue wiring in `apps/app/src/feedback` and the parent
area's Sound, Celebration sounds and Sound it out rows can be heard and tested
while the real recordings are still the reviewer's job.

The real phoneme clips and the recorded cheers replace them after the literacy
review ([Phonics design](../../docs/design/phonics.md)); the file names are the
seam. The mapping from a cue to one of these files is the pure policy in
`libs/core/include/wjs/feedback.hpp`.

They are 16-bit PCM mono WAV at 22050 Hz, embedded as `:/sounds/<name>.wav` in
the application (`apps/app/CMakeLists.txt`), so no file of them is installed.

`tools/sounds/make_sounds.ps1` writes them and is deterministic: `-Check`
compares the checked-in files with what it would write, and the `data.sounds`
ctest runs that in every CI leg with PowerShell.
