{ stdenv, fetchFromGitHub, cmake, boost165 }:

stdenv.mkDerivation rec {
  name = "boost-hana-${version}";
  version = "1.0.0";

  src = fetchFromGitHub {
    owner = "boostorg";
    repo = "hana";
    rev = "v${version}";
    sha256 = "0bp6ssbzrhyihffv5j0q2vqg8qy1nz983qrlxh9f6j9v1gsplwrl";
  };

  nativeBuildInputs = [ cmake ];
  buildInputs = [ boost165 ];

  doCheck = if stdenv.cc.isGNU then false else true;
  enableParallelBuilding = true;
}
