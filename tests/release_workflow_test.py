"""Guard the release asset/version contract without contacting GitHub."""
import importlib.util
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("release_version", ROOT / "dist/scripts/release-version.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ReleaseWorkflowTest(unittest.TestCase):
    def test_authoritative_version_matches_documentation(self):
        version = module.release_version()
        self.assertIn(f"**Current release:** {version}", (ROOT / "README.md").read_text())
        self.assertIn(f"Version {version} (", (ROOT / "Changelog").read_text())
        self.assertIn(f'<release version="{version}"', (ROOT / "dist/unix/com.goshapps.Orange.appdata.xml").read_text())

    def test_architectures_and_publication_gate(self):
        workflow = (ROOT / ".github/workflows/orange-ci.yml").read_text()
        self.assertEqual(workflow.count("runner: ubuntu-24.04-arm"), 2)
        self.assertEqual(workflow.count("arch: x86_64"), 2)
        self.assertIn("needs: [version, linux, flatpak]", workflow)
        self.assertIn("github.ref == 'refs/heads/master'", workflow)
        self.assertIn("--notes-file release-notes.md --draft", workflow)
        self.assertIn('--target "$GITHUB_SHA"', workflow)
        self.assertIn("SHA256SUMS", workflow)

    def test_flatpak_builds_this_checkout_with_pinned_dependencies(self):
        manifest = (ROOT / "dist/flatpak/com.goshapps.Orange.yml").read_text()
        self.assertIn("app-id: com.goshapps.Orange", manifest)
        self.assertIn("path: ../..", manifest)
        self.assertNotIn("strawberry-wrapper", manifest)
        self.assertEqual(len(re.findall(r"type: archive", manifest)), len(re.findall(r"sha256: [0-9a-f]{64}", manifest)))


if __name__ == "__main__":
    unittest.main()
