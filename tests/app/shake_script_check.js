// What shake_script_test.cpp asks of src/app/prototype/shake/qml/ShakeScript.js.
// Plain JavaScript run in Qt's own engine, so the checks are of the script the
// scene plays and not of a copy: the script's own text is evaluated and run(S)
// is given its functions. It returns the list of faults (empty when all hold)
// and, in `summary`, one line for each group of checks, which the test prints.
//
// What it holds down is the part of the scene the library's roll reaches: that a
// die is drawn with its six faces, lands with the face the roll says is up,
// the letter upright, shows other faces on the way, and that none of this moves
// a die or changes where it lands.

var summary = [];

function run(S) {
    var faults = [];
    function check(ok, what) {
        if (!ok) {
            faults.push(what);
        }
    }
    function near(a, b) {
        return Math.abs(a - b) < 1e-6;
    }

    // Each of the six sides can be put up, with its letter upright.
    for (var k = 0; k < 6; ++k) {
        var q = S.faceUp(k, 0);
        var top = S.topSide(q);
        check(top.side === k && near(top.height, 1) && top.tilt < 1e-3, "side " + k + " is not up when asked");
        var r = S.quatRotate(q, S.sides[k].r);
        var u = S.quatRotate(q, S.sides[k].u);
        check(near(r[0], 1) && near(u[1], 1), "side " + k + " is not upright when it is up");
    }
    summary.push("six sides: each goes up, letter upright");

    // The mesh: 24 vertices, 12 triangles that face outward, and each side's
    // texture coordinates inside its own cell of the 3 x 2 block.
    var mesh = S.cubeMesh();
    check(mesh.positions.length === 24 && mesh.normals.length === 24 && mesh.uvs.length === 24
          && mesh.indexes.length === 36, "the mesh has the wrong sizes");
    for (var t = 0; t < 12; ++t) {
        var a = mesh.indexes[3 * t];
        var b = mesh.indexes[3 * t + 1];
        var c = mesh.indexes[3 * t + 2];
        var pa = mesh.positions[a];
        var e1 = [mesh.positions[b][0] - pa[0], mesh.positions[b][1] - pa[1], mesh.positions[b][2] - pa[2]];
        var e2 = [mesh.positions[c][0] - pa[0], mesh.positions[c][1] - pa[1], mesh.positions[c][2] - pa[2]];
        var n = [e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]];
        var normal = mesh.normals[a];
        check(n[0] * normal[0] + n[1] * normal[1] + n[2] * normal[2] > 0, "triangle " + t + " faces inward");
    }
    for (var side = 0; side < 6; ++side) {
        var column = side % 3;
        var row = Math.floor(side / 3);
        for (var vertex = 4 * side; vertex < 4 * side + 4; ++vertex) {
            var uv = mesh.uvs[vertex];
            check(uv[0] >= column / 3 - 1e-9 && uv[0] <= (column + 1) / 3 + 1e-9
                  && uv[1] >= (1 - row) / 2 - 1e-9 && uv[1] <= (1 - row) / 2 + 0.5 + 1e-9,
                  "side " + side + " reads outside its cell");
        }
    }
    summary.push("mesh: 24 vertices, 12 outward triangles, a cell to each side");

    function board(size, seed) {
        // A pretend roll: which face is up, from a small generator.
        var x = (seed * 2654435761) >>> 0;
        var up = [];
        var faces = [];
        for (var i = 0; i < size * size; ++i) {
            x = (Math.imul(x, 1664525) + 1013904223) >>> 0;
            up.push(Math.floor(x / 4294967296 * 6));
            faces.push(["A", "B", "C", "D", "E", "F"]);
        }
        return { rows: size, columns: size, faces: faces, up: up };
    }

    function layout(weighted, reduced) {
        return { tile: 100, gap: 8, pitch: 108, reduced: reduced, weighted: weighted, lidTiles: 2.45 };
    }

    // A play ends with every die on the face the board says, the letter upright,
    // and the dice show other faces on the way (not under reduced motion, which
    // does not tumble). The same board and seed make the same play.
    var scripts = [["weighted", true, false], ["staged", false, false], ["reduced", false, true]];
    var minimumSeen = { weighted: 2.5, staged: 1.5, reduced: 1 };
    for (var s = 0; s < scripts.length; ++s) {
        var name = scripts[s][0];
        var landed = 0;
        var dice = 0;
        var seenTotal = 0;
        var plays = 0;
        for (var size = 3; size <= 4; ++size) {
            for (var seed = 1; seed <= 3; ++seed) {
                var report = S.faceReport(board(size, seed), seed, layout(scripts[s][1], scripts[s][2]));
                check(report.finished, name + " " + size + "x" + size + " did not finish");
                landed += report.landed;
                dice += report.dice;
                seenTotal += report.facesSeenMean;
                plays += 1;
            }
        }
        check(landed === dice, name + ": " + landed + " of " + dice + " dice landed with the face that is up");
        var seenMean = seenTotal / plays;
        check(seenMean >= minimumSeen[name], name + ": a die showed " + seenMean.toFixed(2)
              + " faces on top on average, not " + minimumSeen[name] + " or more");
        if (name === "reduced") {
            check(seenMean === 1, "reduced motion tumbled");
        }
        summary.push(name + ": " + landed + " of " + dice + " dice land face up and upright; a die had "
                     + seenMean.toFixed(2) + " faces on top on average");
    }

    // The same board, seed and script make the same play; another seed, or
    // another face up on one die, does not.
    for (var w = 0; w < 2; ++w) {
        var weighted = w === 0;
        var lay = layout(weighted, false);
        var b = board(4, 9);
        var first = S.digest(b, 100, lay);
        check(first === S.digest(b, 100, lay), (weighted ? "weighted" : "staged") + " replay differs");
        check(first !== S.digest(b, 101, lay), (weighted ? "weighted" : "staged") + " seeds agree");
        var other = board(4, 9);
        other.up[0] = (other.up[0] + 1) % 6;
        check(first !== S.digest(other, 100, lay), (weighted ? "weighted" : "staged") + " ignores the face that is up");
    }
    summary.push("replay: the same board and seed repeat; another seed or face up does not");

    // The faces do not move a die: with another face up on every die, the
    // positions and angles are the same frame for frame, only the turn differs.
    for (var m = 0; m < 2; ++m) {
        var lay2 = layout(m === 0, false);
        var one = board(4, 5);
        var two = { rows: 4, columns: 4, faces: one.faces, up: one.up.map(function (v) { return (v + 3) % 6; }) };
        var moved = 0;
        var turned = 0;
        var times = [0, 300, 700, 1100, 1500, 2100];
        for (var ti = 0; ti < times.length; ++ti) {
            var sa = S.at(one, 77, lay2, times[ti]);
            var sb = S.at(two, 77, lay2, times[ti]);
            for (var i = 0; i < sa.count; ++i) {
                if (sa.px[i] !== sb.px[i] || sa.py[i] !== sb.py[i] || sa.pz[i] !== sb.pz[i]
                    || sa.rx[i] !== sb.rx[i] || sa.ry[i] !== sb.ry[i] || sa.rz[i] !== sb.rz[i]) {
                    moved += 1;
                }
                if (sa.tw[i] !== sb.tw[i] || sa.tx[i] !== sb.tx[i] || sa.ty[i] !== sb.ty[i] || sa.tz[i] !== sb.tz[i]) {
                    turned += 1;
                }
            }
        }
        check(moved === 0, (m === 0 ? "weighted" : "staged") + ": the faces moved " + moved + " dice");
        check(turned > 0, (m === 0 ? "weighted" : "staged") + ": the face that is up did not turn anything");
    }
    summary.push("faces do not move a die: positions and angles are the same for another set of faces up");

    // A board that is shown, not played, has its faces up at once.
    for (var v = 0; v < 2; ++v) {
        var bb = board(4, 11);
        var state = S.create(bb, 11, layout(v === 0, false));
        S.finish(state);
        var good = 0;
        for (var d = 0; d < state.count; ++d) {
            var tt = S.topSide([state.tw[d], state.tx[d], state.ty[d], state.tz[d]]);
            if (tt.side === bb.up[d] && tt.tilt < 1e-3) {
                good += 1;
            }
        }
        check(good === state.count, "a shown board has " + good + " of " + state.count + " dice face up");
    }
    summary.push("show: a board that is not played has its faces up at once");

    return faults;
}
