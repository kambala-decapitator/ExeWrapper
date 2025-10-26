# ExeWrapper

This is an experimental Windows utility aimed to simplify building Qt projects when Qt is built / installed with Conan package manager and depends on dynamic zlib library.

The culprit here is that various "host" (build-time) Qt executables like `uic.exe` depend on zlib which in turn requires that the directory containing `zlib.dll` is added to `PATH` (as Conan stores packaged libraries in separate directories). It's not a problem when you build project in terminal because you can easily inject proper `PATH` using `conanrun.(bat|ps1)` script, but it quickly becomes inconvenient as soon as you want to work in an IDE, which means that you must launch your IDE in a modified `PATH` environment (unless an IDE allows to configure build environment separately, but still it may not allow you to execute a preparation script also rendering it inconvenient).

## The workaround (this utility)

Since we know which executables require `zlib.dll`, we can hack a wrapper executable whose sole purpose would be to call the actual Qt exe with properly configured `PATH`. Unfortunately, it's not as trivial as on UNIX-family operating systems where basically anything can be an executable, therefore a real win32 exe/program is required.

So far it has only been tested on Qt 5 and its executables from `qtbase` and `qttools` modules that are the most important ones (if you forget about QML).

## How to use it

First, get the executable. You can either download it from the [repo Releases](https://github.com/kambala-decapitator/ExeWrapper/releases) or compile on your own (it's a trivial CMake project).

Next, you need a Conan v2 "project" that consumes Qt. It could be in any form (with or without conanfile), but it's important that you have a `conan install` command for your project at hand.

Now we're almost ready to launch the Python script that does all the magic.

1. Open terminal.
2. Define an environment variable called `EXE_WRAPPER_PATH` whose value is a path (preferably absolute) to the `exewrapper.exe` utility.
3. Note down the `conan install` command that you have used, let's say it is in form `conan install <params>`.
4. Execute the Python script that resides in this repo: `python exewrapper.py conan install <params>`. It's not a mistake: you should pass your `conan install` command as parameters to the script.
5. In the bottom of the script output it should say that all was successful (most of the output comes directly from Conan).

That's all! Now you can work with your project as always, without thinking of setting up `PATH`.

## How the magic works

1. The Python script parses your `conan install` command and transforms it into `conan graph info` to fetch info about Qt and zlib packages that are used by your project.
2. The directory path containing `zlib.dll` is written to a file inside Qt package (to its `bin` directory where the executables are).
3. The necessary Qt executables are "patched" (simply renamed actually) and this utility takes their place.
4. When one of these executables is launched (by a build system, by you manually etc.), it reads the file from 2) and uses its contents as a new `PATH` entry for the actual Qt executable. And launches the executable of course acting like a transparent proxy.
