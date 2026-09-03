{ pkgs, lib, ... }: {
  languages.cplusplus.enable = true;

  packages = with pkgs; [
    bear
    alsa-lib
    libpulseaudio
    nlohmann_json
  ];

  env.LD_LIBRARY_PATH = lib.makeLibraryPath [
    pkgs.alsa-lib
    pkgs.libpulseaudio
  ];
}
