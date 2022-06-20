{ stdenv, fetchFromGitHub, cmake, unzip }:

stdenv.mkDerivation rec {
  version = "6.1.2";
  name = "fmt-${version}";
  src = fetchFromGitHub {
    owner = "fmtlib";
    repo = "fmt";
    rev = "398343897f98b88ade80bbebdcbe82a36c65a980";
    sha256 = "0fzqfvwq98ir1mgmggm82xl673s1nbywn8mna50qjiqm71wqq09a";
  };



  nativeBuildInputs = [ cmake ];
  enableParallelBuilding = true;
}
