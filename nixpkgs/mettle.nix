{ stdenv, fetchFromGitHub, pythonPackages,
  pkgconfig, ninja, which, patchelf,
  boost164
}:

stdenv.mkDerivation rec {
  version = "606c30c";
  name = "mettle-${version}";

  src = fetchFromGitHub {
    owner = "jimporter";
    repo = "mettle";
    rev = "fe04d487ee68881bc241bebb6ddfbce4b1236f4a";
    sha256 = "0mkr9gaz38dvv38waa1q0jymniyh704hkvx98ds1iycr7icg4rs6";
  };

  bencode = fetchFromGitHub {
    owner = "jimporter";
    repo = "bencode.hpp";
    rev = "31d090ac38daf6c77243f6312521380beb085107";
    sha256 = "1492grkh2wg1js7vyhfb4131x8kk060ph85cdj9v00kkr3k6m69k";
  };

  packaging = pythonPackages.buildPythonPackage {
    name = "packaging";
    src = fetchFromGitHub {
      owner = "pypa";
      repo = "packaging";
      rev = "16.6";
      sha256 = "0n1dppq9xrhld6abmlfjfl034q90w3d7hyi5jlydh48rjx7i6qfg";
    };
    propagatedBuildInputs = [ pythonPackages.six pythonPackages.pyparsing ];
  };

  enum-compat = pythonPackages.buildPythonPackage {
    name = "enum-compat";
    src = fetchFromGitHub {
      owner = "jstasiak";
      repo = "enum-compat";
      rev = "0.0.2";
      sha256 = "06sha4a5nmfvy2bz029hiavg0lal32nbhd0664i1dkj839k5x2h7";
    };
    propagatedBuildInputs = [ pythonPackages.enum34 ];
  };

  doppel = pythonPackages.buildPythonPackage {
    name = "doppel";
    src = fetchFromGitHub {
      owner = "jimporter";
      repo = "doppel";
      rev = "v0.2.0";
      sha256 = "1ffadanpx060mx9iw5lg9jipc1myrmiyrayawg7ajmn3iinjwifv";
    };
    doCheck = false;
  };

  patchelf-wrapper = pythonPackages.buildPythonPackage {
    name = "patchelf-wrapper";
    src = fetchFromGitHub {
      owner = "jimporter";
      repo = "patchelf-wrapper";
      rev = "v1.0.1";
      sha256 = "0kpcprl5f34pdy0a13ywyd542ghm43nnxbzc59l2sik847bgw97j";
    };
    propagatedBuildInputs = [ which patchelf ];
    doCheck = false;
  };

  bfg9000 = pythonPackages.buildPythonPackage {
    name = "bfg9000";
    src = fetchFromGitHub {
      owner = "jimporter";
      repo = "bfg9000";
      rev = "v0.2.0";
      sha256 = "003xsv16gmn47kc7sb48nfnaksmbswhmbdscvr0q55asg2dalnxj";
    };
    propagatedBuildInputs = [
      pythonPackages.six
      pythonPackages.lxml
      pythonPackages.colorama
      packaging
      enum-compat
      ninja
      doppel
    ] ++ stdenv.lib.optionals stdenv.isLinux [
      patchelf-wrapper
    ];
    doCheck = false;
  };

  nativeBuildInputs = [ pkgconfig bfg9000 ];
  buildInputs = [ boost164 ];

  patchPhase = ''
    cp -a ${bencode}/include/bencode.hpp include
  '';
  configurePhase = ''
    export INSTALL=install
    export BOOST_INCLUDEDIR=${boost164.dev}/include
    export BOOST_LIBRARYDIR=${boost164.out}/lib
    9k --prefix $prefix build
  '';
  buildPhase = ''
    ninja -C build -j $NIX_BUILD_CORES
  '';
  checkPhase = ''
    ninja -C build -j $NIX_BUILD_CORES test
  '';
  installPhase = ''
    ninja -C build install
  '';

  fixupPhase = ''
    stdlib=$(cat ${stdenv.cc}/nix-support/cc-ldflags | sed 's/^ -L//')
    patchelf --set-rpath $stdlib:${boost164.out}/lib:$out/lib \
             $out/bin/mettle
    patchelf --set-rpath $stdlib:${boost164.out}/lib \
             $out/lib/libmettle.so
  '';

  doCheck = true;
  enableParallelBuilding = true;
}
