{ stdenv, fetchFromGitHub, cmake }:

stdenv.mkDerivation rec {
  version = "1.0.5";
  name = "tiny-process-library-${version}";
  src = fetchFromGitHub {
    owner = "eidheim";
    repo = "tiny-process-library";
    rev = "v${version}";
    sha256 = "17lsn4lcanhlwb0gli1jw55il296nbjdb34fy03biy69smsrxz53";
  };

  installPhase = ''
    mkdir -p $out/lib
    install libtiny-process-library.a $out/lib
    mkdir -p $out/include
    install $src/process.hpp $out/include 
  '';

  nativeBuildInputs = [ cmake ];
  enableParallelBuilding = true;
}
