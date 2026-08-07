#!/usr/bin/env python3
"""Create a deterministic Ed25519-signed Shengshi Baiye update manifest."""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import os
from pathlib import Path

from nacl.signing import SigningKey


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--channel", choices=("stable", "beta"), required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--minimum-launcher-version", default="0.1.0")
    parser.add_argument("--url", required=True)
    parser.add_argument("--file", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    key_text = os.environ.get("UPDATE_SIGNING_KEY", "")
    if not key_text:
        raise SystemExit("UPDATE_SIGNING_KEY is required")
    key_bytes = base64.b64decode(key_text, validate=True)
    signing_key = SigningKey(key_bytes[:32])
    payload = args.file.read_bytes()
    digest = hashlib.sha256(payload).hexdigest()
    signed_message = (
        f"{args.channel}\n{args.version}\n{args.minimum_launcher_version}\n"
        f"{args.url}\n{len(payload)}\n{digest}"
    ).encode()
    signature = signing_key.sign(signed_message).signature
    manifest = {
        "channel": args.channel,
        "version": args.version,
        "minimumLauncherVersion": args.minimum_launcher_version,
        "url": args.url,
        "size": len(payload),
        "sha256": digest,
        "signature": base64.b64encode(signature).decode(),
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
