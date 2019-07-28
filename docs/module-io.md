# IO module
The IO module provides very simple i/O to read and write text files encoded in UTF-8.
Binary files or text files in other encodings aren't yet supported.

## FileReader: is Reader
FileReader is a Reader that reads content from a file

Constructor: FileReader(file, mode="r"): open the file for reading with the mode specified

## FileWriter: is Writer
FileWriter is a Writer that writes content to a file.

Constructor: FileWriter(file, mode="w"): open the file for writing with the given mode

## Reader: is Iterable
The Reader class is the base class for everything that read content from I/O streams.
Reader inherits from Iterable and its next method allow to read text content line by line.

Methods:

- close: close the I/O stream and all its associated system resources
- next: read a line from the stream
- read(n=-1): read up to n characters from the stream. If n is negative or omited, read everything up to the end of what is available.
- readLine: read a line, alias for next
- seek(n): seeks (advance forward or backward) the position of the file by n bytes

Properties:

- position: current position in bytes in the file. Setting the property to a new value allow to seek in another position.

## StringReader: is Reader
StringReader is a Reader that reads content from a string in memory.

Constructor: StringReader(str): with a string from which to read the content

## StringWriter: is Writer
StringWriter is a Writer that writes content to a string in memory.

Methods:

- Constructor: StringWriter(): no argument
- toString: return a copy of the string resulting from everything that has been written so far

## Writer: is Object
The Writer class is the base class for everything that writes content to I/O streams.

Methods:

- close: close the I/O stream and all its associated system resources
- flush: flush the I/O stream
- write(data): write the data given to the stream
