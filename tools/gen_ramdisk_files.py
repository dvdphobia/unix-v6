#!/usr/bin/env python3
import argparse
import pathlib
import sys

def sanitize(name: str) -> str:
    out = []
    for ch in name:
        if ch.isalnum():
            out.append(ch)
        else:
            out.append('_')
    s = ''.join(out)
    if not s:
        return "rd_empty"
    if s[0].isdigit():
        s = "rd_" + s
    return "rd_" + s

def emit_array(name: str, data: bytes) -> str:
    lines = []
    lines.append(f"static const unsigned char {name}[] = {{")
    for i in range(0, len(data), 12):
        chunk = data[i:i+12]
        hexes = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append(f"    {hexes},")
    if not data:
        lines.append("    0x00,")
    lines.append("};")
    return "\n".join(lines)

def parse_manifest(path: pathlib.Path):
    entries = []
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        parts = line.split()
        if len(parts) < 3:
            raise ValueError(f"invalid manifest line: {line}")
        mode = parts[0].strip()
        dst = parts[1].strip()
        src = parts[2].strip()
        entries.append((mode, dst, src))
    return entries

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--manifest", required=True)
    ap.add_argument("--output", required=True)
    ap.add_argument("--optional", action="store_true")
    args = ap.parse_args()

    manifest = pathlib.Path(args.manifest).resolve()
    out_path = pathlib.Path(args.output)

    entries = parse_manifest(manifest)
    arrays = []
    files = []

    repo_root = manifest.parent.parent
    base_dir = repo_root
    for mode, dst, src in entries:
        src_path = (base_dir / src).resolve()
        if not src_path.exists():
            src_path = (manifest.parent / src).resolve()
        if not src_path.exists():
            if args.optional:
                print(f"warning: missing {src}", file=sys.stderr)
                continue
            raise FileNotFoundError(src)
        data = src_path.read_bytes()
        ident = sanitize(dst)
        arrays.append(emit_array(ident, data))
        files.append((dst, ident, mode))

    out_lines = []
    out_lines.append("#ifndef RAMDISK_FILES_H")
    out_lines.append("#define RAMDISK_FILES_H")
    out_lines.append("")
    out_lines.append("struct rd_file {")
    out_lines.append("    const char *path;")
    out_lines.append("    const unsigned char *data;")
    out_lines.append("    unsigned int size;")
    out_lines.append("    unsigned short mode;")
    out_lines.append("};")
    out_lines.append("")
    out_lines.extend(arrays)
    out_lines.append("")
    out_lines.append("static const struct rd_file rd_files[] = {")
    for dst, ident, mode in files:
        out_lines.append(f"    {{ \"{dst}\", {ident}, sizeof({ident}), {mode} }},")
    out_lines.append("    { 0, 0, 0, 0 },")
    out_lines.append("};")
    out_lines.append("")
    out_lines.append("#endif")
    out_path.write_text("\n".join(out_lines) + "\n")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
