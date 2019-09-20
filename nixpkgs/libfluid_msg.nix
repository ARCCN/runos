{ stdenv, fetchFromGitHub, autoconf, automake, libtool, pkgconfig }:

stdenv.mkDerivation rec {
  name = "libfluid_msg";

  src = fetchFromGitHub {
    owner = "ARCCN";
    repo = "libfluid_msg";
    rev = "b14c20be38bb47c99a251d22fa5e1fc03f0bc769";
    sha256 = "0k3gxrkj9fxbwpbrf8a65m9akjxqfc1ah40f9g51fclln06d3wcy";
  };

  nativeBuildInputs = [ autoconf automake libtool pkgconfig ];
  buildInputs = [ ];

  preConfigure = ''
    ./autogen.sh
  '';

  CXXFLAGS = ["-std=c++11"]
    ++ stdenv.lib.optional stdenv.isDarwin "-DIFHWADDRLEN=6";

  enableParallelBuilding = true;
}
