# CP26 — ISO-backed CDVDFSV and SIF RPC boot service

The previously linked native AOT guest repeatedly remained in the original countdown at PS2 address 0x00398930 before calling sceSifBindRpc for SearchFile SID 0x80000597. CDVDFSV previously was recognized as a ROM0 module key without an implemented RPC service. The Sony-compatible EE library waits for a non-null server pointer before it searches the disc.

## Implementation
- Add a general BIOS-less IOP ROM0 CDVDFSV service, registered via the normal module manager and active only when loaded. Physical emulated IOP RPC servers retain priority.
- Register Init (0x80000592), SearchFile (0x80000597), and DiskReady (0x8000059A).
- SearchFile parses the primary ISO9660 volume descriptor and directory records directly from the actual ISO using existing host file APIs. The response includes the original disc's sector address, file length, and 32-byte sceCdlFILE metadata. A missing disc, missing filename, invalid image, or truncated request cannot be returned as a successful file lookup.
- Start the standard ROM0 service on the BIOS-less runtime's IOP reset, not by patching an individual guest routine.
- Explicit PS2X_CD_IMAGE and PS2X_CD_ROOT paths allow the launcher to mount the local installed original disc and allow CI to mount the assembled image for a genuine guest test.
- Dedicated standalone IOP unit tests for module lifecycle, ISO directory search, missing files/media, and truncated RPC messages.

## Verification and next criteria
This is an implementation aimed at an observed boot blocker, not evidence that God Hand graphics render. Verify the IOP test workflow, then the native AOT link and Xvfb guest probe. Require a real, non-uniform guest GS frame in built/frames/first_real_gs_frame.ppm before reporting visual-screen success.

Reference: PS2SDK libcdvd SearchFile implementation, https://israpps.github.io/ps2sdk/libcdvd_8c_source.html .
