{ pkgs, lib, ... }: {
  languages.cplusplus.enable = true;

  packages = with pkgs; [
    bear
    alsa-lib
    libpulseaudio
  ];

  env.LD_LIBRARY_PATH = lib.makeLibraryPath [
    pkgs.alsa-lib
    pkgs.libpulseaudio
  ];
}
