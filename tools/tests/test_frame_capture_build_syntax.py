"""Regression for CP04 multiline literal that broke all platform toolchain builds."""
from pathlib import Path
import subprocess
import tempfile
import unittest


class GuestFrameCaptureBuildSyntaxTests(unittest.TestCase):
    def test_capture_logging_literal_compiles(self):
        root = Path(__file__).resolve().parents[2]
        source = (root / 'ps2xRuntime/src/lib/ps2_runtime.cpp').read_text()
        self.assertIn('<< target.string() << " (" << width << "x" << height << ")\\n";', source)
        # Compile the exact printing statement under a standards-conforming
        # C++ compiler instead of relying on fragile substring detection alone.
        line = next(x for x in source.splitlines() if '<< target.string()' in x)
        program = f'''#include <iostream>\n#include <string>\nint main() {{\nconst struct {{ std::string string() const {{ return "path"; }} }} target;\nint width=64, height=32;\nstd::cout {line}\n}}\n'''
        with tempfile.TemporaryDirectory() as folder:
            result = subprocess.run(['c++', '-std=c++17', '-fsyntax-only', '-x', 'c++', '-'],
                                    input=program, text=True, capture_output=True, cwd=folder)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == '__main__':
    unittest.main()
