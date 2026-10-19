pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import WordJewelsSolver

// The flat board the child reads and traces. It is 2D on purpose: the 3D scene
// is the shake and the drop, and this is what settles afterwards. The tray is
// the physical object; the dice keep their shape when selected; the path is a
// gold ribbon under the letters, so it never cuts through them.
Item {
    id: root

    required property var solo
    property rect boardRect: Qt.rect(0, 0, 0, 0)
    property real tileSize: 1
    property real tileGap: 0

    readonly property int rows: solo.rows
    readonly property int columns: solo.columns
    readonly property real pitch: tileSize + tileGap
    readonly property bool accepting: solo.state === "playing"
    readonly property bool presenting: solo.state === "word-found"
    readonly property real slack: tileSize * 0.38
    readonly property real trayInset: Math.max(10, tileSize * 0.08)

    property int focusIndex: 0
    property bool keyboardUsed: false

    activeFocusOnTab: true
    LayoutMirroring.enabled: false
    LayoutMirroring.childrenInherit: true
    Accessible.role: Accessible.Table
    Accessible.name: SoloWording.board

    function tileX(index) {
        return boardRect.x + (index % columns) * pitch;
    }
    function tileY(index) {
        return boardRect.y + Math.floor(index / columns) * pitch;
    }
    function centre(index) {
        return Qt.point(tileX(index) + tileSize / 2, tileY(index) + tileSize / 2);
    }
    function points(indices) {
        return indices.map(i => centre(i));
    }
    function isLit(index) {
        if (solo.path.indexOf(index) >= 0) {
            return true;
        }
        return root.presenting && solo.litPath.indexOf(index) >= 0;
    }

    Connections {
        target: root.solo

        function onStateChanged() {
            if (root.solo.state === "word-found") {
                wordTimer.restart();
            }
        }
        function onBoardChanged() {
            root.focusIndex = 0;
        }
    }

    Timer {
        id: wordTimer
        interval: Theme.wordHold
        onTriggered: root.solo.wordShown()
    }

    // The tray.
    Rectangle {
        x: root.boardRect.x - root.trayInset
        y: root.boardRect.y - root.trayInset
        width: root.boardRect.width + 2 * root.trayInset
        height: root.boardRect.height + 2 * root.trayInset
        radius: Theme.radiusLarge
        color: Theme.tray
        border.width: 2
        border.color: Theme.gold
    }
    Rectangle {
        x: root.boardRect.x - root.trayInset + 6
        y: root.boardRect.y - root.trayInset + 6
        width: parent.width - 12
        height: parent.height - 12
        radius: Theme.radiusLarge - 4
        color: Theme.well
        border.width: 1
        border.color: "#2A4A52"
        visible: false
    }

    // The path is a ribbon between the centres, under the dice. The dice hide
    // the parts over a letter; only the gold connectors in the gaps show.
    Shape {
        anchors.fill: parent
        visible: root.solo.path.length > 1
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: Theme.gold
            strokeWidth: Math.max(5, root.tileSize * 0.07)
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathPolyline { path: root.points(root.solo.path) }
        }
    }

    // A path that was not a word fades where it was.
    property var ghostPath: []
    property string ghostKind: ""
    Connections {
        target: root.solo
        function onTraced(kind, path) {
            if (kind === "tick" || kind === "fade") {
                root.ghostPath = path;
                root.ghostKind = kind;
                ghostFade.restart();
            }
        }
    }
    Shape {
        id: ghost
        anchors.fill: parent
        opacity: 0
        visible: opacity > 0 && root.ghostPath.length > 1
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: Theme.gold
            strokeWidth: Math.max(4, root.tileSize * 0.05)
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathPolyline { path: root.points(root.ghostPath) }
        }
    }
    NumberAnimation {
        id: ghostFade
        target: ghost
        property: "opacity"
        from: 1
        to: 0
        duration: root.ghostKind === "tick" ? Theme.quick * 2 : Theme.pathFade
    }

    // The dice.
    Repeater {
        model: root.solo.tiles

        Item {
            id: cell
            required property int index
            required property string modelData

            x: root.tileX(cell.index)
            y: root.tileY(cell.index)
            width: root.tileSize
            height: root.tileSize

            readonly property bool dead: modelData === ""
            readonly property int order: root.solo.path.indexOf(cell.index) + 1

            TileFace {
                anchors.fill: parent
                letters: cell.modelData
                lit: root.isLit(cell.index)
                order: cell.order
            }

            // A dead tile is a die of six jewels in the scene; on the flat board
            // it keeps the same cream die and shows the same cut stone.
            Jewel {
                visible: cell.dead
                anchors.centerIn: parent
                width: parent.width * 0.58
                height: width
                tier: "power"
                color: Theme.jewelDead
            }

            Rectangle {
                visible: root.accepting && root.activeFocus && root.keyboardUsed
                         && root.focusIndex === cell.index
                anchors.fill: parent
                anchors.margins: -3
                radius: Theme.radiusSmall + 3
                color: "transparent"
                border.width: 2
                border.color: Theme.gold
            }

            Accessible.role: cell.dead ? Accessible.Graphic : Accessible.Button
            Accessible.name: cell.dead
                ? SoloWording.jewelName(Math.floor(cell.index / root.columns) + 1,
                                        cell.index % root.columns + 1)
                : SoloWording.tileName(cell.modelData === "QU" ? "Qu" : cell.modelData,
                                       Math.floor(cell.index / root.columns) + 1,
                                       cell.index % root.columns + 1)
            Accessible.checked: root.isLit(cell.index)
            Accessible.onPressAction: root.solo.tapTile(cell.index)
        }
    }

    // The start tile's gold ring.
    Rectangle {
        id: ring
        visible: root.solo.helpTile >= 0 && root.accepting
        x: root.solo.helpTile >= 0 ? root.tileX(root.solo.helpTile) - Theme.startRing : 0
        y: root.solo.helpTile >= 0 ? root.tileY(root.solo.helpTile) - Theme.startRing : 0
        width: root.tileSize + 2 * Theme.startRing
        height: width
        radius: Theme.radiusSmall + Theme.startRing
        color: "transparent"
        border.width: 3
        border.color: Theme.gold
        Accessible.role: Accessible.Graphic
        Accessible.name: SoloWording.startHere

        SequentialAnimation on scale {
            running: ring.visible && !Theme.reducedMotion
            loops: 5
            NumberAnimation { from: 1.0; to: 1.06; duration: Theme.ringPulse; easing.type: Easing.InOutQuad }
            NumberAnimation { from: 1.06; to: 1.0; duration: Theme.ringPulse; easing.type: Easing.InOutQuad }
        }
    }

    // The first pointer draws; the rest is ignored.
    MouseArea {
        id: pointer
        x: root.boardRect.x - root.slack
        y: root.boardRect.y - root.slack
        width: root.boardRect.width + 2 * root.slack
        height: root.boardRect.height + 2 * root.slack
        enabled: root.accepting
        preventStealing: true

        function local(mouseX, mouseY) {
            return Qt.point(mouseX - root.slack, mouseY - root.slack);
        }
        function over(mouseX, mouseY) {
            const p = local(mouseX, mouseY);
            const s = root.slack;
            return p.x >= -s && p.y >= -s && p.x <= root.boardRect.width + s && p.y <= root.boardRect.height + s;
        }

        onPressed: mouse => {
            root.keyboardUsed = false;
            root.forceActiveFocus();
            root.solo.pressAt(pointer.local(mouse.x, mouse.y).x, pointer.local(mouse.x, mouse.y).y);
        }
        onPositionChanged: mouse => {
            const p = pointer.local(mouse.x, mouse.y);
            root.solo.moveTo(p.x, p.y);
        }
        onReleased: mouse => {
            const p = pointer.local(mouse.x, mouse.y);
            root.solo.releaseAt(p.x, p.y, pointer.over(mouse.x, mouse.y));
        }
        onCanceled: root.solo.clearPath()
    }

    Keys.onPressed: event => {
        if (!root.accepting || root.columns === 0) {
            return;
        }
        root.keyboardUsed = true;
        const row = Math.floor(root.focusIndex / root.columns);
        const column = root.focusIndex % root.columns;
        switch (event.key) {
        case Qt.Key_Left:
            root.focusIndex = row * root.columns + Math.max(0, column - 1);
            event.accepted = true;
            break;
        case Qt.Key_Right:
            root.focusIndex = row * root.columns + Math.min(root.columns - 1, column + 1);
            event.accepted = true;
            break;
        case Qt.Key_Up:
            root.focusIndex = Math.max(0, row - 1) * root.columns + column;
            event.accepted = true;
            break;
        case Qt.Key_Down:
            root.focusIndex = Math.min(root.rows - 1, row + 1) * root.columns + column;
            event.accepted = true;
            break;
        case Qt.Key_Space:
            root.solo.tapTile(root.focusIndex);
            event.accepted = true;
            break;
        case Qt.Key_Return:
        case Qt.Key_Enter:
            root.solo.done();
            event.accepted = true;
            break;
        case Qt.Key_Backspace:
            root.solo.removeLast();
            event.accepted = true;
            break;
        case Qt.Key_Escape:
            if (root.solo.path.length > 0) {
                root.solo.clearPath();
                event.accepted = true;
            }
            break;
        default:
            break;
        }
    }
}
