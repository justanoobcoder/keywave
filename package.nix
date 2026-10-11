{
  lib,
  stdenv,
  alsa-lib,
  libpulseaudio,
  gnumake,
  makeWrapper,
  nlohmann_json,
}:
stdenv.mkDerivation {
  pname = "keywave";
  version = "0.1.0";
  src = ./.;

  nativeBuildInputs = [
    gnumake
    makeWrapper
  ];

  buildInputs = [
    nlohmann_json
  ];

  makeFlags = [
    "GIT_COMMIT=nix-build"
  ];

  installPhase = ''
    runHook preInstall
    install -D -m 755 keywave        $out/bin/keywave
    wrapProgram $out/bin/keywave \
      --prefix LD_LIBRARY_PATH : ${
        lib.makeLibraryPath [
          alsa-lib
          libpulseaudio
          stdenv.cc.cc.lib
        ]
      }
    install -D -m 644 docs/man/keywave.1  $out/share/man/man1/keywave.1
    install -D -m 644 docs/man/keywave.conf.5 $out/share/man/man5/keywave.conf.5
    runHook postInstall
  '';

  meta = {
    description = "Play sound on linux";
    homepage = "https://github.com/justanoobcoder/keywave";
    license = lib.licenses.mit;
    maintainers = "justanoobcoder";
    platforms = lib.platforms.linux;
    mainProgram = "keywave";
  };
}
