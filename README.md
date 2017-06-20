# OCR
A simple driver script for "tesseract" OCR tool.

### The tool
Given an input file in either `pdf` or `djvu` format, the tool extracts images from
the input files using `pdfimages` or `ddjvu` tool, and then converts the images to
plain text using `tesseract` tool.

### Invocation
```./ocr [OPTION]... FILE```

Command line options:
```
-f N     first page number (optional, default: 1)
-l N     last page number (optional, default: last page of the document)
-L LANG  document language (optional, default: 'eng')
-o FILE  output file name (optional, default: stdout)
-w DIR   working directory (optional, default: a temporary directory)
-h       display this help and exit
-v       output version information and exit
```

##### Example

The following command processes a document `some.pdf` in Russian, from page 12 to page 26 (inclusive),
storing the result in the file `document.txt`:
```
./ocr -f 12 -l 26 -L rus -o document.txt some.pdf
```

##### Platform: Linux

Tested on Linux Mint 18.1, will probably work on other Debian-based distributions.

#### External tools

Internally the script relies on `pdfimages` and `ddjvu` tools for extracting images,
and on `tesseract` program for the actual OCR'ing. The tool `pdfimages` is usually a part of `poppler-utils`
package, the tool `ddjvu` comes from `djvulibre-bin` package, and `tesseract` is included in `tesseract-ocr`
package. By default, `tesseract` comes with the English language support only, other languages should
be installed separately, for example, run `sudo apt install tesseract-ocr-rus` to install the Russian
language support. To find out what languages are currently installed type `tesseract --list-langs`.

#### Known limitations

The tool may produce somewhat messy output from `.pdf` files composed of images with masks. No simple
workaround is known at this time. Check the input with `pdfinfo` first.
