# Standard library provided by the CLI
Here's a very quick reference of the standard library provided with the CLI.

The following classes, objects, methods, functions, are only available when running the CLI. They aren't part of the core language.
You may include one or more of these components when embedding QScript, but it isn't done by default. IF not otherwise specified, libraries are independant from eachother.

## Input/output library
This library provides access to files, console, and I/O memory buffers.

### Global functions
- print(...items): print the given items to standard output

### IO class: is Sequence
This class represents an I/O stream.
You can use `?`and `!` operators to determine if a stream is still open and valid.
Iterating through a stream yield lines, as if you call readLine repeatedly.

*`static IO.open(target, [mode="r"], encoding=null)`*: open the given target in the specified mode; optionally encode/decode binary data to/from  the stream into a given character encoding such as UTF-8.

- *target* can currently only be a file name, you may indifferently use `\` or `/` as path sepparator most of the time on windows.
- *mode* is a C-like open mode, it can be *r* for reading, *w* for writing, *a* for appending, with an optional *b* for binary mode as opposed to text mode by default.

Other methods:

- static IO.create(encoding=""): create a write stream to a buffer or string
- static IO.of(source, encoding=""): create a read stream from a buffer or string
- close: close the stream; for output streams, flush before closing.
- flush: flushes the output stream to disk/network/etc.
- read(count=-1): read up to count bytes or characters, or all the data to the end of the stream if count<0. REturns a string or buffer depending on context.
- readLine: read up to the next line or end of the stream and returns the line read.
- seek(position, absolute=false): seek forward or backward in the stream. If absolute=false, then the cursor is moved relatively to current position; if absolute=true, cursor is moved to the absolute position specified, from the beginning if position>=0, from the end if position<0.
- tell: gives the current cursor position in the stream
- toBuffer: for memory output stream created by IO.create() only, creates a buffer of all data written so far to the stream
- toString: for memory output stream created by IO.create() only, creates an UTF-8 string of all data written so far to the stream; for other types of streams, call the usual toString.
- write(data): write data to the stream; depending on context, data must be a string or a buffer.


## ADditions to System class

Static properties:

- in: Read-only IO stream connected to the standard input, using native-dependent encoding
- out: WRite-only IO stream connected to the standard output, using native-dependent encoding
- err: Write-only IO stream connected to the standard error output, using native-dependent encoding
