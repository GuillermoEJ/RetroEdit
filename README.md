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

Move with `hjkl`, `x` deletes char, `d` deletes line, `v` visual select, `y` yank, `p` paste.

`/pattern` to search, `n`/`N` to navigate matches. `u` undo, `Ctrl+r` redo.

## buffers

```
:e file.txt       open file in new buffer
:bn / :bp         next / previous buffer
:ls               list open buffers
```

## built-in vcs

```
:vinit            init vcs for current file
:vcommit message  create a snapshot
:vlog             commit history
:vdiff            changes vs last commit
:vstatus          current state
:vbranch name     create and switch to branch
:vcheckout name   switch branch or checkout commit hash
:vbranches        list branches
:vstash           stash current changes
:vstash pop       restore stashed changes
```

Snapshots are stored in `.retroedit/` next to the file.

## config

Create `~/.retroeditrc` or `.retroeditrc` in the working directory:

```
theme = green
tab_size = 4
relative_numbers = false
crt_effect = false
show_clock = true
```

Available themes: `green`, `amber`, `blue`, `white`.

Runtime: `:set rnu`, `:set nornu`, `:set theme amber`, `:set crt`, `:set nocrt`.

## license

MIT - Guillermo España Jiménez
