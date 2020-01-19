# OCR tools
An ever growing collection of tools to perform [OCR](https://en.wikipedia.org/wiki/Optical_character_recognition).

### Motivation

Doing a good quality OCR in one go is hard. Usually the process includes a number of iterative steps to improve the original
image quality in order to achieve reasonable recognition, followed by some manual correction of the output text.
This is not a problem when digitising a receipt for tax return, but processing a book of 500 pages makes
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
Simple text post-processing can be done using the good old Unix `sed` command, at least for the English language,
other languages usually require a more advanced regular expression engine with full Unicode support (think `perl`).

### Tools
#### Image extractor (renderer)

```
▶ ./render-document
Usage: render-document [OPTION]... FILE
Renders pages of a .pdf or .djvu document FILE to grayscale images in PGM format.
Options:
  -f N     first page number (optional, default: 1)
  -l N     last page number (optional, default: last page of the document)
  -o DIR   output directory (optional, default: .)
  -h       display this help and exit
```

The output is a set of grey-scale images, one per page, with 600dpi resolution. The input document
does not have to be made of images, any valid document can be rendered by this tool.

#### OCR
```
Usage: ocr [OPTION]... DIR
Extracts text from all image files in DIR.
Options:
  -L LANG  document language (optional, default: 'eng')
  -o FILE  output file name (optional, default output directed to stdout)
  -e EXT   input files extension (optional, default: pgm)
  -h       display this help and exit
  -v       output version information and exit
```

A wrapper around `tesseract` tool.

#### Image geometry calculator for cropping
```
▶ ./crop-geometry
Compose crop geometry string for "convert -crop"

Usage:
	crop-geometry left right top bottom input-file-name
where
	"left", "right", "top", and "bottom" (in this order) are percentages
	of the image to crop from the respective sides of the image. The range of
	values is from 0 to 99.99%, with up to 2 digits after the decimal point.
Example:
	crop-geometry 19.33 2 3 40 input.pgm
```

The tool writes the composed geometry string to `stdout`. Can be used like:
```
convert input_file -crop "$(crop-geometry 10 10 10 10 input-file)" output_file
```

#### Image normaliser
```
▶ ./norm-image
Crop image to content, and then add white border 5% thick.

Usage:
	norm-image input-file output-file
```

##### Platform: Linux

Tested on Linux Mint 19.2, will probably work on other Debian-based distributions as well.
