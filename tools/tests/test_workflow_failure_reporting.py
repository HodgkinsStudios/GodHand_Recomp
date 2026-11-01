"""Regression guard: GitHub Actions bash runs with -e even if step sets -uo."""
from pathlib import Path
import subprocess
import unittest


class WorkflowFailureReportingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.workflow = (Path(__file__).resolve().parents[2] / '.github' /
                        'workflows' / 'godhand-aot.yml').read_text()

    def test_failure_exit_status_survives_bash_e(self):
        script = 'set -uo pipefail\nSTATUS=0\nfalse || STATUS=$?\nprintf "%s\\n" "$STATUS"\n'
        result = subprocess.run(['bash', '-eo', 'pipefail', '-c', script],
                                text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), '1')

    def test_expensive_aot_requires_manual_trigger_except_validated_target_recovery(self):
        # CP16 deliberately removed broad push triggers; they caused hours of
        # unnecessary recompilation and interrupted first-boot investigations.
        self.assertIn('workflow_dispatch:', self.workflow)
        self.assertIn("paths: ['tools/seed_aot_entry.py', '.github/workflows/godhand-aot.yml', 'ps2xRecomp/src/lib/elf_parser.cpp']", self.workflow)
        for glob in ('ps2xRuntime/**', 'ps2xRecomp/**', 'ps2xAnalyzer/**',
                     'ps2xIOP/**', 'CMakeLists.txt'):
            self.assertNotIn("- '" + glob + "'", self.workflow)

    def test_record_failure_status_and_errors(self):
        self.assertIn('> "$RUNNER_TEMP/native-build.log" 2>&1 || STATUS=$?', self.workflow)
        self.assertIn('> "$RUNNER_TEMP/native-configure.log" 2>&1 || STATUS=$?', self.workflow)
        self.assertIn('> "$RUNNER_TEMP/native-execution.log" 2>&1 || STATUS=$?', self.workflow)
        self.assertIn('FIRST BUILD ERRORS', self.workflow)
        self.assertIn('native_link_exit_code=%s', self.workflow)
        self.assertIn('ninja: build stopped', self.workflow)


if __name__ == '__main__':
    unittest.main()
