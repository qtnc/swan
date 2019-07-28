# FS module
The FS module provides simple filesystem, path, file and directory manipulation functions.

## DirectoryIterator: is Iterable
A DirectoryIterator allows to iterate through the files present inside a directory.
Once constructed, the method next will return paths to existing files inside the directory, one by one.

Constructor: DirectoryIterator(path): with path pointing to a directory

## Path: is Object
A path represents a file or a directory that may or may not physically exist on the computer.
It's the central point of the FS module.
An error may be thrown during any of the operations made on path, if access fail because nothing exists at the path, if a file is expected instead of a directory or vice-versa, if access is restricted due to permissions, or for any other reason.

- Constructor: Path(path): where path is a string representing a valid path to a filesystem resource
- Operators: `+, /`

Methods:

- abs(base): make an absolute path out of this path, using the base given
- copy(target, overwrite=false): copy the file at this path to target. 
- delete(recurse=false): delete the file at this path. There's no trash, no going back. If this path designates a directory, recursive=true deletes all files inside the directory, recursive=false deletes the directory only if it's empty.
- exists: checks if a file or a directory effectively exists at this path
- extension: return the file extension part of this path. For example, ".txt" would be returned for the path "folder/subfolder/file.txt".
- isDirectory: return true if this path exists and is a directory
- isFile: return true if this path exists and is a file
- length: return the size of the file at this path
- mkdirs: create one or more directories so that this path become a valid directory. Return true if any directory has been created as a result of this call, or false if all directories already existed before this call
- name: return the file name part of this path. For example "file.txt" would be returned for the path "folder/subfolder/file.txt".
- parent: return the parent of this path, i.e. a new path where the last component is removed. For example, "folder/subfolder" would be returned for the path "folder/subfolder/file.txt".
- rel(base): make a relative path out of this path, using the base given
- rename(target): rename the file at this path to target
- stem: return the stem part of this path, i.e. the file name without its extension. For example, "file" would be returned for the path "folder/subfolder/file.txt".
- toString: return the string representation of this path, for example "relative/test.txt", "/home/user/folder/file.txt" or "C:\folder\subfolder\file.txt"

Static properties:

- current: return or change the current working directory, a.k.a. cwd and chdir

## RecursiveDirectoryIterator: is Iterable
A RecursiveDirectoryIterator allows to iterate through the files present inside a directory.
Once constructed, the method next will return paths to existing files inside the directory, one by one.

The difference between RecursiveDirectoryIterator and DirectoryIterator is that the former enumerates recursively all elements inside the directory and all subdirectories, while the later will limit to the elements of the directory given without looking at subdirectories.

Constructor: RecursiveDirectoryIterator(path): with path pointing to a directory
