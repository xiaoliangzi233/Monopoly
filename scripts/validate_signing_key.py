#!/usr/bin/env python3
"""Validate that a local Ed25519 private key matches the launcher public key."""

from __future__ import annotations

import argparse
import base64
import os

from nacl.signing import SigningKey


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--public-key-hex", required=True)
    args = parser.parse_args()

    key_text = os.environ.get("UPDATE_SIGNING_KEY", "")
    if not key_text:
        raise SystemExit("UPDATE_SIGNING_KEY is missing")
    try:
        key_bytes = base64.b64decode(key_text, validate=True)
        signing_key = SigningKey(key_bytes[:32])
    except Exception as error:
        raise SystemExit(f"UPDATE_SIGNING_KEY is invalid: {error}") from error
    actual = signing_key.verify_key.encode().hex()
    if actual.lower() != args.public_key_hex.lower():
        raise SystemExit("UPDATE_SIGNING_KEY does not match the public key embedded in existing launchers")
    print("Signing key matches the production launcher public key.")


if __name__ == "__main__":
    main()
