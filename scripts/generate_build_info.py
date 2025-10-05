from datetime import datetime, timezone

from SCons.Script import Import

Import("env")

now = datetime.now(timezone.utc)
timestamp = now.strftime("%Y-%m-%d %H:%M:%S %Z") or now.isoformat()

env.Append(BUILD_FLAGS=[f'-DBUILD_TIMESTAMP="{timestamp}"'])
