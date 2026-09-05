# gubic executable v00.00-00

**note**: before v1, this format will change sometimes without an increase to a version number. be careful, people of the future!

**note**: this covers parts for `.gobj` too. (i can't believe the go programming language exists... ugh...)

**note**: this is my first time designing my own executable format. expect unclear specifications! the gubic kernel has a lot of useful info on wtf do i mean (most of the time?)

**note**: sorry for three notes! and a useless one!.. okay what am i doing here

the endianness of the gubic executable format is little endian.

a gubic executable is loaded at offset 0x2000 for tag № 1's contents, and the code is loaded at `0x2000` + length of data rounded to nearest multiple of 0x100. an executable can call `gubWhereAmI()` to get the code load offset from `0x2000` as a `uint32_t`.

## the header

every file needs a header that mandatorily sits at the start of the file, otherwise detection becomes a tiny bit funky...

|size, b|description|
|:-:|:-|
|16|mandatory text at the start of the file which reads in ascii: `GUBIC EXE AHEAD!`|
|2|major and minor version of format used. for v00.00-00, this value would read `0x0000` (`00 00` in le)|
|1|patch number. incremented when i don't want to change the minor/major version but don't really want to cause mishaps in older gubic versions. for v00.00-00, this would read `0x00`|

## the rest

you might wonder... why's the header so small? well my friend this file format is tag-based. what does that mean? the sections of a `.gx` (or a `.gobj`) can be rearranged with no side effects! perfect for an executable format of my own, it'll be like multiboot2 tags. :)

### tag header

yep. the tag has it's own header. deal with it.

no padding should be placed after tags. the whole file is packed.

|size, b|description|
|:-:|:-|
|1|if `00`, no more tags should be read. if `01`, the tag can be read. other values are reserved.|
|1|tag type. see below for available tag types. other values are reserved.|
|4|tag size.|

### tags

only the first instance of all of these tags will be parsed unless explicitly written differently.

#### № 1

|size, b|description|
|:-:|:-|
|4|minimum allocated memory size.|
|1|if `01`, the memory will follow this byte. if `02`, every byte will have an undefined value. if `00`, every byte will be guaranteed to be filled with zeroes.|
|variable|this value should only be present if the uint8_t has a value of `01`.|

#### № 2

this tag is the export byte, it shows where to call an exported function. globals cannot be exported, make a function that returns the global instead.

|size, b|description|
|:-:|:-|
|4|offset in code in bytes, where `0x00000000` is the beginning of the code, usually a crt or the `_start` entrypoint.|
|variable|null terminated ascii string of the export name. recommended export's name's allowed characters are all alphanumericals (uppercase *and* lowercase!) and the `_` underscore character.|

#### № 3

this tag is the code byte which will loaded and then `jmp`ed to. the static data and code offset is achieved

|size, b|description|
|:-:|:-|
|4|size of code.|
|variable|code|

#### № 4

this tag specifies a resource. a resource can either be read from a file, or bundled with the executable itself.

every instance of this tag type will be parsed, except when the resource name is already loaded and a similar resource with a duplicate resource name is specified.

|size, b|description|
|:-:|:-|
|variable|null terminated name of resource. any character is allowed, even unprintable ones.|
|1|type of resource. `00` is a bundled resource, `01` is an external resource. other values are reserved.|
|variable|see below|

if the resource type is `00`, here's what should follow after `00`:

|size, b|description|
|:-:|:-|
|4|size of resource.|
|variable|resource|

otherwise, this should follow `01`:

|size, b|description|
|:-:|:-|
|variable|null terminated path of resource. this path must be a valid path, or else the loader has the choice to either not include the resource or not run the executable at all. the path is recommended to be relative, though loaders may support absolute paths which should then begin with `/`.|
