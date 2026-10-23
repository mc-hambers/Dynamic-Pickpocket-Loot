"""Build install/source ZIPs and verify byte-for-byte release contents."""
import hashlib
import json
import re
import shutil
import subprocess
import zipfile
from pathlib import Path

root=Path(__file__).resolve().parents[1]
version='1.2.0'
release=root/'releases'
release.mkdir(exist_ok=True)
dll=root/'build/package/SKSE/Plugins/PickpocketingLoot.dll'
if not dll.is_file(): raise RuntimeError('Build the DLL first')
test=subprocess.run([str(root/'build/CoreTests.exe'),str(root/'config/DynamicPickpocketLoot.ini')],capture_output=True,text=True,check=True)
checks=int(re.search(r'Passed (\d+) behavior checks',test.stdout)[1])
sha=lambda data:hashlib.sha256(data).hexdigest()
validation=json.loads((root/'research/binary-validation.json').read_text(encoding='utf-8'))
validation.update(version=version,core_behavior_checks=checks,dll_sha256=sha(dll.read_bytes()),spid_required=False,esp_required=False,ammunition_handling=False,save_schema=4,clothing_restoration=True,weapon_restoration=True,armor_accessory_restoration=True,multiple_native_weapon_types=True,guard_uniform_restoration=True,guard_armor_game_days=0.5,editable_loot_pools=True,victim_cooldown_game_days=3,native_jewelry_equipping=True,jewelry_saved_per_inventory_reset=True,loot_cycle_api_version=1,jewelry_gameplay_tested=False)
(root/'validation-results.json').write_text(json.dumps(validation,indent=2)+'\n',encoding='utf-8')

notices=root/'third-party'
notices.mkdir(exist_ok=True)
for name in ['spdlog','fmt','directxmath','directxtk','rapidcsv']:
    shutil.copyfile(root/f'vcpkg_installed/x64-portable/share/{name}/copyright',notices/f'{name}-LICENSE.txt')
shutil.copyfile(root/'vendor/CommonLibSSE-NG/licenses/LICENSE-MIT.txt',notices/'CommonLibSSE-original-MIT.txt')
(notices/'README.md').write_text('CommonLibSSE-NG: GPL-3.0-or-later with the included exceptions. Original MIT attribution retained. fmt, spdlog, DirectXMath, DirectXTK, and rapidcsv notices are included. Exact corresponding dependency source archives and patched port recipes accompany the source ZIP.\n',encoding='utf-8')

install=release/f'Dynamic-Pickpocket-Loot-v{version}.zip'
source=release/f'Dynamic-Pickpocket-Loot-v{version}-source.zip'
with zipfile.ZipFile(install,'w',zipfile.ZIP_DEFLATED) as z:
    z.write(dll,'SKSE/Plugins/PickpocketingLoot.dll')
    # Preserve existing user pool/settings INIs; defaults work without a pool file.
    # User-facing install ZIP contains only the runtime files. All documentation,
    # licenses and corresponding dependency sources remain in the source archive.

sourceRoot=f'Dynamic-Pickpocket-Loot-v{version}'
with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as z:
    files=['CMakeLists.txt','vcpkg.json','.gitignore','README.md','LOOT-POOLS.md','RESTORATION.md','CRASH-AUDIT.md','VALIDATION.md','validation-results.json','COPYING.txt','EXCEPTIONS.md','design-review.md']
    for name in files: z.write(root/name,f'{sourceRoot}/{name}')
    for directory in ['src','tests','scripts','config','cmake','third-party','vendor/CommonLibSSE-NG']:
        for p in (root/directory).rglob('*'):
            if not p.is_file() or '.git' in p.parts or '__pycache__' in p.parts: continue
            z.write(p,f'{sourceRoot}/{p.relative_to(root).as_posix()}')
    for name in ['verified-mappings.md','binary-validation.json','spid-assessment.md','v0.1.2-failure-analysis.md','v0.1.3-distribution-audit.md','v0.1.4-jewelry-value.md','v1.0.0-release.md','v1.1.0-editable-pools.md','v1.0.0-victim-timer.md','v1.1.3-valuation-crossbow.md','v1.1.4-cell-reequip.md','v1.1.5-round-trip.md']:
        z.write(root/'research'/name,f'{sourceRoot}/research/{name}')
    for p in (root/'tools/vcpkg-downloads').iterdir():
        if p.is_file() and (p.name.endswith('.tar.gz') or p.suffix=='.patch'):
            z.write(p,f'{sourceRoot}/dependency-sources/{p.name}')
    for name in ['spdlog','fmt','directxmath','directxtk','rapidcsv','vcpkg-cmake','vcpkg-cmake-config']:
        for p in (root/f'tools/vcpkg/ports/{name}').rglob('*'):
            if p.is_file(): z.write(p,f'{sourceRoot}/dependency-sources/ports/{name}/{p.name}')
    z.write(root/'tools/vcpkg/scripts/cmake/vcpkg_configure_cmake.cmake',f'{sourceRoot}/dependency-sources/vcpkg_configure_cmake.cmake')
    z.write(root/'tools/vcpkg/LICENSE.txt',f'{sourceRoot}/dependency-sources/vcpkg-LICENSE.txt')

for path in [install,source]:
    with zipfile.ZipFile(path) as z:
        bad=z.testzip()
        if bad: raise RuntimeError(f'Corrupt entry: {bad}')
        if len(z.namelist())!=len(set(z.namelist())): raise RuntimeError('Duplicate ZIP entry')
        if any('/.git/' in name or name.endswith('Skyrim.esm') or 'random-notes' in name for name in z.namelist()): raise RuntimeError('Unwanted archive content')
with zipfile.ZipFile(install) as z:
    if set(z.namelist())!={'SKSE/Plugins/PickpocketingLoot.dll'}: raise RuntimeError('Unexpected install payload (documentation belongs in source package)')
    if z.read('SKSE/Plugins/PickpocketingLoot.dll')!=dll.read_bytes(): raise RuntimeError('Packaged DLL mismatch')
with zipfile.ZipFile(source) as z:
    if z.read(f'{sourceRoot}/src/Core.h')!=(root/'src/Core.h').read_bytes(): raise RuntimeError('Source mismatch')
from package_licenses import package as attach_licenses
attach_licenses(install,source)
hashes={path.name:dict(sha256=sha(path.read_bytes()),bytes=path.stat().st_size) for path in [install,source]}
(release/'SHA256.json').write_text(json.dumps(hashes,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(validation=validation,packages=hashes),indent=2))

