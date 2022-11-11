{ stdenv, fetchFromGitHub, autoconf, automake, libtool, pkg-config, openssl, libevent }:

stdenv.mkDerivation rec {
  name = "libfluid_base";

  src = fetchFromGitHub {
    owner = "ARCCN";
    repo = "libfluid_base";
    rev = "4c8492229f62385d809c3bcd73c52afa8543dd19";
    sha256 = "0zhzh417vcxqyf6mhyg8ra01ax24gk39a8ddhr94ar6llfy2wrly";
  };

  nativeBuildInputs = [ autoconf automake libtool pkg-config ];
  buildInputs = [ openssl ];
  propagatedBuildInputs = [ libevent ];
  CXXFLAGS = ["-std=c++11"];

  preConfigure = ''
    ./autogen.sh
  '';

  enableParallelBuilding = true;
}
