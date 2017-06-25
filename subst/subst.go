/*
BSD 3-Clause License

Copyright (c) 2017, Maxim Konakov
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package main

import (
	"bufio"
	"fmt"
	"io/ioutil"
	"os"
	"regexp"
	"strconv"
	"strings"
	"text/scanner"
	"unicode"
)

func main() {
	// arguments
	if len(os.Args) != 2 {
		fmt.Fprintf(os.Stderr, "Usage: %s replacement-rules-file\n", os.Args[0])
		os.Exit(1)
	}

	// read substitution rules
	lsub, tsub := readRules(os.Args[1])

	if len(lsub)+len(tsub) == 0 {
		die("Empty substitution specification")
	}

	// line processing
	var text string

	if len(lsub) > 0 {
		var lines []string

		reader := bufio.NewScanner(os.Stdin)

		for reader.Scan() {
			lines = append(lines, subst(strings.TrimRightFunc(reader.Text(), unicode.IsSpace), lsub))
		}

		if err := reader.Err(); err != nil {
			die(err.Error())
		}

		text = strings.Join(lines, "\n")
	} else {
		s, err := ioutil.ReadAll(os.Stdin)

		if err != nil {
			die(err.Error())
		}

		text = string(s)
	}

	// full text processing
	if len(tsub) > 0 {
		text = subst(text, tsub)
	}

	if len(text) > 0 {
		if _, err := os.Stdout.WriteString(text); err != nil {
			die(err.Error())
		}
	}
}

// substitution function
func subst(s string, fns []func(string) string) string {
	if len(s) > 0 {
		for _, fn := range fns {
			if s = fn(s); len(s) == 0 {
				break
			}
		}
	}

	return s
}

// script file grammar:
// ['line' | 'text'] string 'with' string

func readRules(fname string) (line, text []func(string) string) {
	p := newParser(fname)

	defer p.close()

	for isLine, fn := p.next(); fn != nil; isLine, fn = p.next() {
		if isLine {
			line = append(line, fn)
		} else {
			text = append(text, fn)
		}
	}

	return
}

// parser
type parser struct {
	tz   tokeniser
	file *os.File
}

func newParser(fname string) (p *parser) {
	file, err := os.Open(fname)

	if err != nil {
		die(err.Error())
	}

	p = &parser{file: file}
	initTokeniser(&p.tz, file)
	return
}

func (p *parser) close() error {
	return p.file.Close()
}

// rule reader
func (p *parser) next() (isLine bool, fn func(string) string) {
	// 'line' or 'text'
	switch scope := p.nextKeywordOrEOF(); scope {
	case "": // EOF
		return
	case "line":
		isLine = true
	case "text":
		// isLine = false
	default:
		p.tz.reject(scanner.Ident, scope)
	}

	// pattern
	patt := p.nextString()

	if len(patt) == 0 {
		p.tz.fail("Empty string as pattern")
	}

	re, err := regexp.Compile(patt)

	if err != nil {
		p.tz.fail(err.Error())
	}

	// 'with'
	p.matchKeyword("with")

	// substitution
	subst := p.nextString()

	// result
	fn = func(s string) string { return re.ReplaceAllString(s, subst) }
	return
}

// parser support functions
func (p *parser) nextString() string {
	tag, tok := p.tz.next()

	if tag != scanner.String {
		p.tz.reject(tag, tok)
	}

	return tok
}

func (p *parser) nextKeywordOrEOF() string {
	tag, tok := p.tz.next()

	switch tag {
	case scanner.Ident:
		// fine
	case scanner.EOF:
		tok = ""
	default:
		p.tz.reject(tag, tok)
	}

	return tok
}

func (p *parser) matchKeyword(keyword string) {
	tag, tok := p.tz.next()

	if tag != scanner.Ident || tok != keyword {
		p.tz.reject(tag, tok)
	}
}

// tokeniser
type tokeniser struct {
	sc scanner.Scanner
}

func initTokeniser(tz *tokeniser, in *os.File) {
	(&tz.sc).Init(in)

	tz.sc.Mode = scanner.ScanStrings | scanner.ScanRawStrings | scanner.ScanIdents | scanner.ScanComments | scanner.SkipComments
	tz.sc.Filename = in.Name()
	tz.sc.Error = scError
}

func (tz *tokeniser) next() (tag rune, tok string) {
	switch r := tz.sc.Scan(); r {
	case scanner.Ident:
		return r, tz.sc.TokenText()
	case scanner.String, scanner.RawString:
		s, err := strconv.Unquote(tz.sc.TokenText())

		if err != nil {
			tz.fail(err.Error())
		}

		return scanner.String, s
	case scanner.EOF:
		return r, ""
	default:
		die(fmt.Sprintf("Unexpected token: %q", tz.sc.TokenText()))
		return r, "" // unreachable
	}
}

func (tz *tokeniser) fail(msg string) {
	scError(&tz.sc, msg)
}

func (tz *tokeniser) reject(tag rune, tok string) {
	switch tag {
	case scanner.Ident:
		tz.fail(fmt.Sprintf("Unexpected keyword: %q", tok))
	case scanner.String:
		tz.fail(fmt.Sprintf("Unexpected string: %q", tok))
	case scanner.EOF:
		tz.fail("Unexpected end of file")
	default:
		panic("Unknown tag")
	}
}

// scanner error exit
func scError(s *scanner.Scanner, msg string) {
	die(fmt.Sprintf("%s:%d - %s", s.Filename, s.Position.Line, msg))
}

// error exit
func die(msg string) {
	os.Stderr.WriteString("ERROR: " + msg + "\n")
	os.Exit(1)
}
