#!/usr/bin/env python3
"""Create a Gitee release, upload one asset, and expose its download URL."""

from __future__ import annotations

import argparse
import os
from pathlib import Path

import requests


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository", required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--asset", type=Path, required=True)
    parser.add_argument("--prerelease", action="store_true")
    args = parser.parse_args()
    token = os.environ.get("GITEE_TOKEN", "")
    if not token:
        raise SystemExit("GITEE_TOKEN is required")
    base = f"https://gitee.com/api/v5/repos/{args.repository}"
    release_response = requests.post(
        f"{base}/releases",
        data={"access_token": token, "tag_name": args.tag, "name": args.tag,
              "body": "Automated Neon Tycoon release", "prerelease": str(args.prerelease).lower()},
        timeout=30,
    )
    release_response.raise_for_status()
    release_id = release_response.json()["id"]
    with args.asset.open("rb") as asset_file:
        upload_response = requests.post(
            f"{base}/releases/{release_id}/attach_files",
            data={"access_token": token}, files={"file": (args.asset.name, asset_file)}, timeout=180,
        )
    upload_response.raise_for_status()
    result = upload_response.json()
    download_url = result.get("browser_download_url") or result.get("download_url")
    if not download_url:
        raise SystemExit("Gitee did not return an asset download URL")
    output = os.environ.get("GITHUB_OUTPUT")
    if output:
        with open(output, "a", encoding="utf-8") as stream:
            stream.write(f"download_url={download_url}\n")
    else:
        print(download_url)


if __name__ == "__main__":
    main()
