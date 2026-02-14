#!/usr/bin/env python3
"""
Post-build script untuk rename firmware dengan naming convention
Format: SIPETARUNG_v1.0.0_YYYYMMDD_HHMMSS.bin
"""

import os
import shutil
from datetime import datetime

Import("env")

def rename_firmware(source, target, env):
    """Rename firmware file dengan naming convention"""
    
    firmware_version = env.GetProjectOption("build_flags", "").split("FIRMWARE_VERSION=\"")[1].split("\"")[0] if "FIRMWARE_VERSION" in str(env.GetProjectOption("build_flags", "")) else "1.0.0"
    firmware_name = "SIPETARUNG"
    
    # Format: SIPETARUNG_v1.0.0_YYYYMMDD_HHMMSS.bin
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    new_firmware_name = f"{firmware_name}_v{firmware_version}_{timestamp}.bin"
    
    # Path firmware
    firmware_path = target[0]
    build_dir = os.path.dirname(firmware_path)
    new_firmware_path = os.path.join(build_dir, new_firmware_name)
    
    # Rename firmware
    if os.path.exists(firmware_path):
        shutil.copy(firmware_path, new_firmware_path)
        print(f"\n✓ Firmware renamed: {new_firmware_name}")
        print(f"  Path: {new_firmware_path}")
    else:
        print(f"⚠ Warning: Firmware file not found: {firmware_path}")

# Hook script
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", rename_firmware)
