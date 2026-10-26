"""Download the pinned source archives and verify hashes before extraction/build."""
from pathlib import Path
import argparse
import hashlib
import json
import urllib.request

repository = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--cache", type=Path, default=repository.parent / "filezilla-dark-build" / "downloads")
args = parser.parse_args()
manifest = json.loads((repository / "DEPENDENCIES.json").read_text(encoding="utf-8"))
args.cache.mkdir(parents=True, exist_ok=True)


def digest(path, algorithm):
    result = hashlib.new(algorithm)
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(chunk)
    return result.hexdigest()


receipts = []
for package in manifest["dependencies"]:
    target = args.cache / package["archive"]
    selected = "verified existing cache"
    if not target.is_file() or digest(target, package["algorithm"]) != package["digest"]:
        failures = []
        for url in package["urls"]:
            temp = target.with_suffix(target.suffix + ".tmp")
            try:
                request = urllib.request.Request(url, headers={"User-Agent": "FileZilla-Dark-source-build/1"})
                with urllib.request.urlopen(request, timeout=90) as response, temp.open("wb") as out:
                    for chunk in iter(lambda: response.read(1024 * 1024), b""):
                        out.write(chunk)
                actual = digest(temp, package["algorithm"])
                if actual != package["digest"]:
                    raise ValueError("Published source checksum mismatch")
                temp.replace(target)
                selected = url
                break
            except Exception as error:
                failures.append(f"{url}: {error}")
                temp.unlink(missing_ok=True)
        else:
            raise SystemExit("Unable to fetch verified " + package["archive"] + "\n" + "\n".join(failures))
    actual = digest(target, package["algorithm"])
    if actual != package["digest"]:
        raise SystemExit("Source cache hash mismatch: " + package["archive"])
    receipts.append({"name": package["name"], "version": package["version"],
                     "archive": package["archive"], "downloaded_from": selected,
                     "algorithm": package["algorithm"], "digest": actual,
                     "checksum_source": package["checksum_source"]})
    print(f"Verified {package['archive']} ({package['algorithm']} {actual})", flush=True)
(args.cache / "verified-sources.json").write_text(json.dumps(receipts, indent=2) + "\n", encoding="utf-8")
