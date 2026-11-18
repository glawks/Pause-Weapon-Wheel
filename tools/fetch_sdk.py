#!/usr/bin/env python3
"""Download the exact published OpenRAC SDK pinned in sdk.lock."""
import hashlib
import json
from pathlib import Path
from urllib.request import urlopen
from zipfile import ZipFile
from io import BytesIO

ROOT = Path(__file__).resolve().parent.parent
lock = json.loads((ROOT / 'sdk.lock').read_text())
version = lock['version']
target = ROOT / '.sdk' / f'openrac-mod-sdk-{version}'
if (target / 'cmake' / 'OpenRACModConfig.cmake').is_file():
    print(target)
    raise SystemExit(0)
with urlopen(lock['url'], timeout=60) as response:
    data = response.read()
actual = hashlib.sha256(data).hexdigest()
if actual != lock['sha256']:
    raise SystemExit(f'SDK SHA-256 mismatch: expected {lock["sha256"]}, got {actual}')
with ZipFile(BytesIO(data)) as archive:
    for item in archive.infolist():
        parts = Path(item.filename).parts
        if '..' in parts or Path(item.filename).is_absolute():
            raise SystemExit('Unsafe SDK archive path')
    archive.extractall(ROOT / '.sdk')
if not (target / 'cmake' / 'OpenRACModConfig.cmake').is_file():
    raise SystemExit(f'SDK missing expected cmake package at {target}')
print(target)
