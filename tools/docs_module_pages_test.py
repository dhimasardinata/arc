from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def slug_for(rel: str) -> str:
    return rel.removesuffix(".hpp").replace("/", "-")


def headers() -> list[str]:
    root = ROOT / "components" / "arc" / "include" / "arc"
    items = ["arc.hpp"]
    items.extend(f"arc/{path.relative_to(root).as_posix()}" for path in sorted(root.rglob("*.hpp")))
    return items


class DocsModulePagesTest(unittest.TestCase):
    def test_every_public_header_has_module_page(self) -> None:
        missing = []
        for rel in headers():
            page = ROOT / "docs" / "modules" / f"{slug_for(rel)}.md"
            if not page.exists():
                missing.append(str(page.relative_to(ROOT)))

        self.assertEqual(missing, [])

    def test_module_index_links_every_header_page(self) -> None:
        index = (ROOT / "docs" / "module-pages.md").read_text(encoding="utf-8")
        missing = []
        for rel in headers():
            link = f"[`{rel}`](/modules/{slug_for(rel)})"
            if link not in index:
                missing.append(link)

        self.assertEqual(missing, [])

    def test_generated_pages_include_required_sections(self) -> None:
        required = [
            "## What It Owns",
            "## Start From Zero",
            "## Usage Pattern",
            "## Example",
            "## Simulated Output",
            "## Next Reading",
        ]
        bad = []
        for rel in headers():
            page = ROOT / "docs" / "modules" / f"{slug_for(rel)}.md"
            text = page.read_text(encoding="utf-8")
            if any(section not in text for section in required):
                bad.append(str(page.relative_to(ROOT)))

        self.assertEqual(bad, [])

    def test_module_guide_points_to_generated_index(self) -> None:
        text = (ROOT / "docs" / "modules.md").read_text(encoding="utf-8")

        self.assertIn("[Module Page Index](/module-pages)", text)

    def test_generator_has_no_duplicate_core_feature_snippet(self) -> None:
        bad = []
        pattern = re.compile(r"arc_requires\(main_requires core core\)")
        for page in (ROOT / "docs" / "modules").glob("*.md"):
            if pattern.search(page.read_text(encoding="utf-8")):
                bad.append(str(page.relative_to(ROOT)))

        self.assertEqual(bad, [])


if __name__ == "__main__":
    unittest.main()
