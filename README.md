IconExtractor
=============

A simple project for extracting the main icon from a Windows PE.  This project works with 32-bit or 64-bit binaries, and can extract embedded ICOs from an image, or an embedded PNG if one exists.  The caller may specify size constraints of the desired image on the command line.

If an ICO is targeted for extraction, the original color table and mask are combined into a 32bpp ARGB image and stored on disk as a PNG.  32bpp ICOs are dynamically analyzed to determine whether they have alpha channel information, and if it exists, this information is properly honored.  The ICO is also adjusted by its paired transparency bitmask so that pixels reported as transparent by the bitmask are properly rendered as transparent in the PNG.

When the caller has selected an embedded Vista-style PNG for extraction, the PNG is pulled from the executable and written directly to disk.  No recompression, interpretation, or extensive checking is done to verify the integrity of the PNG beyond validation of PNG required headers.
