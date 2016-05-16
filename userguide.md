# Fenced Filter User Guide

- [Introduction](#introduction)
- [How FencedFilter Works](#how_fencedfilter_works)
- [Scanning for Tag Matches](#scanning_for_tag_matches)
  - [Non-Eligible Characters in Tags](#non_eligible_characters_in_tags)
  - [Hyphenated Tags](#hyphenated_tags)
  - [Comment Tags](#comment_tags)
- [Highlighting Files](#highlighting_files)
  - [Enclosing the Match](#enclosing_the_match)
- [Prepare Doxygen to Use FencedFilter](#prepare_doxygen_to_use_fencedfilter)
- [Off-label Uses](#off_label_uses)
  - [Example 1: Highlight a Name](#example_1_highlight_a_name)
  - [Example 2: Highlight Elements, Data from the Internet](#example_2_highlight_elements_data_from_the_internet)
  - [Example 3: Highlight CSS, More Internet Data](#example_3_highlight_css_more_internet_data)


## Introduction

FencedFilter is a Doxygen filter for keyword highlighting fenced code blocks.
It is meant to supplement the Doxygen syntax highlighting for languages that
Doxygen doesn't support.

## How FencedFilter Works

FencedFilter scans text files, looking for fenced code blocks to process.
Text that is not a fenced code block is written out exactly as read, while
the fenced code is replaced with HTML code where a matching highlighting file
can be found.  (Fenced code blocks without a matching highlighting file is
written out exactly as read in order to leave it for Doxygen to process.)

Highlighting files are files that associate words with HTML elements that
are to be used to present the matched words.

FencedFilter associates the highlighting file with a fenced code block through
the `info string` part of the opening code fence.  For example:

~~~~~text
~~~{bash}
if [ -f ~/Documents/bankacct.xls ]; then
   notify ~/Documents/bankacct.xls
fi
~~~
~~~~~

In this example, FencedFilter will look for a file named `bash.hl`.  If this
file is found, its contents will be used to advise FencedFilter what text
should be highlighted, and how.

## Scanning for Tag Matches

In FencedFilter, a word is a string of one or more word-eligible characters.
The word eligible characters are;

- Upper- and lower-case latin characters (a-z,A-Z)
- Numerals (0-9)
- Underscore ('_')
- Hyphen ('-') (if hyphenated-tags are enabled)

FencedFilter will scan the source file character-by-character, mostly ignoring
non-eligible characters by sending them to _stdout_ as is, until it encounters a
word-eligible-character.  The text, starting with that first eligible character,
will be compared against the list of tags in the highlighting file.  If a match
is found, it will be processed.  If no match is found, the characters of the word
will be sent to _stdout_ without translation until the next ineligible character.
For both matched and unmatched words, scanning will proceed with the non-eligible
character the follows the word in the source file.  See [Comment Tags](#comment-tags)
for further discussion of scanning issues.

A match will be made when a string of characters in the source file,
terminated before and after with a non-eligible character or the start
or end of the string.  Thus, the `do` in `redo` or `document` will
not be matched with a `do` tag.

### Non-Eligible Characters in Tags

A tag __can__ include non-eligible characters, and a match will be made
when a matching string, including the non-eligible characters, are
found in sequence.  The non-eligiible character terminates the string
in the source file after all characters in the tag are matched.

A space is a non-eligible character: a space obviously marks the end of
the previous word.  However, a tag like `john doe` would match _john doe_
in the text, but not _john doerr_.  because the 'r' indicates that the
match is not complete.

### Hyphenated Tags

In most programming languages, the hyphen is a subtraction operator and
is therefore a non-eligible character, ie not allowed in names.  Consider:

~~~c
int my     = 100;
int hat    = 50;
int my-hat = 10;
int result = my-hat;  // does result = 50 or 10?
~~~

However, some languages do not perform mathematical operations, and for
them, hypenated names can be allowed.  CSS is an example where many
keywords include hyphens without causing confusion.

Hyphenated tags are allowed with the `!ht` flag is set in the highlighting
file.  The `!ht` flag must appear before any rule lines.

Word matches will be made the same way as for non-hyphenated-tags languages,
by matching the entire contents of the tag against a string in the source
file.  The difference that the `!ht` makes is in how the end of the tag
is detected after the match is made.

Consider CSS again.  If hyphenated tags are not enabled, the tag `background`
will match the first part of `background-attachment`, `background-color`,
`background-image`, etc, and leave the second part of those strings
unhighlighted.  When hyphenated tags are enabled, `background` will not
match any of the background-prefixed tags.

### Comment Tags

__NOTE:__ In this version of FencedFilter, only line comments are allowed,
that is, a comment, once started, ends at the end of the line.  Block comments
are not supported within fenced code blocks because they introduce an extra
level of difficulty.  Compilers typically do not allow nested block comments,
so for most uses, developers wouldn't be able to put a block comment in
Doxygen documentation anyway.  I have elected to workaround this limitation
for the time being.  If this program becomes popular and enough users want them,
I may include embedded block support in the future.

Comments are usually started with a non-eligible character.  As I hope I made
clear earlier, non-word-eligible characters are allowed in tags.  Like other
tags, the comment tag will be compared against text in the source file, but
unlike word tags, FencedFilter will enclose the remainder of the line in an
HTML element according to the rule of the rule line
(see [Enclosing the Match](#enclosing-the-match)).

Tags that begin with a non-eligible character are kept in a list separate from
the word tags because every non-eligible character in the source file must be
checked against the list.  This is different from comparing words, where the
search commences only with the first eligible character.

__NOTE:__ The '#' character marks comments in a highlighting file.  If a '#'
is also a comment in the language supported by the highlighting file, it must
be escaped with a '\' (backslash).

~~~
# BASH highlighting
comments : span.comment
   \#    # escaping the '#'
~~~   
Likewise, some comments may require a trailing space, like the double-dash comment
for SQL.  For the double-dash SQL comment, escape the trailing space with a '\',
making sure a space follows the backslash and not a newline.

~~~
# SQL highlighting
comments : span.comment
   --\   # escaping the space
~~~

## Highlighting Files

Highlighting files are used to advise FencedFilter what text should be
highlighted and how.  The following is a fragment of the `bash.hl` file:

~~~hl
# This is a comment.  Text following a '#' will be ignored, unless it's escaped.
comment : span.comment   # This is a rule line,
                         # `comment` is the tag, `span.comment` is the rule.
   \#                    # This is a tag line.  Note the # is escaped with a '\' (backslash).
keyword : span.keyword   # This rule line is the parent of the tags that follow.
   case
   do
   done
   elif
~~~

### Enclosing the Match

FencedFilter will, within matched fenced code blocks, enclose text that
matches tags with HTML elements according to the _rule_ in the
rule line.  The HTML element name will match the text before the period
in the rule, and the class designation will be the text following the
period in the rule.  For example, while using the _bash_  highlighting
file, FencedFilter will replace instances of `do`  with

~~~html
<span class="keyword">do</span>
~~~
because `do` is associated with `span.keyword` in the bash.hl file.

## Prepare Doxygen to Use FencedFilter

FencedFilter is a prescan filter for Doxygen.  It is Doxygen will use the
filter if the `INPUT_FILTER` line in Doxyfile is set to the path to the
FencedFilter executable.

If, as recommended, the executable and the highlighting files are installed
in the same directory as Doxygen will run, the Doxyfile line will look like:
~~~txt
INPUT_FILTER           = ./fencedfilter
~~~

If FencedFilter is installed elsewhere, it would also work to use symbolic
links to FencedFilter and its provided highlighting files

## Off-label Uses

FencedFilter is primarily intended to provide some language keyword
highlighting for languages that are not otherwise supported in Doxygen.
However, the method use to find and highlight tags could be extended to
other types of text.  All that is necessary is to create the highlighting
file and to create a fenced code block referring to the highlighting file.

However, the fenced code blocks can be used for other kinds of data,
as well, producing what might be called _text blocks_ instead of _code blocks_.

Look at the following examples for some ideas.

### Example 1: Highlight a Name

One interesting, trivial? example would be to highlight a name in a
text block.

A fenced code block that looks like this,

~~~~~txt
~~~johndoe
My name is John Doe, not John Doerr.
Please pay attention to spelling!
~~~
~~~~~

referring via the _johndoe_ info string to file johndoe.hl,

~~~hl
!ci              # make case-insensitive matches

name : span.keywordflow
   john\ doe     # included spaces must be escaped
~~~

will result in this highlighted 'code' block:

~~~johndoe
My name is John Doe, not John Doerr.
Please pay attention to spelling!
~~~

### Example 2: Highlight Elements: Data from the Internet

FencedFilter can make it easier to highlight a long list of terms
in a text block.  If a text block has many element names, it might
be useful to highlight those names.

~~~elements
Superman, we've got your oxygen tanks ready to go.  The
shipment of gold ingots has arrived, but I'm not strong enough
to carry them in, but it's also not safe for you to be in that
room without protection: I'm sure those villans plan to expose
you to the deadly kryptonite in their krypton lights.
~~~

A highlighting file with the element names would make that possible,
but it's likely that you will have to make your own.  Included with
this project are two examples of pulling information from the internet
to make a highlighting file.  Look in the `hlfilework` directory
for `css_make_hl` and `elements_make_hl`.

### Example 3: Highlight CSS: More Internet Data

Making highlighting files can be tedious, especially for something
like CSS with its bounty of properties.  There are two examples
of harvesting CSS properties in the `hlfilework` directory,
`css_make_hl` and `css3_make_hl`.  Each solves different problems:

- __css_make_hl__ includes both CSS property names and CSS value
  names.  The css_step_two.xsl makes two lists, one of the properties,
  and the other of the values.  The values list includes only the
  first of possible duplicate values.
  
  The problem with css_make_hl is that it only includes CSS1 and CSS2
  properties.  Here's a small example.  Note that both property and value
  is colored for CSS1 and CSS2, but the CSS3 property `align-content` is
  not recognized.

~~~css
div.demo
{
   display       : inline-block;
   overflow      : scroll;
   align-content : center;
}
~~~

- __css3_make_hl__ includes CSS1, CSS2, and CSS3 properties, but
  not the property values.  The unique, possibly useful, feature
  of this highlighting file is that the properties are grouped
  by CSS level, using a different style for each level to help
  identify the level by color.

  Here's the same example as above, using the css3 highlighting file.
  Note that each property is a different color according to its CSS
  level, but the values are all black: they are not recognized.

~~~css3
div.demo
{
   display       : inline-block;
   overflow      : scroll;
   align-content : center;
}
~~~

  It is possible to improve `css3_make_hl` to include the allowed
  values strings, but that would require opening the property web
  page for those properties that are provided with one.  The property
  pages include allowed values, which could be harvested.

  To harvest the values would require four additional steps:
  -1 `css3_step_one.xsl` would have to modified include the URL of the
     associated property page.
  -1 A `css3_step_two.xsl,` instead of producing the highlighting
     file, would now generate a BASH script that would open each
     property page, harvesting the properties and adding them
     to a new XML file with an unclosed root element, with the final
     step of the BASH file being to close the root element of the
     properties XML file.
  -1 The generated BASH file would be run to create the values XML file.
  -1 A `css3_step_three.xsl` would combine the contents of the
     properties XML file and the values XML file to create a highlighting
     file with both properties and values, supporting CSS level 3.
   





