# Product requirements: WordJuels

A Boggle companion and game for children. This is the product document the
[roadmap](roadmap.md) is built from. It is the author's brief; the roadmap
says what exists, what each requirement needs, and in what order.

## Overview

WordJuels is an interactive tablet (iPad and Android) app for 5- and 6-year-olds
to build reading and vocabulary skills through the mechanics of Boggle. It has
two functions: a smart companion for physical family play, and a standalone
digital game. The goal is educational engagement: celebrating success and
guiding young readers without doing all the work for them.

## Game modes and features

### 1. Family Companion mode ("Scan & Assist")

**Concept.** The family plays with a physical Boggle grid. The app helps the
child so they can compete fairly with the adults.

- **Camera scan.** The parent uses the tablet camera to scan the physical grid.
- **Smart filter solver.** The backend solves the board but hides advanced
  words. It filters and indexes only age-appropriate words (for example 3- to
  4-letter words, sight words, simple phonics).
- **Nudge UI, not spoiling.** Instead of listing the words, the app gives
  hints. For example, it highlights a starting letter on a digital replica of
  the scanned board and says, "Can you find a 3-letter animal starting with
  'C'?"

### 2. Solo Play mode ("Digital Board")

**Concept.** No physical game is needed. The app replaces the whole board game
for standalone learning.

- **The shake phase.** A playful 3D or 2D animation of the Boggle box shaking,
  with haptic feedback and rattling sound effects.
- **The drop.** Virtual dice settle into a standard 4x4 grid, or a simplified
  3x3 grid (ideal for 5-year-olds).
- **Word tracing.** The child drags a continuous line with a finger to connect
  adjacent letters and spell words. Letters light up and speak their phonetic
  sounds as they are traced.

## User experience and gamification for early readers

Age target: 5 to 6 years (Kindergarten / Year 1). Text is minimal; the interface
is highly visual and audio-driven.

- **Celebration and rewards.** Success triggers vibrant visual rewards
  (exploding digital jewels, confetti, stars). Audio encouragement reads the
  found word aloud: "C-A-T. Cat! You found an animal!"
- **Visual board states.**
  - Active: clear, highly legible sans-serif type, in a face designed for early
    readers (OpenDyslexic, or Andika).
  - Selected: a glowing line tracks the finger path.
  - Success: found words become colourful "Juels" collected in a side bank.

## Implementation phases

### Phase 1: core engine integration

- Migrate the existing word-solver algorithm into the tablet environment.
- Add a dictionary filter that categorises words by syllable length,
  difficulty and child-friendly themes (animals, colours, shapes).

### Phase 2: input and interaction

- **Gesture recognition.** A smooth touchscreen path-drawing mechanic over a
  grid. Loose tolerances, so small fingers are not frustrated if they stray
  slightly off a tile.
- **Computer vision (future, P2).** An OCR / object-detection model (for
  example Apple's Vision framework) able to identify a grid layout and read
  text from angled physical dice faces.

### Phase 3: audio and animation

- Text-to-speech or pre-recorded audio triggered on successful word validation.
- An animation loop for the physical "shaking" simulation using the device's
  accelerometer.
