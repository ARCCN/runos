{ stdenv, autoconf, automake, libtool, fetchFromGitHub, gflags }:

stdenv.mkDerivation rec {
  name = "glog";

  src = fetchFromGitHub {
    owner = "google";
    repo = "glog";
    rev = "96a2f23dca4cc7180821ca5f32e526314395d26a";
    sha256 = "1xd3maiipfbxmhc9rrblc5x52nxvkwxp14npg31y5njqvkvzax9b";
  };

  nativeBuildInputs = [ autoconf automake libtool ];
  buildInputs = [ gflags ];

  preConfigure = ''
    ./autogen.sh
  '';


  enableParallelBuilding = true;
}
