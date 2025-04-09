{ stdenv, lib, gcc9, fetchurl, fetchpatch, fetchFromGitHub, cmake, which, patches ? [ ], }:

stdenv.mkDerivation rec {
  name = "boost165";
  version = "1.69.0";

src = fetchurl {
      urls = [
        "https://sourceforge.net/projects/boost/files/boost/1.69.0/boost_1_69_0.tar.bz2"
        "https://boostorg.jfrog.io/artifactory/main/release/1.69.0/source/boost_1_69_0.tar.bz2"
      ];
      sha256 = "8f32d4617390d1c2d16f26a27ab60d97807b35440d45891fa340fc2648b04406";
    };

  nativeBuildInputs = [ gcc9 ];
  patches = 
     [ ./Binary-Operator-Before-Token.patch ];

  configurePhase = ''
    ./bootstrap.sh
  '';

  buildPhase = ''
    ./b2 --includedir=$prefix --libdir=$out/lib -j$NIX_BUILD_CORES --layout=system variant=release threading=multi link=shared runtime-link=shared debug-symbols=off toolset=gcc --without-python
  '';

  installPhase = ''
    ./b2 --includedir=$prefix --libdir=$out/lib -j$NIX_BUILD_CORES --layout=system variant=release threading=multi link=shared runtime-link=shared debug-symbols=off toolset=gcc --without-python install
  '';

  enableParallelBuilding = true;
}
