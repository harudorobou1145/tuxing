"""Extract the source monument with compact indices, retaining source coordinates."""
import argparse
from pathlib import Path


def extract(source, destination):
    faces, used, counts = [], [set(), set(), set()], [0, 0, 0]
    selected = False
    kinds = ("v", "vt", "vn")
    with source.open(encoding="utf-8-sig", errors="replace") as stream:
        for line in stream:
            fields = line.split()
            if not fields:
                continue
            if fields[0] in kinds:
                counts[kinds.index(fields[0])] += 1
            elif fields[0] == "o":
                selected = " ".join(fields[1:]) == "Tour_centre_Square"
            elif fields[0] == "f" and selected:
                face = []
                for token in fields[1:]:
                    corner = []
                    for kind, value in enumerate(token.split("/")):
                        index = int(value) if value else 0
                        if index < 0:
                            index += counts[kind] + 1
                        if index:
                            if not 1 <= index <= counts[kind]:
                                raise ValueError("Invalid source OBJ index")
                            used[kind].add(index)
                        corner.append(index)
                    face.append(corner)
                faces.append(face)
    if not faces:
        raise ValueError("Tour_centre_Square not found in source")
    maps = [{old: new for new, old in enumerate(sorted(ids), 1)} for ids in used]
    blocks, counts = [[], [], []], [0, 0, 0]
    with source.open(encoding="utf-8-sig", errors="replace") as stream:
        for line in stream:
            fields = line.split(maxsplit=1)
            if fields and fields[0] in kinds:
                kind = kinds.index(fields[0])
                counts[kind] += 1
                if counts[kind] in used[kind]:
                    blocks[kind].append(line.strip())
    with destination.open("w", encoding="utf-8") as stream:
        stream.write("# Extracted from Tour_centre_Square; source coordinates retained.\n")
        for block in blocks:
            stream.write("\n".join(block) + "\n")
        stream.write("o heroes_monument\n")
        for face in faces:
            stream.write("f " + " ".join("/".join(str(maps[k][i]) if i else ""
                for k, i in enumerate(corner)) for corner in face) + "\n")
    print(f"Extracted {len(faces)} faces to {destination}")


if __name__ == "__main__":
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=root / "model/tiananmen/Tiananmen+Square+3D+Scene2.obj")
    parser.add_argument("--destination", type=Path, default=root / "model/tiananmen/heroes_monument.obj")
    args = parser.parse_args()
    extract(args.source, args.destination)
