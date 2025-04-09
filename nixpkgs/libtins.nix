{ stdenv, fetchFromGitHub, cmake, boost165, libpcap }:

stdenv.mkDerivation rec {
  name = "libtins-${version}";
  version = "3.4";

  src = fetchFromGitHub {
    owner = "mfontanini";
    repo = "libtins";
    rev = "v${version}";
    sha256 = "001d84cpwa6pxw91vf591pzf3blg3wnbfbg29jmgv5bcprahavp5";
  };

  cmakeFlags = [
    "-DLIBTINS_ENABLE_CXX11=1"
    "-DLIBTINS_ENABLE_WPA2=0"
    "-DLIBTINS_ENABLE_ACK_TRACKER=0"
  ];

  nativeBuildInputs = [ cmake ];
  buildInputs = [ boost165 libpcap ];

  enableParallelBuilding = true;
}
