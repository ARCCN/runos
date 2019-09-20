{ stdenv, breakpad }:

breakpad.overrideDerivation(oldAttrs: {
  buildPhase = if stdenv.isDarwin then ''
    XCODEBUILD="/usr/bin/xcodebuild -sdk macosx -arch $CMAKE_OSX_ARCHITECTURES"
    XCCONFIG="GCC_VERSION=com.apple.compilers.llvm.clang.1_0 \
              OTHER_CFLAGS=-stdlib=libc++ \
              OTHER_LDFLAGS=-stdlib=libc++ \
              GCC_TREAT_WARNINGS_AS_ERRORS=NO"

    echo Bulding breakpad framework
    pushd src/client/mac; $XCODEBUILD build $XCCONFIG; popd

    echo Bulding breakpad tools
    pushd src/tools/mac/dump_syms; $XCODEBUILD build $XCCONFIG; popd
    pushd src/tools/mac/symupload; $XCODEBUILD -target symupload build $XCCONFIG; popd
    pushd src/tools/mac/symupload; $XCODEBUILD -target minidump_upload build $XCCONFIG; popd

    echo Building processor
    make -j$NIX_BUILD_CORES
  '' else "";

  installPhase = if stdenv.isDarwin then ''
    # Copy framework
    mkdir -p $out/Library/Frameworks
    cp -r src/client/mac/build/Release/Breakpad.framework $out/Library/Frameworks
    BREAKPAD_FRAMEWORK=$out/Library/Frameworks/Breakpad.framework
    BREAKPAD_LIB=$BREAKPAD_FRAMEWORK/Breakpad
    install_name_tool \
      -change '@executable_path/../Frameworks/Breakpad.framework/Resources/breakpadUtilities.dylib' \
              $BREAKPAD_FRAMEWORK/Resources/breakpadUtilities.dylib \
      -id $BREAKPAD_LIB $BREAKPAD_LIB
                              

    # Copy tools
    mkdir -p $out/bin
    cp src/tools/mac/dump_syms/build/Release/dump_syms                      \
       src/tools/mac/symupload/build/Release/{symupload,minidump_upload}    \
       src/processor/{microdump_stackwalk,minidump_dump,minidump_stackwalk} \
       $out/bin

    # Copy headers
    mkdir -p $out/include/breakpad
    cp src/client/mac/handler/{exception_handler,ucontext_compat}.h            \
       src/common/scoped_ptr.h                                                 \
       src/client/mac/crash_generation/{crash_generation_server,client_info}.h \
       src/common/mac/MachIPC.h                                                \
       $out/include/breakpad

    # Fix includes
    substituteInPlace $out/include/breakpad/exception_handler.h         \
      --replace '"client/mac/handler/ucontext_compat.h"'                \
                "<breakpad/ucontext_compat.h>
                 #include<pthread.h>
                 " \
      --replace '"common/scoped_ptr.h"' "<breakpad/scoped_ptr.h>"       \
      --replace '#include "client/mac/crash_generation/crash_generation_client.h"' \
                "class CrashGenerationClient;"
    substituteInPlace $out/include/breakpad/crash_generation_server.h \
      --replace '"common/mac/MachIPC.h"' \
                "<breakpad/MachIPC.h>"
  '' else "";

  enableParallelBuilding = true;
})
