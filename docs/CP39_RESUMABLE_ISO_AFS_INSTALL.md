# CP39 — Resume AFS extraction even after interrupted first-run ISO installation

The first-run native installer previously waited for the complete second-stage CRI AFS unpack before publishing its installation marker. If AFS unpacking failed due to a disk write interruption, the next launcher run could treat the entire ISO as uninstalled despite a valid copied ISO and extracted ISO9660 filesystem.

The ISO install stage now writes `install.complete` only after **both the local ISO copy and the ISO9660 extraction** have succeeded. The AFS stage remains independently gated by `Extracted_AFS/afs.complete` and per-entry size verification. AFS extraction starts automatically during that first install. If it fails, the existing ISO is preserved. On a subsequent launcher run, the existing-ISO check triggers the background AFS repair job. Manual repeat calls to `install_iso` also run the AFS repair rather than recopying the ISO.

The Play action will still refuse to run until AFS is fully complete. Test coverage includes re-running the installer after removing one already-extracted top-level AFS entry. This repair changes installation reliability only; native graphics remain a separate guest AOT/GS task.
