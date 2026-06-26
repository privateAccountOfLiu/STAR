#!/usr/bin/env python3
"""xxd -i replacement: convert a binary file to a C header"""
import sys, os

def xxd_i(input_path, output_path=None):
    with open(input_path, 'rb') as f:
        data = f.read()

    base_name = os.path.basename(input_path).replace('.', '_').replace('-', '_')

    lines = []
    lines.append(f"unsigned char {base_name}[] = {{")
    for i in range(0, len(data), 12):
        chunk = data[i:i+12]
        hex_str = ', '.join(f'0x{b:02x}' for b in chunk)
        if i + 12 < len(data):
            lines.append(f"  {hex_str},")
        else:
            lines.append(f"  {hex_str}")
    lines.append(f"}};")
    lines.append(f"unsigned int {base_name}_len = {len(data)};")

    output = '\n'.join(lines) + '\n'

    if output_path:
        with open(output_path, 'w') as f:
            f.write(output)
    else:
        sys.stdout.write(output)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <input_file> [output_file]", file=sys.stderr)
        sys.exit(1)
    infile = sys.argv[1]
    outfile = sys.argv[2] if len(sys.argv) > 2 else None
    xxd_i(infile, outfile)
