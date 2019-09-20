{ stdenv, cmake, fetchFromGitHub }:

stdenv.mkDerivation rec {
  name = "range-v3";

  nativeBuildInputs = [ cmake ];

  src = fetchFromGitHub {
    owner = "ericniebler";
    repo = "range-v3";
    rev = "793e7215777172f504c5c9a6c71aeed4b6a8f122";
    sha256 = "13a7njwq0ab6i2p1ri0kyxymk9bdsbn67gpq03ja3zcrpi7hp49h";
  };

  dontBuild = true;
  installPhase = ''
    cp -r "$src"/include "$prefix"
  '';
}
