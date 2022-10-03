{ stdenv, autoconf, automake, libtool, fetchFromGitHub, google-gflags }:

stdenv.mkDerivation rec {
  name = "glog";

  src = fetchFromGitHub {
    owner = "DubHokku";
    repo = "glog";
    rev = "367373d25d44a72bb7d3d72f5464a8ae537b7b75";
    sha256 = "0b04jpmpjir72a38vdyay81m0x9islghcf8620y780838pav7n5c";
  };

  nativeBuildInputs = [ autoconf automake libtool ];
  buildInputs = [ google-gflags ];

  preConfigure = ''
    ./autogen.sh
  '';


  enableParallelBuilding = true;
}
