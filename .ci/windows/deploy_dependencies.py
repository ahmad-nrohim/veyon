#!/usr/bin/env python3
"""
deploy_dependencies.py
Recursively inspects all PE executables and DLLs in the staging directory,
resolves their imported DLL dependencies from the MinGW/UCRT64 environment,
and copies missing DLLs into the root staging folder.
"""

import os
import sys
import shutil
import struct

WINDOWS_SYSTEM_DLLS = {
    'kernel32.dll', 'user32.dll', 'gdi32.dll', 'advapi32.dll', 'shell32.dll',
    'ole32.dll', 'oleaut32.dll', 'ws2_32.dll', 'netapi32.dll', 'iphlpapi.dll',
    'wtsapi32.dll', 'userenv.dll', 'secur32.dll', 'shlwapi.dll', 'version.dll',
    'setupapi.dll', 'cfgmgr32.dll', 'dnsapi.dll', 'mpr.dll', 'winmm.dll',
    'imm32.dll', 'uxtheme.dll', 'dwmapi.dll', 'comdlg32.dll', 'crypt32.dll',
    'bcrypt.dll', 'ncrypt.dll', 'msvcrt.dll', 'ntdll.dll', 'rpcrt4.dll',
    'd3d11.dll', 'dxgi.dll', 'd2d1.dll', 'dwrite.dll', 'wldap32.dll',
    'authz.dll', 'winhttp.dll', 'd3d12.dll', 'comctl32.dll', 'normaliz.dll',
    'pdh.dll', 'sensapi.dll', 'propsys.dll', 'cryptbase.dll', 'sspicli.dll',
    'powrprof.dll', 'profapi.dll', 'kernelbase.dll', 'msvcp_win.dll',
    'ucrtbase.dll', 'shcore.dll', 'opengl32.dll', 'glu32.dll', 'winspool.drv',
    'dbghelp.dll', 'avrt.dll', 'hid.dll', 'usp10.dll', 'd3d9.dll', 'dxva2.dll',
    'mscms.dll'
}

def is_system_dll(dll_name):
    name = dll_name.lower()
    if name in WINDOWS_SYSTEM_DLLS:
        return True
    if name.startswith(('api-ms-win-', 'ext-ms-')):
        return True
    return False

def get_pe_imports(file_path):
    imports = []
    try:
        with open(file_path, 'rb') as f:
            data = f.read()
        if len(data) < 0x40 or data[:2] != b'MZ':
            return []
        pe_offset = struct.unpack('<I', data[0x3C:0x40])[0]
        if data[pe_offset:pe_offset+4] != b'PE\0\0':
            return []
        magic = struct.unpack('<H', data[pe_offset+24:pe_offset+26])[0]
        is_64 = (magic == 0x20b)
        opt_hdr_size = struct.unpack('<H', data[pe_offset+20:pe_offset+22])[0]
        num_sections = struct.unpack('<H', data[pe_offset+6:pe_offset+8])[0]
        
        if is_64:
            import_rva = struct.unpack('<I', data[pe_offset+144:pe_offset+148])[0]
        else:
            import_rva = struct.unpack('<I', data[pe_offset+128:pe_offset+132])[0]
        if import_rva == 0:
            return []
            
        sec_offset = pe_offset + 24 + opt_hdr_size
        sections = []
        for i in range(num_sections):
            hdr = data[sec_offset + i*40 : sec_offset + (i+1)*40]
            vsize = struct.unpack('<I', hdr[8:12])[0]
            vrva = struct.unpack('<I', hdr[12:16])[0]
            rsize = struct.unpack('<I', hdr[16:20])[0]
            rptr = struct.unpack('<I', hdr[20:24])[0]
            sections.append((vrva, vsize, rptr, rsize))
            
        def rva_to_offset(rva):
            for vrva, vsize, rptr, rsize in sections:
                if vrva <= rva < vrva + max(vsize, rsize):
                    return rptr + (rva - vrva)
            return None
            
        file_imp_offset = rva_to_offset(import_rva)
        if not file_imp_offset:
            return []
            
        idx = 0
        while True:
            desc_offset = file_imp_offset + idx * 20
            orig_first_thunk, timedate, fwd_chain, name_rva, first_thunk = struct.unpack('<IIIII', data[desc_offset:desc_offset+20])
            if name_rva == 0 and first_thunk == 0:
                break
            name_offset = rva_to_offset(name_rva)
            if name_offset:
                end = data.find(b'\0', name_offset)
                if end != -1:
                    imports.append(data[name_offset:end].decode('ascii', errors='ignore'))
            idx += 1
    except Exception as e:
        pass
    return imports

def find_candidate_dll(dll_name, search_dirs):
    dll_lower = dll_name.lower()
    for sdir in search_dirs:
        if not os.path.exists(sdir):
            continue
        # Direct check first
        direct = os.path.join(sdir, dll_name)
        if os.path.isfile(direct):
            return direct
        # Case-insensitive search in directory
        for entry in os.listdir(sdir):
            if entry.lower() == dll_lower:
                candidate = os.path.join(sdir, entry)
                if os.path.isfile(candidate):
                    return candidate
    return None

def resolve_dependencies(staging_dir, mingw_prefix):
    search_dirs = [
        os.path.join(mingw_prefix, 'bin'),
        os.path.join(mingw_prefix, 'lib'),
        os.path.join(mingw_prefix, 'share', 'qt6', 'plugins'),
        os.path.join(mingw_prefix, 'plugins')
    ]
    
    print(f"Resolving DLL dependencies for staging dir: {staging_dir}")
    print(f"MinGW search prefix: {mingw_prefix}")
    
    copied_dlls = set()
    iteration = 0
    
    while True:
        iteration += 1
        newly_copied = 0
        
        # Build map of currently available DLLs in staging_dir (case-insensitive)
        available_files = {}
        for root, _, files in os.walk(staging_dir):
            for f in files:
                available_files[f.lower()] = os.path.join(root, f)
                
        # Find all binaries in staging_dir
        binaries = []
        for root, _, files in os.walk(staging_dir):
            for f in files:
                if f.lower().endswith(('.exe', '.dll')):
                    binaries.append(os.path.join(root, f))
                    
        # Check all imports
        missing = set()
        for b in binaries:
            imps = get_pe_imports(b)
            for imp in imps:
                if is_system_dll(imp):
                    continue
                if imp.lower() not in available_files:
                    missing.add(imp)
                    
        if not missing:
            print(f"Iteration {iteration}: All dependencies resolved! No missing DLLs.")
            break
            
        print(f"Iteration {iteration}: Found {len(missing)} missing DLLs: {sorted(missing)}")
        for dll_name in missing:
            src = find_candidate_dll(dll_name, search_dirs)
            if src:
                dst = os.path.join(staging_dir, os.path.basename(src))
                shutil.copy2(src, dst)
                copied_dlls.add(os.path.basename(src))
                newly_copied += 1
                print(f"  Copied: {src} -> {dst}")
            else:
                print(f"  WARNING: Could not find {dll_name} in search dirs!")
                
        if newly_copied == 0:
            print("No more missing DLLs could be found in search dirs.")
            break
            
    # Final audit
    print("\n--- Final Dependency Audit ---")
    available_files = {}
    for root, _, files in os.walk(staging_dir):
        for f in files:
            available_files[f.lower()] = os.path.join(root, f)
            
    final_missing = {}
    binaries = []
    for root, _, files in os.walk(staging_dir):
        for f in files:
            if f.lower().endswith(('.exe', '.dll')):
                binaries.append(os.path.join(root, f))
                
    for b in binaries:
        rel_b = os.path.relpath(b, staging_dir)
        imps = get_pe_imports(b)
        for imp in imps:
            if is_system_dll(imp):
                continue
            if imp.lower() not in available_files:
                final_missing.setdefault(imp, []).append(rel_b)
                
    if final_missing:
        print("ERROR: Unresolved non-system dependencies remain:")
        for dll, users in sorted(final_missing.items()):
            print(f"  - {dll} (required by: {', '.join(users)})")
        return False
    else:
        print(f"Audit PASSED: 100% of all required DLLs are present ({len(copied_dlls)} runtime DLLs deployed).")
        return True

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: deploy_dependencies.py <staging_dir> <mingw_prefix>")
        sys.exit(1)
    staging = sys.argv[1]
    prefix = sys.argv[2]
    ok = resolve_dependencies(staging, prefix)
    if not ok:
        sys.exit(1)
