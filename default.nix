{ pkgs ? import <nixpkgs> {} }:
let
  stdenv = pkgs.stdenv;
  callPackage = pkgs.callPackage;

  # Keep this list sorted
  boost165 = callPackage ./nixpkgs/boost.nix { };
  boost-hana = callPackage ./nixpkgs/boost-hana.nix { boost165 = boost165; };
  #breakpad = callPackage ./nixpkgs/breakpad.nix { };
  conan = callPackage ./nixpkgs/conan.nix { };
  cpp-netlib = callPackage ./nixpkgs/cpp-netlib.nix { boost165 = boost165; };
  fmtlib = callPackage ./nixpkgs/fmtlib.nix { };
  glog = callPackage ./nixpkgs/glog.nix { };
  libevent = callPackage ./nixpkgs/libevent.nix { };
  libfluid_base = callPackage ./nixpkgs/libfluid_base.nix { libevent = libevent; };
  libfluid_msg = callPackage ./nixpkgs/libfluid_msg.nix { };
  libpcap = callPackage ./nixpkgs/libpcap.nix { };
  libtins = callPackage ./nixpkgs/libtins.nix { boost165 = boost165; };
  #mettle = callPackage ./nixpkgs/mettle.nix { };
  range-v3 = callPackage ./nixpkgs/range-v3.nix { };
  tiny-process = callPackage ./nixpkgs/tiny-process-library.nix { };


in rec {
  runosEnv = stdenv.mkDerivation {
    name = "runos-env";

    # Derivations built for the build system (native builds).
    # Needed for the cross-build.
    nativeBuildInputs = [
      pkgs.gcc9
      pkgs.cmake
      pkgs.pkg-config
      pkgs.nodePackages.uglify-js
    ];


    # Target platform build dependencies.
    buildInputs = [
      pkgs.python3 # For tools
      pkgs.python310Packages.pyparsing
      fmtlib # String Formatting
      boost165
      boost-hana # Metaprogramming
      #mettle # Unit Testing
      #breakpad # Crash reporting
      pkgs.libedit # CLI
      cpp-netlib # REST
      #conan
      pkgs.gflags
      glog # Logging
      libevent
      libfluid_base
      libfluid_msg
      libpcap
      libtins
      range-v3
      tiny-process
      pkgs.libsForQt515.qtbase
      pkgs.jq
      pkgs.gcc14
    ]
      # Qt build is broken on OS X. Use system Qt instead.
      # ++ stdenv.lib.optional stdenv.isLinux pkgs.qt55.qtbase
      # Fix linkage when sanitizers enabled
      ++ pkgs.lib.optional stdenv.isDarwin pkgs.libcxxabi;

    NIX_QT5_TMP=true;

    shellHook = [''
      export RANGE_V3_INCLUDE_DIRS="${range-v3}"
      export GLOG_logtostderr=1
      export GLOG_colorlogtostderr=1
    ''] ++ pkgs.lib.optional stdenv.isDarwin [''
      export CMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH":/usr/local/opt/qt5
    ''];
  };
}
