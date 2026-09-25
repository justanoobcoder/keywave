{
  description = "Dev shell for KeyWave";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        inherit (pkgs) lib;
      in
      {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            clang-tools
            alsa-lib
            libpulseaudio
            nlohmann_json
          ];

          LD_LIBRARY_PATH = lib.makeLibraryPath [
            pkgs.alsa-lib
            pkgs.libpulseaudio
          ];
        };
      }
    );
}
