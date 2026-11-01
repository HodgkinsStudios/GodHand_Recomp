# CP27 — Consistent physical ISO sector mapping

The PS2 IOP had two independently implemented file lookup paths: EE CDVDFSV SearchFile now used original ISO9660 sectors (CP26), while imported IOP sceCdSearchFile still generated synthetic virtual-disc sector numbers. Those synthetic LSNs were incompatible with the actual original ISO read path when a physical image was mounted.

This pass exposes a shared ISO9660 file descriptor lookup and changes IOP cdvdman import ordinal 10 to use the real ISO sector/size from the same implementation when PS2X_CD_IMAGE is present. The existing virtual ISO behavior remains only for configurations without a mounted physical image.

Added a regression that calls the IOP import directly against a synthetic ISO fixture and checks that the actual LSN and file length agree with EE CDVDFSV. Missing original files are not replaced by virtual-disc stand-ins.

This pass does not certify real God Hand graphics. Require a full linked AOT execution and genuine GS framebuffer for that milestone.
