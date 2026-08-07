#!/usr/bin/env python3
"""Verify signed manifests and stream-check their release packages."""

from __future__ import annotations

import argparse
import base64
import hashlib
import os

import requests
from nacl.signing import VerifyKey


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--public-key-hex", default=os.environ.get("UPDATE_PUBLIC_KEY_HEX", ""))
    parser.add_argument("--manifest", action="append", required=True)
    args = parser.parse_args()
    if len(args.public_key_hex) != 64:
        raise SystemExit("A 32-byte Ed25519 public key is required")
    verifier = VerifyKey(bytes.fromhex(args.public_key_hex))

    for source in args.manifest:
        response = requests.get(source, timeout=30)
        response.raise_for_status()
        manifest = response.json()
        message = (
            f"{manifest['channel']}\n{manifest['version']}\n{manifest['minimumLauncherVersion']}\n"
            f"{manifest['url']}\n{manifest['size']}\n{manifest['sha256']}"
        ).encode()
        verifier.verify(message, base64.b64decode(manifest["signature"], validate=True))

        digest = hashlib.sha256()
        size = 0
        with requests.get(manifest["url"], stream=True, timeout=180) as package:
            package.raise_for_status()
            for chunk in package.iter_content(1024 * 1024):
                digest.update(chunk)
                size += len(chunk)
        if size != manifest["size"]:
            raise SystemExit(f"{source}: size mismatch ({size} != {manifest['size']})")
        if digest.hexdigest() != manifest["sha256"]:
            raise SystemExit(f"{source}: SHA-256 mismatch")
        print(f"verified {source}: {size} bytes, sha256={digest.hexdigest()}")


if __name__ == "__main__":
    main()
