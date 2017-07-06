# OCR tools
An ever growing collection of tools to perform [OCR](https://en.wikipedia.org/wiki/Optical_character_recognition).

### Motivation

There is no good tool that could handle the entire
process of character recognition end to end in one go, without various adjustments and parameter tuning
in between different stages. Depending on the hardware, the OCR stage itself may take tens of minutes to process
a large document (for example, a book of 500 pages), and so may take other image extraction/processing stages.
Rerunning the whole process just to see if one parameter change can actually improve the end result is both time
consuming and boring. The toolset is designed to support running OCR in separate, mostly independent steps
so that each step can be rerun with a different set of parameters, and the steps can be invoked in
a different order. All that may be unnecessary to recognise just a few pages, but for a book the automation becomes
crucial as a few days spent on adjusting the input images or regular expressions for postprocessing
can easily save weeks of manual correction of the text.

### OCR Process

In general, the process of converting a document to text includes the following steps:

``` image-extraction -> image-processing -> OCR -> text-postprocessing```

This toolset targets processing of large documents and is designed to automate each step separately
storing intermediate results in a directory. The separation is especially useful
for image processing step where images get resized, cropped and optimised in a variety of ways, as well as for
the text post-processing where a good number of regular expressions are applied to remove OCR artefacts and
correct errors.

### Tools
#### Image extractor (renderer)

```
Usage: render-document [OPTION]... FILE
Renders .pdf or .djvu document FILE to grayscale images in .pgm format.
Options:
  -f N     first page number (optional, default: 1)
  -l N     last page number (optional, default: last page of the document)
  -o DIR   output directory (optional, default: .)
  -h       display this help and exit
  -v       output version information and exit
```

The output is a set of grey-scale images, one per page, with 600dpi resolution. The input document
does not have to be made of images, any valid document can be rendered by this tool.

#### OCR
```
Usage: ocr [OPTION]... DIR
Extracts text from all image files in DIR.
Options:
  -L LANG  document language (optional, default: 'eng')
  -o FILE  output file name (optional, default: stdout)
  -h       display this help and exit
  -v       output version information and exit
```

A wrapper around `tesseract` tool.


##### Platform: Linux

Tested on Linux Mint 18.1, will probably work on other Debian-based distributions.

#### External tools

Internally the toolset relies on `pdfimages` and `ddjvu` tools for extracting images,
and on `tesseract` program for the actual OCR'ing. The tool `pdfimages` is usually a part of `poppler-utils`
package, the tool `ddjvu` comes from `djvulibre-bin` package, and `tesseract` is included in `tesseract-ocr`
package. By default, `tesseract` comes with the English language support only, other languages should
be installed separately, for example, run `sudo apt install tesseract-ocr-rus` to install the Russian
language support. To find out what languages are currently installed type `tesseract --list-langs`.


