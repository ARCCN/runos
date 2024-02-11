{ stdenv, fetchFromGitHub, cmake, unzip }:

stdenv.mkDerivation rec {
  version = "10.0.0";
  name = "fmt-${version}";
  src = fetchFromGitHub {
    owner = "fmtlib";
    repo = "fmt";
    rev = "a0b8a92e3d1532361c2f7feb63babc5c18d00ef2";
    sha256 = "1bsw375wpziaxk3iqywr8gi8s6l2ga222mwyv9vrrzfjad6kcmmi";
  };



  nativeBuildInputs = [ cmake ];
  enableParallelBuilding = true;
}
