# The offences the QML lint (check_qml_literals.cmake) still allows, per rule and
# per file: "path under src/app/qml=count". It is a ceiling that only goes down.
# A screen stream that fixes its file lowers (or deletes) its own lines here; a
# count above its line, or a file with no line, fails quick.lint. Rules are
# described at the top of check_qml_literals.cmake. The kit (qml/kit) and the
# parent area's wording have no lines and never get one.
#
# The first-wave owners: Home.qml and solo/ (Home and Solo streams), parent/
# (the parent-screens stream), scan/ (Mode A).
#
# Regenerate the current counts (to see what is left) with
#   cmake -DREPO_ROOT=<root> -DALLOWLIST=<a file holding set(WJS_ALLOW_PIXEL_SIZE)> \
#         -DREPORT=<out.txt> -P tests/qml/check_qml_literals.cmake

set(WJS_ALLOW_PIXEL_SIZE
    "parent/DashboardScreen.qml=1"
    "parent/SettingsScreen.qml=1"
    "scan/CheckScreen.qml=1"
)

set(WJS_ALLOW_LETTER_SPACING
    "Home.qml=2"
    "parent/DashboardScreen.qml=1"
    "parent/SettingsScreen.qml=1"
)

set(WJS_ALLOW_HEX_COLOUR
)

set(WJS_ALLOW_BARE_TEXT
    "Home.qml=5"
    "parent/AboutScreen.qml=5"
    "parent/DashboardScreen.qml=16"
    "parent/GateScreen.qml=6"
    "parent/LicencesScreen.qml=3"
    "parent/ResetStep.qml=2"
    "parent/Section.qml=1"
    "parent/SettingRow.qml=2"
    "parent/SettingsScreen.qml=6"
    "scan/AssistScreen.qml=3"
    "scan/CheckScreen.qml=9"
    "scan/ScanScreen.qml=8"
    "solo/FlatBoard.qml=1"
    "solo/JewelsScreen.qml=16"
)

set(WJS_ALLOW_DISABLED_OPACITY
)

set(WJS_ALLOW_GOLD_SELECTION
)

set(WJS_ALLOW_ICON_ONLY
    "scan/AssistScreen.qml=1"
)

