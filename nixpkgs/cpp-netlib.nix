{ stdenv, boost165, cmake, fetchFromGitHub }:

stdenv.mkDerivation rec {
  name = "cpp-netlib-0.13-release";
  src = fetchFromGitHub {
    owner = "cpp-netlib";
    repo = "cpp-netlib";
    rev = "b930b8e9cd3fda583f18e7e1503872340868fa00"; # 0.13-release
    sha256 = "179q145zcapmw7b6arw4vdvz5qm11fz9mch9rsg1xifvqfwp2zn6";
  };

  postInstall = ''
    substituteInPlace $out/include/boost/network/uri/detail/uri_parts.hpp \
      --replace "make_optional" "boost::make_optional";
  '';

  cmakeFlags = [ "-DCPP-NETLIB_BUILD_TESTS=OFF -DCPP-NETLIB_BUILD_EXAMPLES=OFF" ];
  nativeBuildInputs = [ cmake ];
  buildInputs = [ boost165 ];
}
