"""Test generic native AOT entry promotion without any retail game data."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

script = Path(__file__).resolve().parents[1] / 'promote_aot_entry.py'
spec = importlib.util.spec_from_file_location('promote_aot_entry', script)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

FUNC = '''#include <ps2_runtime.h>
// Function: sub_00336498
// Address: 0x336498 - 0x336510
void sub_00336498_0x336498(uint8_t* rdram, R5900Context* ctx, PS2Runtime *runtime) {
    switch (ctx->pc) {
        case 0x3364c0u: goto label_3364c0;
        default: break;
    }
    ctx->pc = 0x336498u;
label_336498:
    // 0x336498: 0x3e00008 jr $ra
    ctx->pc = 0x336498u;
    // 0x3364f8: 0x3c02003e lui $v0,0x3e
    ctx->pc = 0x3364f8u;
    SET_GPR_S32(ctx, 2, 123);
    // 0x3364fc: 0x3c03003e lui $v1,0x3e
    ctx->pc = 0x3364fcu;
}
'''
REG = '''extern const uint32_t g_ps2RecompiledFunctionTableBase = 0x336498u;
extern const uint32_t g_ps2RecompiledFunctionTableEnd = 0x336510u;
PS2Runtime::RecompiledFunction g_ps2RecompiledFunctionTable[30u] = {};
struct GeneratedFunctionTableInitializer {
 GeneratedFunctionTableInitializer() {
   g_ps2RecompiledFunctionTable[0] = sub_00336498_0x336498; // 0x336498
 }
};
'''

class TestPromote(unittest.TestCase):
    def test_true_indirect_entry(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'sub_00336498_0x336498.cpp').write_text(FUNC)
            (root / 'register_functions.cpp').write_text(REG)
            (root / 'diag.txt').write_text(
                '[guest-branch:missing-target] kind=IndirectCall source=0x330394 target=0x3364f8')
            report = mod.run(root, root / 'diag.txt')
            self.assertEqual(len(report['promoted']), 1)
            code = (root / 'sub_00336498_0x336498.cpp').read_text()
            table = (root / 'register_functions.cpp').read_text()
            self.assertIn('case 0x3364f8u: goto label_3364f8;', code)
            self.assertIn('label_3364f8:', code)
            self.assertIn('= sub_00336498_0x336498; // 0x3364f8 promoted', table)
            self.assertEqual(mod.run(root, root / 'diag.txt')['promoted'], [])

    def test_reject_noninstruction(self):
        with self.assertRaises(mod.InvalidPromotion):
            mod.promote_one(FUNC, 0x3364e4)

    def test_reject_delay_slot(self):
        code = FUNC.replace('lui $v0,0x3e', 'lui $v0,0x3e (Delay Slot)')
        with self.assertRaises(mod.InvalidPromotion):
            mod.promote_one(code, 0x3364f8)

    def test_retains_all_prior_promoted_entries(self):
        import json
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'sub_00336498_0x336498.cpp').write_text(FUNC)
            (root / 'register_functions.cpp').write_text(REG)
            (root / 'diag.txt').write_text(
                '[EE:main-dormant-cause] missing_recompiled_function pc=0x3364fc')
            (root / 'previous.json').write_text(json.dumps({
                'promoted': [{'pc':'0x3364f8', 'owner':'sub_00336498_0x336498.cpp'}]}))
            result = mod.run(root, root / 'diag.txt', previous_report=root / 'previous.json')
            self.assertEqual(len(result['promoted']), 2)
            code = (root / 'sub_00336498_0x336498.cpp').read_text()
            table = (root / 'register_functions.cpp').read_text()
            self.assertIn('case 0x3364f8u:', code)
            self.assertIn('case 0x3364fcu:', code)
            self.assertIn('// 0x3364f8 promoted original entry', table)
            self.assertIn('// 0x3364fc promoted original entry', table)

    def test_no_missing_diagnostic_no_mutation(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'sub_00336498_0x336498.cpp').write_text(FUNC)
            (root / 'register_functions.cpp').write_text(REG)
            (root / 'diag.txt').write_text('no missing address')
            self.assertFalse(mod.run(root, root / 'diag.txt')['promoted'])
            self.assertEqual(REG, (root / 'register_functions.cpp').read_text())

if __name__ == '__main__':
    unittest.main()
