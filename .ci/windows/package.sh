#!/usr/bin/env bash
set -e

SOURCE_DIR="$1"
BINARY_DIR="$2"
INSTALL_FILES="$3"

if [ -z "$SOURCE_DIR" ] || [ -z "$BINARY_DIR" ] || [ -z "$INSTALL_FILES" ]; then
    echo "Usage: $0 <SOURCE_DIR> <BINARY_DIR> <INSTALL_FILES_DIR>"
    exit 1
fi

echo "Staging files to ${INSTALL_FILES}..."
rm -rf "${INSTALL_FILES}"
mkdir -p "${INSTALL_FILES}"

if [ -z "$MINGW_PREFIX" ]; then
    if [ -d "/ucrt64" ]; then
        MINGW_PREFIX="/ucrt64"
    elif [ -d "/mingw64" ]; then
        MINGW_PREFIX="/mingw64"
    else
        MINGW_PREFIX="/usr"
    fi
fi

DLLDIR="${MINGW_PREFIX}/bin"
DLLDIR_LIB="${MINGW_PREFIX}/lib"

# Interception driver
mkdir -p "${INSTALL_FILES}/interception"
if [ -d "${SOURCE_DIR}/3rdparty/interception" ]; then
    cp -f "${SOURCE_DIR}/3rdparty/interception/"* "${INSTALL_FILES}/interception/" 2>/dev/null || true
fi

# DDEngine
if [ -d "${SOURCE_DIR}/3rdparty/ddengine" ]; then
    cp -f "${SOURCE_DIR}/3rdparty/ddengine/"*.dll "${INSTALL_FILES}/" 2>/dev/null || true
fi

# Core DLL & executables
find "${BINARY_DIR}/core" -maxdepth 2 -name 'veyon-core.dll' -exec cp -f {} "${INSTALL_FILES}/" \; 2>/dev/null || true
find "${BINARY_DIR}" -mindepth 2 -name 'veyon-*.exe' -exec cp -f {} "${INSTALL_FILES}/" \; 2>/dev/null || true

# Plugins
mkdir -p "${INSTALL_FILES}/plugins"
find "${BINARY_DIR}/plugins" -name '*.dll' -exec cp -f {} "${INSTALL_FILES}/plugins/" \; 2>/dev/null || true
# Copy lib*.dll, vnchooks.dll, and built helper libraries to root as well for loader resolution
find "${BINARY_DIR}" -name 'vnchooks*.dll' -exec cp -f {} "${INSTALL_FILES}/" \; 2>/dev/null || true
find "${BINARY_DIR}" -name 'lib*.dll' -exec cp -f {} "${INSTALL_FILES}/" \; 2>/dev/null || true

# Translations
mkdir -p "${INSTALL_FILES}/translations"
LRELEASE_BIN=$(which lrelease-qt6 2>/dev/null || which lrelease 2>/dev/null || ls ${MINGW_PREFIX}/bin/lrelease* 2>/dev/null | head -n 1 || true)
if [ -n "$LRELEASE_BIN" ] && [ -x "$LRELEASE_BIN" ]; then
    echo "Compiling translations using $LRELEASE_BIN..."
    for ts in "${SOURCE_DIR}/translations/"*.ts; do
        if [ -f "$ts" ]; then
            qm_name=$(basename "$ts" .ts).qm
            "$LRELEASE_BIN" "$ts" -qm "${INSTALL_FILES}/translations/${qm_name}" 2>/dev/null || true
        fi
    done
fi
find "${BINARY_DIR}/translations" -name '*.qm' -exec cp -f {} "${INSTALL_FILES}/translations/" \; 2>/dev/null || true
find "${SOURCE_DIR}/translations" -name '*.qm' -exec cp -f {} "${INSTALL_FILES}/translations/" \; 2>/dev/null || true

# Pre-synchronized Keys
if [ -d "${SOURCE_DIR}/keys" ]; then
    mkdir -p "${INSTALL_FILES}/keys"
    cp -rf "${SOURCE_DIR}/keys/"* "${INSTALL_FILES}/keys/" 2>/dev/null || true
fi

# Qt Plugins
mkdir -p "${INSTALL_FILES}/crypto"
find "${MINGW_PREFIX}/lib/qca-qt6" "${MINGW_PREFIX}" -name 'libqca-ossl.dll' -exec cp -f {} "${INSTALL_FILES}/crypto/" \; 2>/dev/null || true

mkdir -p "${INSTALL_FILES}/imageformats"
find "${MINGW_PREFIX}/share/qt6/plugins/imageformats" "${MINGW_PREFIX}" -name 'qjpeg.dll' -exec cp -f {} "${INSTALL_FILES}/imageformats/" \; 2>/dev/null || true

mkdir -p "${INSTALL_FILES}/platforms"
find "${MINGW_PREFIX}/share/qt6/plugins/platforms" "${MINGW_PREFIX}" -name 'qwindows.dll' -exec cp -f {} "${INSTALL_FILES}/platforms/" \; 2>/dev/null || true

mkdir -p "${INSTALL_FILES}/styles"
find "${MINGW_PREFIX}/share/qt6/plugins/styles" "${MINGW_PREFIX}" -name 'qwindowsvistastyle.dll' -exec cp -f {} "${INSTALL_FILES}/styles/" \; 2>/dev/null || true

mkdir -p "${INSTALL_FILES}/tls"
find "${MINGW_PREFIX}/share/qt6/plugins/tls" "${MINGW_PREFIX}" -name 'qopensslbackend.dll' -exec cp -f {} "${INSTALL_FILES}/tls/" \; 2>/dev/null || true

# Verify critical platform plugin
if [ ! -f "${INSTALL_FILES}/platforms/qwindows.dll" ]; then
    echo "ERROR: platforms/qwindows.dll was not found in ${MINGW_PREFIX}!"
    exit 1
fi

# Qt6 Core DLLs
for qtdll in Qt6Core.dll Qt6Core5Compat.dll Qt6Gui.dll Qt6Widgets.dll Qt6Network.dll Qt6Concurrent.dll Qt6HttpServer.dll Qt6WebSockets.dll; do
    find "${DLLDIR}" -name "$qtdll" -exec cp -f {} "${INSTALL_FILES}/" \; 2>/dev/null || true
done

# Run automated recursive dependency resolver & auditor
echo "Resolving all runtime DLL dependencies recursively..."
PYTHON_BIN=$(which python3 2>/dev/null || which python 2>/dev/null || echo "python")
"$PYTHON_BIN" "${SOURCE_DIR}/.ci/windows/deploy_dependencies.py" "${INSTALL_FILES}" "${MINGW_PREFIX}"

# Licenses
cp -f "${SOURCE_DIR}/COPYING" "${INSTALL_FILES}/COPYING" 2>/dev/null || true
cp -f "${SOURCE_DIR}/COPYING" "${INSTALL_FILES}/LICENSE.TXT" 2>/dev/null || true
cp -f "${SOURCE_DIR}/README.md" "${INSTALL_FILES}/README.TXT" 2>/dev/null || true

# NSIS Scripts and Assets
mkdir -p "${INSTALL_FILES}/nsis"
cp -rf "${SOURCE_DIR}/nsis/"* "${INSTALL_FILES}/nsis/" 2>/dev/null || true
cp -rf "${SOURCE_DIR}/nsis/"* "${INSTALL_FILES}/" 2>/dev/null || true
if [ -f "${BINARY_DIR}/nsis/veyon.nsi" ]; then
    cp -f "${BINARY_DIR}/nsis/veyon.nsi" "${INSTALL_FILES}/" 2>/dev/null || true
elif [ -f "${SOURCE_DIR}/nsis/veyon.nsi" ]; then
    cp -f "${SOURCE_DIR}/nsis/veyon.nsi" "${INSTALL_FILES}/" 2>/dev/null || true
fi

echo "Staging completed. Running makensis..."
cd "${INSTALL_FILES}"
makensis veyon.nsi

echo "Moving generated installer to ${BINARY_DIR}..."
cp -f veyon-*setup.exe "${BINARY_DIR}/" 2>/dev/null || true
cp -f veyon-*setup.exe "${SOURCE_DIR}/" 2>/dev/null || true
echo "Installer created successfully!"
