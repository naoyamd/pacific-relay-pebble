"""Prepare a separate Pebble project, leaving the original package intact."""
import json
import shutil
from pathlib import Path


ROOT = Path(__file__).absolute().parent.parent
DEST = ROOT / "work" / "reverse"
DEST.mkdir(parents=True, exist_ok=True)
shutil.copytree(ROOT / "src", DEST / "src", dirs_exist_ok=True)
shutil.copy2(ROOT / "wscript", DEST / "wscript")
main = DEST / "src" / "c" / "main.c"
main.write_text("#define PRIMARY_PACIFIC 1\n" + main.read_text(encoding="utf-8"), encoding="utf-8")
package = json.loads((ROOT / "package.json").read_text(encoding="utf-8"))
package["name"] = "pacific-relay-reverse"
package["pebble"]["displayName"] = "PACIFIC RELAY SFO"
package["pebble"]["uuid"] = "a5e64b23-78cb-4d92-a14e-4cba520fa90c"
(DEST / "package.json").write_text(json.dumps(package, indent=2) + "\n", encoding="utf-8")
print(f"Reverse project ready: {DEST}")
