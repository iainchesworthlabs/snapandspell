pragma Singleton
import QtQuick

// The seam for sound and touch feedback. Every moment in the table of
// docs/design/application.md ("Audio and touch feedback") that wants a sound, a
// spoken name or a tap of the hand calls cue(); nothing is connected yet, because
// the modules that play them (Qt Multimedia, Sensors, TextToSpeech) are not in the
// tree and the recordings wait for the literacy review (G7). Output only: the app
// has no audio input.
//
// kinds: "speak" (detail: the picture's name), "rattle", "tile" (the tile's
// letters), "word" (the word), "tick", "chime", "cleared".
QtObject {
    signal cue(string kind, string detail)
}
