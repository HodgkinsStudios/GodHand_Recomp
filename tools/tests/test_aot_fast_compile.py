"""Regressions for the opt-in fast first-boot AOT compilation mode."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class FastAOTBuildTests(unittest.TestCase):
    def test_option_defaults_off_and_does_not_drop_generated_files(self):
        source = (ROOT / 'ps2xRuntime/CMakeLists.txt').read_text()
        self.assertIn('option(PS2X_AOT_FAST_COMPILE', source)
        self.assertIn('guest C++ quickly while diagnosing native first boot" OFF)', source)
        self.assertIn('target_sources(ps2EntryRunner PRIVATE ${PS2X_AOT_SOURCE_FILES})', source)
        self.assertIn('COMPILE_OPTIONS "-O0;-g0;-fno-lto"', source)
        self.assertIn('COMPILE_OPTIONS "/Od;/GL-"', source)

    def test_only_game_development_job_opts_in(self):
        workflow = (ROOT / '.github/workflows/godhand-aot.yml').read_text()
        self.assertIn('-DPS2X_AOT_FAST_COMPILE=ON', workflow)
        self.assertIn('--limit 24 --timeout 90 --workers 2', workflow)


if __name__ == '__main__':
    unittest.main()
