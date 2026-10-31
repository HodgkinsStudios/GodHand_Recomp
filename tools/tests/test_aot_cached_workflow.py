import sys
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORKFLOW = ROOT / '.github/workflows/godhand-link.yml'

class WorkflowTests(unittest.TestCase):
    def test_batch_saves_before_possible_cancel(self):
        s = WORKFLOW.read_text()
        self.assertIn('cancel-in-progress: false', s)
        self.assertIn('CMAKE_EXPORT_COMPILE_COMMANDS=ON', s)
        self.assertIn('CMAKE_CXX_COMPILER_LAUNCHER=ccache', s)
        self.assertIn('actions/cache/restore@v4', s)
        for n in range(1,5):
            self.assertIn(f'Compile generated C++ batch {n} of 4', s)
            self.assertIn(f'Save compiled C++ cache after batch {n}', s)
        self.assertEqual(s.count('actions/cache/save@v4'), 4)
        self.assertIn('Upload full compiler logs', s)
        self.assertIn('GODHAND_GS_FRAME_PPM', s)
        self.assertIn('Fail job if native game has not linked yet', s)
        self.assertIn('GodHand_Recomp_Game.sha256', s)
        self.assertNotIn('> "$RUNNER_TEMP/godhand-build.log" 2>&1', s)

    def test_stream_progress_without_fake_link(self):
        s=WORKFLOW.read_text()
        self.assertIn('cmake --build build-game --target ps2EntryRunner --parallel 4',s)
        self.assertIn('2>&1 | tee "$RUNNER_TEMP/godhand-link.log"',s)
        self.assertIn('steps.batch4.outcome == \'success\'',s)
        self.assertIn('native_link_exit_code=skipped_due_to_batch_compilation_failure',s)

if __name__ == '__main__':
    unittest.main()
