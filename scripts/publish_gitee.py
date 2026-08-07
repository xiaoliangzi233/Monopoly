#!/usr/bin/env python3
"""Create a Gitee release, upload one asset, and expose its download URL."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
from urllib.parse import quote

import requests


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository", required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--asset", type=Path, required=True)
    parser.add_argument("--prerelease", action="store_true")
    parser.add_argument("--target-commitish", default="main")
    parser.add_argument("--replace-existing", action="store_true")
    args = parser.parse_args()
    token = os.environ.get("GITEE_TOKEN", "")
    if not token:
        raise SystemExit("GITEE_TOKEN is required")
    base = f"https://gitee.com/api/v5/repos/{args.repository}"
    common = {
        "access_token": token,
        "tag_name": args.tag,
        "target_commitish": args.target_commitish,
        "name": args.tag,
        "body": "Automated Neon Tycoon release",
        "prerelease": str(args.prerelease).lower(),
    }
    lookup = requests.get(
        f"{base}/releases/tags/{quote(args.tag, safe='')}",
        params={"access_token": token},
        timeout=30,
    )
    if lookup.status_code == 200 and args.replace_existing:
        old_release = lookup.json()
        deletion = requests.delete(
            f"{base}/releases/{old_release['id']}",
            data={"access_token": token},
            timeout=30,
        )
        if deletion.status_code not in (200, 204):
            raise SystemExit(f"Gitee release deletion failed ({deletion.status_code}): {deletion.text}")
        release_response = requests.post(f"{base}/releases", data=common, timeout=30)
        if not release_response.ok:
            raise SystemExit(f"Gitee release recreation failed ({release_response.status_code}): {release_response.text}")
        release = release_response.json()
    elif lookup.status_code == 200:
        release = lookup.json()
    elif lookup.status_code == 404:
        release_response = requests.post(f"{base}/releases", data=common, timeout=30)
        if not release_response.ok:
            raise SystemExit(f"Gitee release creation failed ({release_response.status_code}): {release_response.text}")
        release = release_response.json()
    else:
        raise SystemExit(f"Gitee release lookup failed ({lookup.status_code}): {lookup.text}")

    for existing in release.get("assets", []):
        if existing.get("name") == args.asset.name and existing.get("browser_download_url"):
            download_url = existing["browser_download_url"]
            break
    else:
        release_id = release["id"]
        with args.asset.open("rb") as asset_file:
            upload_response = requests.post(
                f"{base}/releases/{release_id}/attach_files",
                data={"access_token": token}, files={"file": (args.asset.name, asset_file)}, timeout=180,
            )
        if not upload_response.ok:
            raise SystemExit(f"Gitee asset upload failed ({upload_response.status_code}): {upload_response.text}")
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
