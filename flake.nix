{
  description = "C Development with Nix GCC or Clang";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    systems.url = "github:nix-systems/default";
    flake-parts.url = "github:hercules-ci/flake-parts";
    treefmt-nix.url = "github:numtide/treefmt-nix";
  };

  outputs =
    inputs@{ flake-parts, systems, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = import systems;
      imports = [
        inputs.treefmt-nix.flakeModule
      ];
      perSystem =
        {
          config,
          self',
          inputs',
          pkgs,
          system,
          ...
        }:
        {
          treefmt = {
            projectRootFile = "flake.nix";
            programs.nixfmt.enable = true;
            programs.clang-format.enable = true;
            programs.cmake-format.enable = true;
            programs.mdformat.enable = true;
          };

          devShells.default = pkgs.mkShell.override {
            stdenv = pkgs.clangStdenv;
          } {
            packages = with pkgs; [
              clang
              clang-tools
              cmake
              cppcheck
              ninja
              pkg-config
            ];
            buildInputs = with pkgs; [
              glfw
              libGL
              opencl-headers
              ocl-icd
              zlib
              xorg.libX11
              xorg.libXrandr
              xorg.libXinerama
              xorg.libXcursor
              xorg.libXi
              llvmPackages.openmp
              wayland
            ];
          };
        };
    };
}
