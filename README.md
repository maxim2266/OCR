# OCR tools
An ever growing collection of tools to perform [OCR](https://en.wikipedia.org/wiki/Optical_character_recognition).

### Motivation

Doing a good quality OCR in one go is hard. Usually the process includes a number of iterative steps to improve the original
image quality in order to achieve reasonable recognition, followed by some manual correction of the output text.
This is not a problem when digitising a page or two, but processing a book of 500 pages makes
things a lot harder. Depending on the hardware, the OCR stage itself may take tens of minutes to complete, and the
other image processing stages may take considerable time as well. The only solution seems to be splitting the process
into a number of steps where each step can be run (and rerun) independently until a reasonable quality of the output
is achieved. This toolset is aimed to support that kind of incremental approach.

### OCR Process

In general, the process of converting a document to text includes the following steps:

``` image-extraction -> image-processing -> OCR -> text-postprocessing```

The image extraction and OCR steps are relatively easy to automate, and this toolset provides two simple
scripts to do just that. The image processing step depends heavily on the input image quality and may involve
a number of different tools. This toolset provides a few scripts that may be useful for building an image processing pipeline.
Text post-processing can often be done using the good old Unix `sed` command, at least for the English language,
other languages usually require a more advanced regular expression engine with full Unicode support (think `perl`).

### Tools
#### Image extractor

```
▶ ./extract-images
Usage: extract-images [OPTION]... FILE
Renders pages of a .pdf or .djvu FILE to grayscale images in PGM format.
Options:
  -f N     first page number (optional, default: 1)
  -l N     last page number (optional, default: last page of the document)
  -o DIR   output directory (optional, default: .)
  -h       display this help and exit
```

The output is a set of grey-scale images in `.pgm` format, one per page.

#### OCR on a single image
```
▶ ./extract-text
Usage:	extract-text FILE [OPTION]...
  Run OCR on the given image FILE. Recognised text is written to STDOUT.
  All the given options are passed down to the "tesseract" tool.
```

The main purpose of the script is to validate command line parameters before passing
them down to `tesseract`, because error messages from `tesseract` are rather cryptic.

#### OCR
```
▶ ./ocr
Usage:	ocr DIR [OPTION]...
  Run OCR on all .pgm files in the given directory DIR.
  Recognised text is written to STDOUT. All the given
  options are passed down to the "tesseract" tool.
```

#### Image cropping
```
▶ ./crop-image
Usage: crop-image [OPTIONS]... FILE

Crop the borders of the image FILE.
Options:
  -{l,r,t,b} N  crop N% of the image from the specified edge; l,r,t,b stand for
                "left", "right", "top", and "bottom" respectively; valid range
                of values is from 0% to 99.99%
  -o FILE       write output to the FILE (optional, default: STDOUT)
```
This crops image with `pamcut`, with crop values given in percents, not in pixels.

#### Image normaliser
```
▶ ./norm-image
Crop image to content, and then add white border 5% thick.

Usage:
	norm-image INPUT-FILE OUTPUT-FILE
```

##### Platform: Linux

Tested on Linux Mint 19.2, will probably work on other Debian-based distributions as well.
