import QtQuick
import QtQuick3D

// One die of the board scene: a lit cube that carries its die's six faces, one
// on each side, and a flat shadow on the floor. A dead jewel is the same cube
// with the atlas's jewel on every side (FaceAtlas.qml). BoardScene places and
// turns it every frame; nothing here moves by itself.
//
// The cube is the one mesh every die shares (BoardScene's `dieMesh`, six sides
// with a cell of the block each), and the six faces are the die's block of the
// board's atlas (FaceAtlas.qml), picked with the texture's own offset and scale,
// so a board of any size is one texture and one mesh.
Node {
    id: slot

    required property int index
    property Item atlas
    property Geometry mesh
    property int columns: 1
    property int rows: 1
    property real size: 80

    // What BoardScene moves: the die itself (relative to its cell's centre on
    // the floor) and its shadow.
    property alias body: body
    property alias shadow: shadow
    // The cube itself, which BoardScene turns to the face that is up.
    property alias cube: cube

    Model {
        id: shadow

        source: "#Sphere"
        // A flat disc: the sphere squashed to a plate, and not lit.
        scale: Qt.vector3d(0.016 * slot.size, 0.016 * slot.size, 0.02)
        position: Qt.vector3d(0, 0, 0.2)
        castsShadows: false
        receivesShadows: false
        opacity: 0.0
        materials: DefaultMaterial {
            diffuseColor: "#000000"
            lighting: DefaultMaterial.NoLighting
        }
    }

    Node {
        id: body

        Model {
            id: cube

            geometry: slot.mesh
            scale: Qt.vector3d(slot.size / 100, slot.size / 100, slot.size / 100)
            castsShadows: false
            receivesShadows: false
            materials: [faceMaterial]
        }
    }

    PrincipledMaterial {
        id: faceMaterial

        roughness: 0.7
        metalness: 0.0
        specularAmount: 0.15
        baseColorMap: Texture {
            sourceItem: slot.atlas
            scaleU: 1 / slot.columns
            scaleV: 1 / slot.rows
            positionU: (slot.index % slot.columns) / slot.columns
            // A sourceItem's rows count from the bottom of the texture.
            positionV: (slot.rows - 1 - Math.floor(slot.index / slot.columns)) / slot.rows
        }
    }
}
