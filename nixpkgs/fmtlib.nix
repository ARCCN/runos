{ stdenv, fetchurl, cmake, unzip }:

stdenv.mkDerivation rec {
  version = "3.0.1";
  name = "fmt-${version}";
  src = fetchurl {
    url = "https://github.com/fmtlib/fmt/releases/download/${version}/${name}.zip";
    sha256 = "0l4514mk83cjimynci3ghrfdchjy8cya1qa45c1fg2lsj7fg16jc";
  };

  preFixup = ''
    rm $out/include/fmt/{format,ostream}.cc
  '';

  nativeBuildInputs = [ cmake unzip ];
  enableParallelBuilding = true;
}
