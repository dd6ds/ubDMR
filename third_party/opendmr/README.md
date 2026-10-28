# AMBE+2 codec (from OpenDMR)

The `decoder/` and `encoder/` directories are copied unchanged from
[OpenDMR](https://github.com/MW0MWZ/OpenDMR) v1.0.0 (commit `d28164b`):

- `decoder/`: mbelib-neo (AMBE 3600x2450 decoding), GPL-2.0-or-later.
- `encoder/`: OP25's MBEEncoder and IMBE analysis, GPL-3.0-or-later
  (`mbeenc.*`) and GPL.

OpenDMR's own wrapper (`opendmr.cpp`) is not used. Its encoder packs the nine
AMBE+2 parameters into the 49 bits one after another instead of in the
codec's bit order, and passes the gain as a linear factor where the encoder
expects a log2 offset. `src/softwarevocoder.cpp` calls the decoder and the
encoder directly and handles both itself.

Build flags for this code live in `CMakeLists.txt`; compiler warnings are
silenced there because the code is kept identical to upstream.
