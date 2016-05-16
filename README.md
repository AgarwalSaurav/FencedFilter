# FencedFilter

## Introduction

This program works with Doxygen to processing fenced code blocks to highlight
keywords.   It is not a complete solution for syntax-highlighting code because
it makes no attempt to interpret the code, but it can provide a useful subset
of that utility in many situations.  It will also process fenced-code blocks in
Markdown files.

The primary purpose is to read user-created vocabularies of keywords in order to
highlight the keywords in a fenced-code block with Doxygen.

This project has been developed in a Linux environment, so there will be
some compatibility issues, especially when running the utilities in the
`hlfilework` directory where BASH scripts, and the `get`, `xmllint`, and
`xsltproc` programs are used to prepare highlighting files from online
sources.

There are several areas that could be improved in the project, but it is really
a distraction from a more important project I'm working on.  In other words, I
have already spent too much time on it relative to its usefulness.  In the event
that the program becomes popular, I would probably love the excuse to continue
working on it.

## Compatibility

This project was developed in a Linux environment.  While the C/C++ code should be
portable to any environment, the build process is probably not.  Primarily, the
makefile makes no Windows accommodations.  There are also a few example scripts
that use Internet sources to build keyword lists that won't run on a vanilla
Windows environment, using BASH, `wget`, `xmllint`, and `xsltproc`.  These should
still serve as helpful examples that a minimally-competent Windows programmer
could modify using the Windows equivalent command line programs.  Being a Doxygen
add-on, programmers are the expected audience, anyway.

## Building the Project

### Downloading

This is the first time I have made a project public on GitHub, so the instructions
around downloading the project will be vague until I figure this part out.  It
should be familiar to people who have cloned/downloaded projects from GitHub.

### Building

Use `cd` to enter the directory, then run `make`.  That's it.

See the [Compatibility](#compatibility) section above for why this may not work
in non-Linux environments.

As I write this README, it occurs to me that I could also have the makefile run
the utilities in `hlfilework`.  I will get around to this, but I want to complete
this README first.

### Installing

There is no `install` command in the makefile.  For now, I am expecting that the
`fencedfilter` will reside in a project directory to be run by Doxygen.  The main
reason for this that the program looks for the highlighting files in the working
directory.

## Using FencedFilter

There is a [User Guide](userguide.md) that will explain how to use the program.

## Other Comments

### The Code

I chose to code the program in C/C++ rather than in Python or Java because it
is started for each source code file in a project.  I figured that it would save
time to avoid having to reload the language runtimes for every file.  Besides,
I prefer C/C++ coding.

The main program, fencedfilter.cpp, could stand refactoring.  I started this
with the simple goal of properly highlighting comment lines for MySQL.  As it
grew to include keyword highlighting, I created classes to handle the highlighting
files, but the main file is a bunch of functions, not organized into an object.

### The Code Documentation

As you might expect, seeing that this project works with Doxygen, the code
is documented in that style.  You can run `doxygen` to generate the usual
HTML-based documentation.

Because some examples use highlighting files, particularly the
[User Guide](userguide.md), it is recommended that the project be built before
running Doxygen, or else the `fencedfilter` program will not be available when
Doxygen looks for it.

### Limitations

Perhaps this section should be a main header, but here you go.

- The program only attempts to process fenced code blocks.  @code ~ @endcode or
  backtick-enclosed text will not be processed.

- Recognizing code fences is pretty unsophisticated.  I looked at
  [CommonMark fenced code blocks](http://spec.commonmark.org/0.25/#fenced-code-blocks)
  for the rules and decided against trying to support even a subset of the
  conditions.  I feel like it's better to just support the simplest format than
  to have to explain why some formatting styles work and others don't.

- It does not do syntax-checking.  It assumes that the code presented is correct.
  That means, among other things, that it will highlight invalid names if they
  are in the highlighting file.

- It does not handle block comments.  For the C-like languages I use, source file with
  nested block comments will not compile.  For what I want to do with this utility,
  I can easily use line comments.

- It does not highlight strings.  I don't really need it yet, so in the interest of
  returning to my main project, I'll leave this for a future version.

### Future Development

As I have said above, interest in the program, if there is any,  will influence
future development.  It works as I need it to for now, though I am itching to do
mode.  These are some things I would like to do:

-# I'd like to make it a proper Markdown processor to generate HTML.  It doesn't
   seem like that would be too hard, but I can't count the times I have thought
   that, only to spend weeks or more working on non-essential programs.

-# I should refactor the main page to be a better example of C++ coding.

-# As it stands, the program runs for every file that Doxygen scans, rereading
   the highlighting files each time.  I have considered making the program into
   a service that a very small program would access with sockets.  That way,
   the highlighting files would be read once the first time requested, then
   reused during the running of Doxygen.  This would only benefit larger projects,
   and perhaps not even measurably, but the idea appeals to me.

-# I alphabetize the keywords when I parse the highlighting files, but then
   do a sequential search through the words.  There is room for optimization
   by keeping a count of keywords and then doing a tree-like search by comparing
   comparing values at several indexes of the keyword array.


