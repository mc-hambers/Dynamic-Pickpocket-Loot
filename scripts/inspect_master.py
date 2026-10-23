"""Read-only TES5 master inspection. Never writes into the game directory."""
import argparse
import json
import struct
import zlib
from pathlib import Path

WANTED = {b'CLAS', b'FACT', b'RACE', b'KYWD', b'NPC_', b'ALCH', b'ARMO', b'AMMO', b'MISC', b'INGR', b'SCRL', b'SLGM'}

def subrecords(data):
    pos, extended = 0, None
    while pos + 6 <= len(data):
        tag, size = struct.unpack_from('<4sH', data, pos)
        pos += 6
        if tag == b'XXXX':
            extended = struct.unpack_from('<I', data, pos)[0]
            pos += size
            continue
        size = extended if extended is not None else size
        extended = None
        if pos + size > len(data):
            raise ValueError('Invalid subrecord length')
        yield tag, data[pos:pos + size]
        pos += size

def records(path):
    with path.open('rb') as stream:
        while header := stream.read(24):
            if len(header) != 24:
                raise ValueError('Truncated record header')
            tag, size, flags, form = struct.unpack_from('<4sIII', header)
            if tag == b'GRUP':
                continue
            if tag not in WANTED:
                stream.seek(size, 1)
                continue
            data = stream.read(size)
            if flags & 0x40000:
                expected = struct.unpack_from('<I', data)[0]
                data = zlib.decompress(data[4:])
                if len(data) != expected:
                    raise ValueError('Invalid compressed record')
            fields = list(subrecords(data))
            row = dict(type=tag.decode(), id=f'{form:08X}', flags=flags)
            for subtag, content in fields:
                if subtag == b'EDID':
                    row['editor'] = content.rstrip(b'\0').decode('utf-8', 'replace')
                if subtag in {b'CNAM', b'RNAM', b'TPLT', b'DATA', b'DNAM', b'BOD2', b'ENIT', b'KWDA', b'ACBS', b'SNAM', b'FNAM', b'YNAM', b'ZNAM', b'ETYP'}:
                    row.setdefault('fields', {}).setdefault(subtag.decode(), []).append(content.hex())
            yield row

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('master', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    rows = list(records(args.master))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(rows, indent=2), encoding='utf-8')
    counts = {t: sum(r['type'] == t for r in rows) for t in sorted({r['type'] for r in rows})}
    print(json.dumps(counts))

if __name__ == '__main__':
    main()
