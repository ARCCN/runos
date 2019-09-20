{ stdenv, autoconf, automake, libtool, fetchFromGitHub, google-gflags }:

stdenv.mkDerivation rec {
  name = "glog";

  src = fetchFromGitHub {
    owner = "ARCCN";
    repo = "glog";
    rev = "033e2ccfee54ba3f8400e76929e511f2f698f4db";
    sha256 = "0q6dp96byggnkp1hnz52jqn88rpxhbpyy82k4i6mxvjgjgikhba9";
  };

  nativeBuildInputs = [ autoconf automake libtool ];
  buildInputs = [ google-gflags ];

  preConfigure = ''
    ./autogen.sh
  '';


  enableParallelBuilding = true;
}
