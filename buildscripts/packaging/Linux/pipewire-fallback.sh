# This file is part of Orchestrion.
#
# Copyright (C) 2026 Matthieu Hodgkinson
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#
# An AppRun hook (linuxdeploy sources the AppDir's apprun-hooks/ before
# starting the app). libpipewire isn't bundled: the system's is the one to
# use, matching its PipeWire server and modules. Only where there is none is
# the bundled one put on the library path, for the app to start; it then
# plays through ALSA.

orchestrion_appdir="${APPDIR:-${this_dir:-$(dirname "$(readlink -f "$0")")}}"
if ! { ldconfig -p 2>/dev/null || /sbin/ldconfig -p 2>/dev/null; } |
    grep -q 'libpipewire-0\.3\.so\.0 '; then
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+${LD_LIBRARY_PATH}:}${orchestrion_appdir}/usr/fallback/libpipewire-0.3.so.0"
fi
unset orchestrion_appdir
