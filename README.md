# OCR tools
The ever growing collection of tools to perform [OCR](https://en.wikipedia.org/wiki/Optical_character_recognition).

### Motivation

Achieving a good quality OCR in one go is not easy. Depending on the quality of the input,
the process may include a number of iterations to improve the original image(s) in order
to achieve reasonable recognition quality, followed by some (often manual) correction of
the recognised text to remove various OCR errors. This is not a massive problem when
digitising a page or two, but processing a book of 500 pages makes things a lot harder.
This project aims to help with complex OCR projects, but instead of providing one monolithic
tool that would include all the processing a user can possibly want, here we develop a number of
smaller instruments that can do only the obviously needed steps like OCR itself, but also
allowing for user-defined processing to be integrated into the pipeline.

### Tools

The toolset wraps around a number of well-known programs that perform tasks like PDF
or image processing, character recognition, etc., aiming to create an environment for
iterative processing of large documents with the ability to utilise custom scripts.

For example, given a document `text.pdf`, the simplest OCR session may look like
the following:

```sh
▶ mkdir book && cd book
▶ ocr-open ../text.pdf
ocr-open: processing file "../text.pdf"
ocr-open: extracting all pages
▶ ocr
ocr: processing page 1 [ "./page-01.pgm" ]
ocr: processing page 2 [ "./page-02.pgm" ]
{ ... }
ocr: processing page 15 [ "./page-15.pgm" ]
▶ ocr-ls --text | xargs cat
{ ... recognised text }
▶
```
In this simple example we first create a directory and `cd` into it, after that we convert
each page of the document `text.pdf` to an image using `ocr-open` tool, and then we do
the actual character recognition via `ocr` tool. The last command gives an example of
how other custom tools can be integrated into the process with the help of the `ocr-ls`
utility. Here we use the standard Linux `cat` utility to display the recognised text.

Internally, the toolset operates on images in [PGM](http://netpbm.sourceforge.net/doc/pgm.html)
format, that has been chosen as the lowest common denominator between all the tools
wrapped by this toolset, and also because it is understood by the good old `netpbm`
package, which is often a bit faster than `imagemagic` when it comes to simple
operations like image cropping.

All images are named using pattern `page-N.pgm`, where `N` is the page number ranging from
1 to the maximum of 9999, as in the source document, and with a sufficient number
of leading zeroes to make sure that a list of files sorted alphabetically gives the correct
page order. The text recognised from each page is stored in a file named using the same pattern,
but with the `.txt` extension. Most of the tools in this toolset can operate on a sub-range of
pages via `-p` or `--pages` command line option, see help (`-h` or `--help`) on
a particular tool. Generally, the toolset is designed to operate on "pages" rather
than files, for convenience.

Another thing these tools are designed to do is to check all the parameters and input
files before passing them over to the underlying programs, because the error messages
from those programs are sometimes a bit cryptic.

For details on the command line options supported by a particular tool, simply
invoke the tool with `-h` or `--help` option.

The included tools are:

##### `ocr-open`

This is usually the first command to invoke when starting a new project. The tool
converts each page of the specified document to a separate image. There are options
to specify the range of pages to extract, and the destination directory.
Input document can be either in `.pdf` or `.djvu` format. Internally the tool invokes
either `ddjvu` or `pdftoppm` program, depending on the type of the input file.

##### `ocr-ls`

The main purpose of the tool is to produce a list of files for bulk-processing.
The tool outputs a list of files, text or images, from the selected range(s) of pages, in order.
A simple example is given above, where it is used to concatenate all the recognised text.
For a more involved example, consider the situation where every page except the first one
has a page number at the bottom that we don't want to see in the recognised text,
and so we want to crop (for example) 6.5% from the bottom of each image starting from
the page 2, and till the end of the document. This can be achieved with the following
command:
```sh
ocr-ls -p 2- | xargs -I{} -n 1 crop-image -b 6.5% {} {}
```
_(see below for the description of the `crop-image` command)_

##### `ocr`

The tool invokes `tesseract` program to recognise text from the given images. There
are options to specify the range of pages to process, as well as the directory
where the image files are stored. Per each page, the recognised text is written to
the same directory, and to the file with the same name but with a `.txt` extension.
For example, this is how to extract text in Russian and English, from pages
5 to 10 only, all located in the directory `book`:

```sh
ocr -p 5-10 -d book/ -- -l rus+eng
```
Note: everything to the right from `"--"` is passed over to the `tesseract` program.

##### `crop-image`

Crops the specified image. The amount of space to crop is given as the percentage of
the image's width or height, which is often more convenient than using pixels. Wraps
around the `pamcut` utility from `netpbm` toolset.

##### `norm-image`

A tiny utility that crops the image to content and then adds 5% white border. Wraps around
ImageMagic `convert` tool. Rarely useful, except the situations where there are
poor quality scanned images with some dust bits on the space surrounding the text,
that sometimes get recognised as punctuation.

##### `norm-text`

A script to normalise text by removing hyphenation and line breaks inside paragraphs.
Normally, `tesseract` separates paragraphs by empty lines, and this is required
for the tool to work correctly. The tool takes its input from `stdin`, and
writes to `stdout`.

##### `norm-page`

Ensures the correct paragraph boundary at the end of the page. Takes one or more text
files as input, and writes its output to `stdout`. Can be used in conjunction with
other tools, for example:
```sh
ocr-ls -t | xargs norm-page | norm-text
```

### Installation

The toolset makes use of external tools that need to be installed first:
```sh
sudo apt install netpbm imagemagick tesseract-ocr djvulibre-bin poppler-utils
```

Optionally, install language packs for `tesseract`, for example:
```sh
sudo apt install tesseract-ocr-rus
```

The preferred way to install the toolset is to grab the `ocr-*.tar.xz` archive
attached to the latest [release](https://github.com/maxim2266/OCR/releases)
on github (starting from version 0.8), and extract it to a directory listed on the
`$PATH`. Alternatively, if the very recent but yet unreleased updates are required,
just clone the project from github
```sh
git clone --recursive https://github.com/maxim2266/OCR
```
then install dependencies for the build
```sh
sudo apt install build-essential libmagic-dev
```
and finally run `make release` from the root directory of the project. This will compile the
toolset and create an archive with all the utilities, which can then be extracted to a directory
on the `$PATH`.

The toolset has been tested on Linux Mint 19.3, and will probably work on other Debian-based
distributions as well. Supported `tesseract` version is 4.0.0 or later.
