from json import loads
from os import environ, rename
from pathlib import Path
from subprocess import check_output
import shutil
import sys

exeWrapperEnvVar = "EXE_WRAPPER_PATH"
exeWrapperPath = environ.get(exeWrapperEnvVar)
if exeWrapperPath is None:
    sys.exit(f"Error: please set environment variable {exeWrapperEnvVar}")
exeWrapperPath = Path(exeWrapperPath)

# collect args for `conan graph info`
graphInfoArgs = []

# skip `conan install` and the output folder arg
outputFolderPossibleArgs = ["-of", "--output-folder"]
shouldSaveArg = True
for arg in sys.argv[3:]:
    if not shouldSaveArg:
        shouldSaveArg = True
        continue
    if any(arg.startswith(o) for o in outputFolderPossibleArgs):
        # if the option doesn't contain =, then next arg is the option's value
        shouldSaveArg = any(arg.startswith(f"{o}=") for o in outputFolderPossibleArgs)
        continue
    graphInfoArgs.append(arg)

# call `conan graph info`
zlibPkgName = "zlib"
qtPkgName = "qt"
packageFilterArg = lambda pkgName: f"--package-filter={pkgName}/*"

graphInfoExtraArgs = ["--no-remote", packageFilterArg(zlibPkgName), packageFilterArg(qtPkgName), "--filter=package_id", "--format=json"]
graphInfoJson = check_output(["conan", "graph", "info"] + graphInfoArgs + graphInfoExtraArgs)
packageNodes = loads(graphInfoJson)["graph"]["nodes"]

def packageBinPath(pkgName):
    pkg = next((pkg for pkg in packageNodes.values() if pkg["ref"].startswith(pkgName)), None)
    packageID = f"{pkg['ref']}:{pkg['package_id']}"
    return Path(check_output(["conan", "cache", "path", packageID], text=True).rstrip()) / "bin"

qtBinPath = packageBinPath(qtPkgName)

# save zlib dll directory path, ensure it ends with ;
with open(qtBinPath / "_path.txt", "w") as f:
    f.write(f"{packageBinPath(zlibPkgName)};")

# in Qt 5 rcc depends on zlib directly, others - through QtCore
qtExes = [
    "lrelease",
    "lupdate",
    "rcc",
    "uic",
]
for qtExe in qtExes:
    qtExeRealPath = qtBinPath / f"{qtExe}.exe"
    qtExeNewPath = qtBinPath / f"{qtExe}_real.exe"

    try:
        rename(qtExeRealPath, qtExeNewPath)
    except FileExistsError: # always the case on Windows
        continue

    shutil.copy2(exeWrapperPath, qtExeRealPath)

print(f"Qt executables in {qtBinPath} have been 'patched' successfully")
