#!/usr/bin/env python3
# Very minimal man 2 html converter. Only supports subset used by me.

# Start of line:
# .TH   title section [extra1] [extra2] [extra3]
# .SH   section header
# .SS   secondary section header
# .br   Break--stop filling text
# .fi   Fill text
# .nf   Turn off filling
# .B    bold (same or next line)
# .BI   alternately bold and italic
# .BR   alternately bold and romanic
# .PP   paragraph, I guess
# .TP   indent/reset indent when no parameter
# .sp n leave n blank lines

# Inline:
# \fx   One letter font x
# \f(xx Two letter font xx
# \"    comment to line end

# Fonts:
# B 	Bold
# I 	Italic
# R 	Roman
# P 	Previous
# H 	Helvetica
# CW 	Constant Width
# HB 	Helvetica Bold
# HI 	Helvetica Italic

# Conversions:
# \(dq  "
# \(rs  \
# \(cq  '
# \.    .
# \-    -
# \~    (unbreakable space)

from html import escape as escape_html, unescape as unescape_html

import re

LINE_CMD = re.compile(r"^\.([a-zA-Z]+)\s*(.*)")
INLINE = re.compile(r'\\f(.)|\\f\((..)|\\([-~\.])|\\\((..)|"([^"]*)"|(https?://[-a-zA-Z?=\.&/:]+)|(\s+)|\\"(.*)')
WORD = re.compile(r'^ *(\w+) *$')
SECTION = re.compile(r'^ *\((\d+\w?)\),? *$')
LINK = re.compile(r'^ *(\w+) *\((\d+\w?)\)[-.,!?:]? *$')
TAG = re.compile(r'<[^<>]*>')

FONT_TO_BEGIN = {
	"B": "<strong>",
	"I": "<em>",
	"R": '<span class="roman">',
	"H": '<span class="helvetica">',
	"CW": '<code>',
	"HB": '<strong class="helvetica">',
	"HI": '<em class="helvetica">',
}

FONT_TO_END = {
	"B": "</strong>",
	"I": "</em>",
	"R": '</span>',
	"H": '</span>',
	"CW": '</code>',
	"HB": '</strong>',
	"HI": '</em>',
}

def strip_html(html):
	return unescape_html(TAG.sub('', html))

def do_fmt(font, buf, fmt_stack):
	if fmt_stack:
		old = fmt_stack[-1]
		buf.append(FONT_TO_END[old])
	
	if font == "P":
		fmt_stack.pop()
		if fmt_stack:
			old = fmt_stack[-1]
			buf.append(FONT_TO_BEGIN[old])
	else:
		buf.append(FONT_TO_BEGIN[font])
		fmt_stack.append(font)

class Context:
	__slots__ = 'lines', 'url', 'buf', 'title', 'section', 'extras', 'keep_spaces', 'was_empty', 'next_is_h4'

	def __init__(self, lines, url):
		self.lines = lines
		self.url = url
		self.buf = []
		self.title = ""
		self.section = ""
		self.extras = []
		self.keep_spaces = False
		self.was_empty = True
		self.next_is_h4 = False
	
	def parse(self):
		for line in self.lines:
			line = line.rstrip('\n')
			if not line:
				# TODO: better paragraphe code using <p>
				if not self.was_empty:
					self.buf.append('<br/>')
					self.was_empty = True
			else:
				self.was_empty = False
				m = LINE_CMD.match(line)
				if m:
					cmd = m.group(1)
					args = m.group(2)
					if cmd == 'nf':
						if not self.keep_spaces:
							self.buf.append('<pre>')
							self.keep_spaces = True
						self.next_is_h4 = False
					elif cmd == 'fi' or cmd == 'br': # not sure about br
						if self.keep_spaces:
							self.buf.append('</pre>')
							self.keep_spaces = False
						self.next_is_h4 = False
					elif cmd == 'sp':
						if args:
							n = int(args)
						else:
							n = 1
						for _ in range(n):
							self.buf.append('<br/>')
						self.next_is_h4 = False
					else:
						if self.keep_spaces:
							delim = ''
						else:
							delim = ' '
						args = self.parse_text(args, self.keep_spaces)

						if cmd == 'TH':
							if self.keep_spaces:
								self.buf.append('</pre>')
								self.keep_spaces = False

							self.title   = args[0]
							self.section = args[1]
							self.extras  = args[2:]
							self.buf.append('<h1>')
							self.buf.append(escape_html(self.title))
							self.buf.append('</h1>')
							self.next_is_h4 = False
						elif cmd == 'SH':
							if self.keep_spaces:
								self.buf.append('</pre>')
								self.keep_spaces = False

							self.buf.append('<h2>')
							self.buf.append(delim.join(args))
							self.buf.append('</h2>')
							self.next_is_h4 = False
						elif cmd == 'SS':
							if self.keep_spaces:
								self.buf.append('</pre>')
								self.keep_spaces = False

							self.buf.append('<h3>')
							self.buf.append(delim.join(args))
							self.buf.append('</h3>')
							self.next_is_h4 = False
						elif cmd == 'B':
							if self.next_is_h4:
								self.next_is_h4 = False

								if self.keep_spaces:
									self.buf.append('</pre>')
									self.keep_spaces = False

								self.buf.append('<h4>')
								self.buf.append(delim.join(args))
								self.buf.append('</h4>')
							else:
								self.buf.append('<strong>')
								self.buf.append(delim.join(args))
								self.buf.append('</strong>')
						elif cmd == 'BI':
							if self.next_is_h4:
								self.next_is_h4 = False

								if self.keep_spaces:
									self.buf.append('</pre>')
									self.keep_spaces = False

								self.buf.append('<h4>')
								self.alternate(args, None, 'I', '')
								self.buf.append('</h4>')
							else:
								self.alternate(args, 'B', 'I', '' if self.keep_spaces else ' ')
						elif cmd == 'BR':
							if self.next_is_h4:
								self.next_is_h4 = False

								if self.keep_spaces:
									self.buf.append('</pre>')
									self.keep_spaces = False

								self.buf.append('<h4>')
								self.alternate(args, None, 'R', '')
								self.buf.append('</h4>')
							else:
								for i in range(0, len(args), 2):
									pair = args[i:i+2]
									if len(pair) > 1:
										first, second = pair
										m1 = WORD.match(strip_html(first))
										m2 = SECTION.match(strip_html(second))
										is_link = m1 and m2
										if is_link:
											self.buf.append('<a href="%s">' % escape_html(self.url.format(
												title=m1.group(1),
												section=m2.group(1)
											)))
										self.buf.append(FONT_TO_BEGIN['B'])
										self.buf.append(first)
										self.buf.append(FONT_TO_END['B'])
										self.buf.append(FONT_TO_BEGIN['R'])
										self.buf.append(second)
										self.buf.append(FONT_TO_END['R'])
										if is_link:
											self.buf.append('</a>')
										
									else:
										self.buf.append(FONT_TO_BEGIN['B'])
										self.buf.append(pair[0])
										self.buf.append(FONT_TO_END['B'])
						elif cmd == 'PP':
							# TODO: better paragraphe code using <p>
							self.buf.append('<br/>')
						elif cmd == 'TP':
							self.next_is_h4 = True
						else:
							raise ValueError("command not supported: .%s" % cmd)

					# TODO: line command
				else:
					words = self.parse_text(line, self.keep_spaces)
					if self.keep_spaces:
						delim = ''
					else:
						delim = ' '
						for i, word in enumerate(words):
							m = LINK.match(strip_html(word))
							if m:
								words[i] = '<a href="%s">%s</a>' % (
									escape_html(self.url.format(
										title=m.group(1),
										section=m.group(2)
								)), word)

					self.buf.append(delim.join(words))
					self.next_is_h4 = False
			self.buf.append("\n")
	
	def alternate(self, args, font1, font2, delim):
		for arg in args:
			self.buf.append(FONT_TO_BEGIN.get(font1, ''))
			self.buf.append(arg)
			self.buf.append(FONT_TO_END.get(font1, ''))
			self.buf.append(delim)
			font1, font2 = font2, font1

	def parse_text(self, text, keep_spaces=False):
		if not keep_spaces:
			text = text.strip()
		buf = []
		elems = []
		fmt_stack = []
		i = 0
		while i < len(text):
			m = INLINE.search(text, i)
			if not m:
				buf.append(escape_html(text[i:]))
				break
			
			if i != m.start():
				buf.append(escape_html(text[i:m.start()]))

			if m.group(1):
				font = m.group(1)
				do_fmt(font, buf, fmt_stack)
			elif m.group(2):
				font = m.group(2)
				do_fmt(font, buf, fmt_stack)
			elif m.group(3):
				ch = m.group(3)
				if ch == '-':
					buf.append('&mdash;')
				elif ch == '.':
					buf.append('.')
				elif ch == '~':
					buf.append('&nbsp;')
			elif m.group(4):
				ch = m.group(4)
				if ch == "dq":
					buf.append('&quot;')
				elif ch == "rs":
					buf.append('\\')
				elif ch == "cq":
					buf.append("'")
				else:
					raise ValueError("not supported: \\(%s" % ch)
			elif m.group(5):
				s = m.group(5)
				buf.append(escape_html(s))
			elif m.group(6):
				url = m.group(6)
				esc_url = escape_html(url)
				buf.append('<a href="%s">%s</a>' % (esc_url, esc_url))
			elif m.group(7): # spaces
				if keep_spaces:
					buf.append(m.group(7))
				elems.append(''.join(buf))
				buf = []
			elif m.group(8): # comment
				pass
			
			i = m.end()
		
		if buf:
			elems.append(''.join(buf))
		
		return elems

	def __str__(self):
		return """<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8"/>
<title>{title} &mdash; {section}</title>
<style type="text/css">
.roman {{
	font-family: serif;
}}
.helvetica {{
	font-family: sans-serif;
}}
h3 em,
h3 .roman,
h4 em,
h4 .roman {{
	font-weight: normal;
}}
</style>
</head>
<body>
{body}
</body>
</html>
""".format(title=self.title, section=self.section, body=''.join(self.buf))

def man2html(instream, outstream, url="https://linux.die.net/man/{section}/{title}"):
	ctx = Context(instream, url)
	try:
		ctx.parse()
	except:
		print(''.join(ctx.buf))
		raise
	outstream.write(str(ctx))

if __name__ == '__main__':
	import sys
	with open(sys.argv[1]) as instream, open(sys.argv[2], 'w') as outstream:
		man2html(instream, outstream, *sys.argv[3:4])