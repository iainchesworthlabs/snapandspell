# Toybox Pixel for Word Jewels Solver: design

Draft 1 for sign-off. These files describe the restyle of the application in the Toybox Pixel style of the Snake design package, and are design material, not production code.

```
DESIGN.md              every screen and component under Toybox Pixel, the decisions, the mapping to Theme.qml (start here)
tokens.json            Toybox Pixel 1.0 from the Snake package, unchanged
wjs-tokens.json        what Word Jewels uses from it and adds: gems, tile states, Night, type roles, motion, contrast pairs
gallery.html            gallery of every screen; open in a browser
screens/               one standalone HTML page for each screen, driven by the two token files
screenshots/           PNG of each screen at 2x (rendered with headless Edge), plus _legibility.png
fonts/                 Pixelify Sans, Silkscreen, Tilt Neon (from the Snake package) and Andika, each with its licence text
source/make_mockups.py writes toybox.css, screens/ and gallery.html from the tokens
source/render_all.sh   renders screens/ to screenshots/ with the Windows Edge from WSL
source/contrast.py     checks the contrast ratios stated in wjs-tokens.json
```

Rebuild: `python3 source/make_mockups.py`, then `source/render_all.sh`. The Hebrew and Chinese pages use the project's own catalogues (`i18n/wjs_*.ts`) and the system fonts of the machine that renders them.

Shared with Snake: the values in `tokens.json`. Specific to Word Jewels: everything in `wjs-tokens.json`. The fonts in `fonts/` other than Andika come from the Snake package; Andika is the project's own bundled copy.
