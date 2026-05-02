# TinyShell
A simple shell program I am writing to learn C programming. I am building it by following the ["Build Your Own Shell" Challenge](https://app.codecrafters.io/courses/shell/overview) on [codecrafters.io](https://codecrafters.io).

## Why am I learning C?
To discuss this let's go back to before I learned C. I started programming 2 years ago from now and I was writing mostly `javascript` and `python`, but languages like these have most of the stuff abstracted from the programmer and I had this urge to take a peek beneath the abstraction, then I learned that `javascript` and `python` were also written in a programming language called `C`(Solved one of my high school mysteries of "if programming languages were used to write instructions for the computers then how are the programming languages themselves built?") and that's when I decided to start learning `C`. I am really not sure if learning C will be of any use to me in future but the thing that does keep me glued to this language, that has almost nothing compared to any modern language is that I learn a lot about the machine itself when I program in C.

## Features
- 5 Builtin commands: `echo`, `type`, `exit`, `pwd`, `cd` all have the same functionaly as their `bash` counterpart.
- Support for quotes, single/double
- Support for escaping characters with backslash
- Executing external programs like `git`.
- Redirecting `stdout` and `stderr`.  