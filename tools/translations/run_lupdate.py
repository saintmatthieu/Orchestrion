# "Translated" with AI from MuseScore's run_lupdate.sh
import os
import subprocess
import sys
from pathlib import Path

def run_indented(*args):
    """Run a command and indent its output."""
    process = subprocess.Popen(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    for line in process.stdout:
        print(f"    {line.rstrip()}")
    for line in process.stderr:
        print(f"    {line.rstrip()}", file=sys.stderr)
    process.wait()
    return process.returncode

def main():
    # Go to repository root
    os.chdir(Path(__file__).resolve().parent.parent.parent)

    # Check for -no-obsolete in LUPDATE_ARGS
    lupdate_args = os.getenv("LUPDATE_ARGS", "")
    if "-no-obsolete" in lupdate_args.split():
        print("Note: cleaning up obsolete strings")
    else:
        print('Note: preserving obsolete strings (set LUPDATE_ARGS to "-no-obsolete" to clean them up)')

    # Default arguments for lupdate
    default_lupdate_args = [
        "-recursive",
        "-tr-function-alias", "translate+=trc",
        "-tr-function-alias", "translate+=mtrc",
        "-tr-function-alias", "translate+=qtrc",
        "-tr-function-alias", "translate+=TranslatableString",
        "-tr-function-alias", "qsTranslate+=qsTrc",
        "-tr-function-alias", "QT_TRANSLATE_NOOP+=QT_TRANSLATE_NOOP_U16",
        "-extensions", "cpp,h,mm,ui,qml,js",
    ]

    lupdate = "lupdate"
    src_dir = "src"
    ts_file = "share/locale/orchestrion_en.ts"

    # Run lupdate
    print("Running", lupdate, *default_lupdate_args,
          lupdate_args, src_dir, "-ts", ts_file)
    run_indented(
        lupdate,
        *default_lupdate_args,
        *lupdate_args.split(),
        src_dir,
        "-ts", ts_file,
    )

    """ Haven't looked what this post-processing is doing, but it's bringing mild complications. Let's try without.
    # Postprocessing
    print("\nPostprocessing:")
    postprocess = "MuseScore/tools/translations/process_source_ts_files.py"
    postprocess_launcher = os.getenv("POSTPROCESS_LAUNCHER", sys.executable)
    postprocess_args = os.getenv("POSTPROCESS_ARGS", "")

    print("Running", postprocess_launcher, postprocess, postprocess_args)
    run_indented(
        postprocess_launcher,
        postprocess,
        *postprocess_args.split(),
    )
    """


if __name__ == "__main__":
    main()
