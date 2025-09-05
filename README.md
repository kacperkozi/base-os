# Base OS

Base OS is a minimal, bootable operating system on a USB stick that transforms any untrusted computer into a temporary, sterile, air-gapped environment for signing transactions on the Base network.

## Project Structure

- `base-os/`: Contains the configuration and build scripts for the bootable Debian Linux distribution.
- `signing-app/`: The command-line application for transaction construction and signing.
- `mini-app-broadcaster/`: A mobile application to broadcast the signed transaction to the Base network.
- `docs/`: Project documentation, including the technical product requirements.
