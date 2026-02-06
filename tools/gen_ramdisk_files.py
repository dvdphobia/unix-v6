#!/usr/bin/env python3
import argparse
import pathlib
import sys
import os

#
# Generates file content arrays and a table of files (ramdisk_files.h)
# Adapted for ramdisk.c which iterates over rd_files[]
#

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
    
    # Analyze directory structure
    dirs = set()
    dirs.add('/')
    
    # Maps path -> variable name for content
    content_map = {}
    # Maps path -> mode
    mode_map = {}
    
    arrays = []
    
    repo_root = manifest.parent.parent
    base_dir = repo_root

    for mode, dst, src in entries:
        # Normalize path
        p = pathlib.PurePosixPath(dst)
        path_str = str(p)
        
        src_path = (base_dir / src).resolve()
        if not src_path.exists():
            src_path = (manifest.parent / src).resolve()
        
        if not src_path.exists():
            if args.optional:
                print(f"warning: missing {src}", file=sys.stderr)
                continue
            raise FileNotFoundError(f"Source file not found: {src} (cwd: {os.getcwd()})")
            
        data = src_path.read_bytes()
        ident = sanitize("file" + path_str)
        arrays.append(emit_array(ident, data))
        
        content_map[path_str] = (ident, len(data))
        mode_map[path_str] = mode

    # Header
    out_lines = []
    out_lines.append("#ifndef RAMDISK_FILES_H")
    out_lines.append("#define RAMDISK_FILES_H")
    out_lines.append("")
    out_lines.append("#include \"include/types.h\"")
    out_lines.append("#include \"include/param.h\"")
    out_lines.append("#include \"include/inode.h\"")
    out_lines.append("")
    
    # File contents arrays
    out_lines.extend(arrays)
    out_lines.append("")
    
    # Define struct rd_file
    out_lines.append("struct rd_file {")
    out_lines.append("    const char *path;")
    out_lines.append("    const unsigned char *data;")
    out_lines.append("    int size;")
    out_lines.append("    int mode;")
    out_lines.append("};")
    out_lines.append("")
    
    # Define rd_files array
    out_lines.append("static const struct rd_file rd_files[] = {")
    
    for path, (ident, size) in content_map.items():
        mode = mode_map[path]
        # Convert octal string to C integer (e.g. "0755" -> 0755)
        if not (mode.startswith("0") and len(mode) > 1):
            mode = "0" + mode # standard unix mode usually octal
            
        out_lines.append(f"    {{ \"{path}\", {ident}, {size}, {mode} }},")
        
    # Null terminator
    out_lines.append("    { 0, 0, 0, 0 }")
    out_lines.append("};")
    out_lines.append("")
    out_lines.append("#endif")
    
    out_path.write_text("\n".join(out_lines) + "\n")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
