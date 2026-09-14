# Third-party DSP code

Vendored sources, kept in their upstream formatting (see `.clang-format` here).

- `SimpleCompressor/` — Daniel Rudrich's SimpleCompressor gain-reduction computer
  and look-ahead delay (https://github.com/DanielRudrich/SimpleCompressor),
  GPL-3.0. Taken from Audacity's copy, which only changed the include of the
  fast-log helper.
- `DynamicRangeProcessor/` — Audacity's `CompressorProcessor` (the per-block
  driver around SimpleCompressor: look-ahead delay of the audio, make-up gain,
  frame statistics), its settings types and the `FastLog2` helper,
  GPL-2.0-or-later, from https://github.com/audacity/audacity
  (`libraries/lib-dynamic-range-processor`, `libraries/lib-utility`). Changes:
  include paths, the export macro removed, the lock-free output packet queue
  (Audacity's UI plumbing) dropped from the types header.

Both serve Orchestrion's built-in compressor and limiter (`internal/DynamicsProcessor`).
