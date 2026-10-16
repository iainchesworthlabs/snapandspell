import QtQuick
import QtQuick3D

// One die of the board scene: a lit, bevelled cube that carries its die's six
// faces, one on each side, and a flat shadow on the floor. A dead jewel is the
// same cube with the atlas's jewel on every side (FaceAtlas.qml). BoardScene
// places it every frame; nothing here moves by itself.
//
// The mesh is DieGeometry's (one for each die, so that the letter on each face
// can be turned a quarter turn at a time, for the face that ends up on top to
// read upright) and the six faces are the die's block of the board's atlas (3
// cells across and 2 down, a cell to a face), picked with the texture's own
// offset and scale, so a board of any size is one texture.
Node {
    id: slot

    required property int index
    property Item atlas
    property int columns: 1
    property int rows: 1
    property real size: 80
    // The block of the atlas this die shows: its own, which is its die's, unless
    // the board gave no faces and the dice are interchangeable (then it is the
    // cell of the tile at the place the die goes to).
    property int cell: index

    // What BoardScene moves: the die itself (relative to its cell's centre on
    // the floor) and its shadow.
    property alias body: body
    property alias shadow: shadow
    property alias geometry: dieGeometry
    // The cube itself, which BoardScene turns to the face that is up (the staged
    // script's dice; the simulated ones are turned by `body`).
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

            geometry: DieGeometry {
                id: dieGeometry

                size: slot.size
            }
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
            positionU: (slot.cell % slot.columns) / slot.columns
            // A sourceItem's rows count from the bottom of the texture.
            positionV: (slot.rows - 1 - Math.floor(slot.cell / slot.columns)) / slot.rows
        }
    }
}
