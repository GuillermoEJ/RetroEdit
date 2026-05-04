# retroedit

Terminal code editor inspired by vim with its own built-in version control. C++17, no dependencies.

## build

```
mkdir build && cd build
cmake ..
cmake --build .
```

## usage

```
./retroedit file.cpp
```

Works like vim: `i` to type, `Esc` to go back to normal mode, `:w` save, `:q` quit.

Move with `hjkl`, `x` deletes char, `d` deletes line.

## built-in vcs

```
:vinit            init vcs for current file
:vcommit message  create a snapshot
:vlog             commit history
:vdiff            changes vs last commit
:vstatus          current state
```

Snapshots are stored in `.retroedit/` next to the file.

## license

MIT - Guillermo España Jiménez
