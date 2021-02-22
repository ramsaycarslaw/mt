# Use Statements

> Note: Given the similatities between Rusts syntax and `MT`'s, I have used the Rust syntax highlighting, not all keywords will be highlighted.

Use statements are the primary method of linking code in `MT`. Use statements are still a work in progress but they are stable enough to use in your code and there is no planned changes to the syntax.

Minimal Example
---------------
Comsider two files `file1.mt` and `file2.mt`, if we 'use' `file2.mt` from `file1.mt` then all the code in `file2.mt` will be run.

```rust
// file2.mt

print "Hello from file 2";
```

And `file1.mt`:

```rust
// file1.mt

use "file2.mt";
```

Then we have the following


```sh
$ mt file1.mt
Hello from file 2
```

Syntax
------

The syntax of a use statement was inspired by Rust. it is as follows:
```rust
use "<filepath>";
```

Where `<filepath>` is the *path* of the file you want to import.

### Nested Directories

Nested directorys work as you would expect, assume that `file2.mt` was moved to the path `/lib/src/file2.mt` then we can ammed the use statement as you would expect.

```rust
use "/lib/src/file2.mt";
```


### Relative Paths

Again, relative paths work as you would imagine, assume `file1.mt` is placed in a folder called `src` and `file2.mt` is in the root folder (one directory up), then we can use:

```rust
use "../file2.mt";
```


## Known Issues

Use statements *are* protected against import loops (i.e. file 1 imports file 2 which imports file 1 etc.) but there is a bug in how this is handeled. In the aforementioned case where file 1 imports 2, the contents of file 1 will be run twice instead of once, this will be addressed in an upcomming version.
