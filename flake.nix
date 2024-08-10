{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    utils.url = "github:numtide/flake-utils";
  };

  outputs = { nixpkgs, utils, ... }:
    utils.lib.eachSystem [ "x86_64-linux" ] (system:
      let pkgs = nixpkgs.legacyPackages.${system};
      in {
        devShells.default =
          pkgs.mkShell { packages = with pkgs; [ cmake ninja ]; };
      });
}
