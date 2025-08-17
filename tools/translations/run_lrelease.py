# Python adaptation of MuseScore's run_lrelease.sh script

import os
import glob
import subprocess

here = os.path.dirname(os.path.abspath(__file__))
ts_dir = os.path.normpath(os.path.join(here, '../../share/locale'))
qm_dir = ts_dir

ts_files = glob.glob(os.path.join(ts_dir, 'orchestrion_*.ts'))

for ts_file in ts_files:
  file_name = os.path.splitext(os.path.basename(ts_file))[0]
  qm_file = os.path.join(qm_dir, f'{file_name}.qm')
  subprocess.run(['lrelease', ts_file, '-qm', qm_file], check=True)