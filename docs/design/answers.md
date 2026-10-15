# Answers to the open questions

Section 11 of the [product requirements](../product-requirements.md) now numbers
69 questions: 1 to 15 from the first brief, 16 to 57 from the additions of
2026-10-16, and 58 to 69, which are the author's open details about the MVP
(section 12) and are treated here as planning assumptions, not decisions. Each
is answered as a proposal. The detail is on the [game](game.md),
[phonics](phonics.md), [board](board.md), [application](application.md),
[dashboard](dashboard.md), [performance](performance.md) and
[path](path.md) pages; this page gives the answer and the reason, and says who
has to confirm.

How the evidence is marked is on the [overview](index.md#how-to-read-these-pages).

**Who confirms**

| Mark | Meaning |
| --- | --- |
| *design* | Settled by the design on these pages; the author may still change it |
| **Author** | Nothing in the brief or the code settles it; it has a default |
| **Reviewer** | A literacy specialist or classroom teacher (gate G7); not the author, not engineering |
| *deferred* | Outside the MVP; recorded so it is not lost |

## Game

**1. What does history hold?** *design.* Earlier boards, newest first: a small
picture of the board and the words found. This board's words are the "words I
found" part of the same panel, so history that repeated them would be a second
copy. Options were this game's words, earlier games, or both in one list. The
last 20 boards are kept on the device. See [Game](game.md#the-panel).

**2. What are the help levels, and how is one chosen?** *design.* **MVP: one
level, the start tile** (question 68). After the MVP, a ladder of five rungs the
child climbs one press at a time (count, start tile, length and group,
direction, the word) with a ceiling the parent sets in a dial: both a setting
and a dial, one for each person. Never offered unasked. See
[Game](game.md#help).

**3. What counts as a word the child may find?** *design.* Three states. A word
of the phase index that is on the board is **viable**: it counts. Another
dictionary word of 3 or more letters is **real, not today's**: a neutral cue, no
star, and the word is neither shown nor read, so no block list is needed in the
MVP. Anything else fades. Options were only the child-facing lists, all of the
dictionary, or the dictionary with hints from the tier. The lists are too small
to judge (16% of three-letter and 6% of four-letter ENABLE words). See
[Game](game.md#words-and-what-each-does).

**4. What scores, and what is a "new high score" scored against?** *design,
the author's table.* Stars by tier (3 letters +1, 4 letters +2, 5 or more +5).
The best is the most stars in one board, for each board size, on each device,
recorded silently, celebrated once at the moment, never shown to the child as a
number, and shown to the parent. One child per device (question 27). See
[Game](game.md#high-scores-without-a-pull).

**5. In the Family Companion, does the app keep the child's score against the
other players?** *design; **Author** to confirm.* No. It is a helper, and a
scoreboard would need every player's words, which it never sees. The child's own
words are a tray for the round. If a scoreboard is wanted it is a different
feature with a different privacy picture.

## Board

**6. Which dice, and from where?** *design.* The MVP uses none: boards are
seeded from the phase index (question 24). After the MVP, dice **made by this
project** from letter counts of `data/dictionaries/en.txt`, recorded as
`data/README.md` requires. Options: a set the author owns (a legal question, not
answered here), a list found online (provenance unknown, so not usable), or
made here. No distribution of letters is given on these pages. See
[Board](board.md#where-the-letters-come-from).

**7. How is a 3x3 board related to the others?** *design.* A smaller board of
the same game, in Solo Play only, the first board on a new device, with its own
nine dice or seeding. Not a window onto the 4x4 (it would show the wrong words),
and not in the scanner because a real board is 4x4 or larger. In the MVP.

**8. How are 5x5 and 6x6 shown in the two-thirds area on a phone?** *design.*
After the MVP (section 12.5). The board is the largest square that fits; a size
is offered only where its tile is 48 dp or more, so on a 360 dp phone 6x6 is
51 dp and on a 320 dp phone it is not offered. See
[Board](board.md#layouts-and-sizes).

## Scanning

**9. What starts a scan?** *design.* **MVP: a button**, because the MVP is a
single capture (question 67). After it, both: a board that has been still for
about a second reads itself, and a **Read it now** button is always there. The
lid coming off is not used, because a hand and a lid moving over the board look
like a board moving.

**10. How are recognition errors corrected, and how visibly?** *design.* Every
read ends on a check screen: the replica upright, doubtful tiles ringed, a tap on
a tile opens a letter picker. Never skipped in the MVP: an OCR error would feed
the solver a wrong board, and no accuracy has been measured. See
[Game](game.md#the-check-screen).

**11. How is the board size determined?** *design.* Counted from the picture and
shown before the letters, changeable on the check screen. In the MVP the board
is a 4x4.

## Application

**12. How does the layout behave in portrait, on a phone and in a narrow
window?** *design.* It is split by the window's shape, not the device: wide puts
the board in the leading two-thirds, tall puts it in the top two-thirds. The
panel keeps three roles at a density that fits. See
[Board](board.md#the-rule) and [layouts.svg](layouts.svg).

**13. 3D, 2D or scripted, and a physics engine?** *Decided by the author,
2026-10-16.* The brief expects a 3D dice shake in a 3D container, and the author
accepted the recommendation: **Qt Quick 3D behind a one-component seam for the
MVP, scripted, with no physics engine, and the licence decision left at D2.**
The MVP is a private build, which no licence rule touches; the cost (a binary
that is GPL-3.0 only, and Apple's store) comes only at a store submission,
when the seam makes the alternative a rewrite of one component. A physics engine
would decide the board that the game has already decided. Checked in the shared
tree's SBOMs: `Quick3D` is Commercial or GPL-3.0-only; Quick and Particles carry
the LGPL option. See [Board](board.md#the-shake-and-the-drop).

**14. What are the budgets, and what does a test measure?** *design; **Author**
to name the lowest device.* A 16.7 ms frame; no frame over 33 ms in the shake;
full dictionary under 1 s off the UI thread; any board solved under 50 ms;
**exact work budgets** in every `ctest`, **loose time ratios** opt-in. Measured
on a desktop: load 55 to 65 ms and 17.7 MB; solve 0.03 to 0.85 ms by size, 1.7
ms at worst. Nothing on a tablet. See [Performance](performance.md).

**15. What does it do without a camera?** *design.* A camera is a runtime
condition (Cubeulator's browser build reads one). Mode B is whole. Mode A offers
**Type in your board**, which is the check screen with an empty grid.

**16. What does it do and say when a path is not a word, or is a real word
outside the tier?** *design.* Not a word: the path fades, no sound, no text. A
real word that is not today's: a soft tick, no star, no word shown or read.
Neither is wrong; neither scolds (7.1). See [Game](game.md#words-and-what-each-does).

**17. What are the stars and badges for, what do they look like, where are
they kept?** *design.* Stars: every viable word, by tier. A sticker: the first
time a word is found on the device. Looks: a found word is a jewel sized by tier;
the star count is the number. Kept in the local store; shown to the child only in
the vault. Session badges are after the MVP.

**18. Which typeface meets the clarity requirement?** *design; **Reviewer** and
**Author** (a side-by-side).* **Andika**, designed for beginning readers, with
an I, l and 1 that its design notes say are made distinct; OpenDyslexic is named
in the brief and not assessed. **The glyphs have not been looked at**: neither
face is installed here. A five-step procedure for doing it at the sizes in use
is on [Application](application.md#type).

## Progression (7.2)

**19. How is a word's tier decided?** *design.* By letter count, `Qu` counting two,
so `QUIT` is a Power Word. An inflection of a viable word is viable, earns
stars and shares its base word's sticker; whether a given inflection is
suitable is the **Reviewer's**.

**20.** Settled in section 12.1: Power Words.

**21. Which words have a sticker, and who draws them?** *design; **Author**.* The
closed set is the active phase's index. The pictures and the sentences need art
and text the repository does not have, under a licence it can state. In R1 a
sticker can be the word itself as a jewel; pictures are R2. See
[Path](path.md#r2-more-to-find-and-more-to-keep).

**22. What does a level's "words it unlocks" do?** *design.* Adds those stickers
to the book and makes those words likelier in seeding. It never changes what is
accepted: every viable word counts. Worlds are not in the MVP unless the author
says so.

**23. Are the thresholds a sitting or two, and what is the pace?** *design;
**Author**.* **Estimate:** a board holds 3 to 5 words worth 1 to 2 stars, so
about 3 to 10 stars; 20 stars is two or three boards. Set the thresholds from
how many boards a week are expected.

**24. How far may letters be steered, and what is measured?** *design.*
Steering chooses targets and fillers and nothing else; every board is verified
by the solver to hold 3 to 5 viable words; the parent picks the phase and the
app never moves between phases; a test reports variety over 1,000 seeds. The
scanner cannot be steered, and there the phase is only a filter. See
[Phonics](phonics.md#seeding).

**25. What is the reduced-motion and reduced-sound behaviour, and who sets it?**
*design.* The screen shake is not shown for anyone (the jewel bounces); confetti
is confined to the board and the panel and flashes at most three times a second;
reduced motion removes particles; celebration sounds can be set to quiet or
off. The system's setting is the default and the parent area overrides it. See
[Application](application.md#motion).

**26. Whose voice, recorded or synthesised, in which languages?** *design;
**Author**.* Isolated sounds are pre-rendered recordings (the brief says speech
synthesis does not do them well); whole words and sentences may use the
platform's own speech or recordings. Nothing is captured from the child. English
in the MVP; the brief asks about Australian English. Who records is open (roadmap
D5).

**27. Whose progress is kept?** *design.* One child per device in the MVP, on the
device only, reset by a parent behind the gate. Several children need picture
avatars and separate stores, later.

**28. What is the mascot?** *Author.* After the MVP. The same character in every
world; accessories are token and art swaps.

## Phonics and school fonts (7.1 and 7.3)

**29. Is phonics a mode, a setting or the default?** *design.* A setting, the
**phase**, chosen by the parent; the default is Phase A. Two axes: the phase is
what the board is made of; tiers, stars and worlds are what the child earns.

**30. Does tracing bind single-letter dice into a grapheme at selection time, or
do dice carry multi-letter faces?** *design.* At selection time, greedily and
longest-first over the **active phase's** graphemes; dice carry single letters
(and `Qu`). Out of order, no link, and the word is sounded as separate sounds. A
split digraph's arc is drawn when the word is complete. See
[Phonics](phonics.md#binding-a-trace-into-grapheme-blocks).

**31.** Settled in section 12.1: A and I link; the sound is /eɪ/.

**32. Where does segmentation come from, and tricky words and accent?** *design;
**Reviewer**.* Hand-authored for the reviewed list, one split per word; the
repository has no pronunciation lexicon and the dictionary has no
pronunciations. A word that does not decode is marked `tricky` and spoken
whole. Whether the sounds follow Australian English is the reviewer's.

**33. What exactly do Phase A and Phase B contain?** ***Reviewer***. Data files,
so the answer changes no code. The mechanism has phases as cumulative sets of
graphemes. Where AI and AY sit, and whether there is a Phase C, is for the
review.

**34. How is each phase's word set made?** ***Reviewer***. The dictionary filter
(249 words for SATPIN) is an *input* to a hand-written, reviewed list, never the
list: the filter admits entries a child should not meet and 127 plurals.

**35. Does a phase constrain boards, and what if too few words?** *design.* Yes:
the phase index is the dictionary seeding verifies against. If too few words
fit, targets fall back to the phase generally. Order of authority: the phase is
a hard constraint; focus sounds and steering are preferences.

**36. Is the sound sticker book the same album?** *design.* One album; a sticker
belongs to a word; "by word" and "by sound" are views, and a word appears on the
page of each of its graphemes.

**37. Who records the phonemes, in what accent, and how is the blending kept in
step?** ***Reviewer*** *and **Author**.* After the review. In step by
construction: each block lights when its clip starts, so there is no clock to
drift.

**38. Which school font belongs to which state, and may each be bundled?**
***Author***. **Not established.** A NSW font is described by a vendor as the
Department's copyright; a Victorian font has an unofficial CC-BY-4.0 entry;
separate *Edu* families under the OFL imitate them. Only fonts whose licence
file permits bundling are offered. See [Application](application.md#school-fonts-71-questions-38-to-40).

**39. What does the font setting change?** *design.* The lower-case words in
the tray, vault and captions. Not the dice, which stay print capitals.

**40. Which requirement wins where a school font is ambiguous?** *design.*
Clarity; a school font is offered only where it passes the glyph check.

## Parent dashboard (7.4)

**41. What exactly is recorded for the "read aloud" moment?** *deferred.* Outside
the MVP: no voice is captured. Returns only with the review of
[Path](path.md#r6-and-after-the-deferred-privacy-sensitive-features).

**42. How are "mastered", "emerging" and "blind spot" defined?** *design;
**Reviewer**.* Neutral terms: *found*, *often found* (three or more distinct
words, unassisted), *met*, *available, not found*. "Unassisted" is exact in the
MVP because the only help is the start tile. See
[Dashboard](dashboard.md#definitions-questions-42-to-48).

**43. How is an abandoned trace told from a stray touch?** *design.* It is not,
reliably, so it is not recorded or shown: a touch plays a letter's sound, so
exploring produces many short paths that are play.

**44. How are findings phrased?** *design; **Reviewer**.* Observations, never a
gap: "on 6 boards, found on 4", "not met yet". A gap in play is not a deficit.

**45. How do focus sounds relate to phases, and what does seeding do when too
few words use them?** *design.* A phase is a set of sounds; focus sounds pick
within it. Few words: fall back to the phase and say "few words use this sound
yet".

**46. Does the dashboard in the scanner only track and advise?** *design.* Yes.
A real board cannot be seeded; the dashboard shows what was on it and what was
found, and phrases any advice as a suggestion to look for together.

**47. When is the weekly report produced?** *design.* When the dashboard is
opened, on the device, with no notification and no schedule; a week by the
device's calendar locale.

**48. Which hints are "family play insights", and who taps?** *design.* "Start
tiles shown"; the app cannot tell who pressed.

**49. What is the parental gate?** *design; **Author**.* Two fingers on two
marked spots for two seconds, with a screen-reader action. The age assumed unable
to pass it is not established and is tried with children; the stores' rules are
unverified.

**50. What happens on a new device; is an export or backup wanted?** *design;
**Author**.* A new device starts fresh; nothing is exported. An iOS app's data is
in a device backup by default (general knowledge, not checked); whether to
exclude it is a decision between "air-gapped" and a lost iPad.

**51. Several children with separate heatmaps, or one?** *design.* One in the
MVP.

## Time played and the printable summary

**52 to 57.** *deferred.* None is in the MVP (section 12.2 and 12.3), and none is
designed. What each must meet when it returns is in
[Dashboard](dashboard.md#deferred-and-what-each-must-meet-when-it-returns): time
played is defined as active play and is not stored now; a limit sits beside the
system's controls and never ends play abruptly; the summary is a parent's
action with a preview, behind the gate, handed to the system's facilities with
no stored address, and is the first route by which a child's data leaves.

## The MVP assumptions (58 to 69)

**58. Dictionary scope.** **Reviewer**, *confirm.* Hand-filtered, reviewed,
fewer than 249, each board holding three to five. Written by the reviewer or by
the author and reviewed by them. **Measured:** blind random boards from the six
letters land in three to five about one time in four at best; so seeding is
verification, not luck (see [Phonics](phonics.md#why-verification-is-the-method)).

**59. The five.** *Planning assumption confirmed as a starting subset.* If they
are the whole set, every board offers some of the same five words, in different
places. **Measured:** with only the five, 70% of real-looking 4x4 boards hold
none, so the scanner would say "nothing this time" on most boards.

**60. RAIN.** **Reviewer.** Phase A shows the grapheme effect with
SATPIN-spelled words that carry "ai"; RAIN is Phase B. Whether Phase B is in the
MVP is the author's; the mechanism supports it with data.

**61. A real word that is not on the board's list.** *design.* A soft tick, no
star, no word shown or read, no "wrong". Whether a child can tell it from "not a
word" is a test with children.

**62. Removed or deferred.** *design, confirm.* Deferred. Every deferred feature
is on [Path](path.md#r6-and-after-the-deferred-privacy-sensitive-features) with
what it must meet; nothing in the MVP's design needs them.

**63. After a board is cleared.** *design.* The resting screen: a still, quiet
picture; one outline button, held for a second, available after the
celebration; high scores recorded silently and shown to the parent only. See
[Game](game.md#the-resting-screen).

**64. Platform.** *Author, confirm.* The iPad first, as a private ad-hoc build;
the section 2 list is the product's scope. Consequence: the shared Qt tree has no
iOS kit, and Multimedia, Sensors and TextToSpeech are missing from it.

**65. Zero network capability: how is it kept true and checked?** *design.* A
per-target table of build facts and checks: no `INTERNET` permission on Android,
no network classes in first-party code, a QML network manager that refuses
remote URLs, a content-security policy on the web, and no profiler in what
ships. Qt Network cannot be left unlinked (Qml depends on it), so the rule is
"no capability and no use". See
[Application](application.md#privacy-built-in-and-checked).

**66. Programme names in history.** *Author.* This design's documents carry no
programme name. The names are in earlier commits of `feature/product-design`,
which a pull request from it would carry because the repository merges by merge
commit. Options: rewrite that branch and force-push it (only the design
sessions use it); or **start a fresh branch from `develop` holding the final
tree as one commit and open the pull request from that**, which needs no
force-push. The second is recommended. **Either one affects
`feature/game-design`**, which has merged `feature/product-design` as asked, and
would be rebuilt on the new branch.

**67. The scanner in the MVP.** *design; **Author**.* In, as a single capture
with the check screen. It is the riskiest piece and has not been started; the
single capture is what makes it safe to include.

**68. Help in Mode A.** *design.* The start tile is the one level in both modes.

**69. The reviewer.** ***Author***. Not named in this repository. The review is
needed **before any audio is recorded and before a child uses the content**
(section 12.6), and also before the phase index is called final. What can be
done before it: the mechanism, the checker, the seeding with test data, the
sequence timeline with placeholder clips, and the build gate that refuses an
unreviewed phase file.
