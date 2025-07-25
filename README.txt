## 🐚 Mysh – A UNIX-Style Shell in C

**Mysh** is a custom-built UNIX-style shell implemented in C that supports a wide range of command-line features found in modern shells like Bash and Zsh. Designed for both **interactive** and **batch** modes, Mysh can handle standard shell behaviors such as:

* **Pipes (`|`)**
* **Input/Output redirection (`<`, `>`)**
* **Wildcards (`*`)**
* **Built-in commands (`cd`, `pwd`, `which`, `exit`)**
* **Conditional execution** using `then` and `else` statements

The shell uses a robust **tokenization and parsing engine**, with custom `Token` and `Command` data structures to handle argument processing, redirection, and wildcard expansion. All command execution is handled through `fork` and `execv`, with support for bare commands and absolute/relative paths.

### 🔧 Key Features

* Full support for batch mode (`./mysh script.sh < input.txt > output.txt`) and interactive mode (`mysh>` prompt)
* Single-pipe execution: `cmd1 | cmd2`, with proper redirection support on either side
* Wildcard matching (`ls *.txt`, `cat *A.txt`) using custom glob-like logic
* Built-in shell commands with stateful behavior (`cd`, `exit`, etc.)
* Conditional logic:

  ```bash
  ls missingfile
  then echo success
  else echo failure
  ```

### ✅ Test Coverage

Mysh has been validated with a comprehensive suite of **12 manually written test cases**, each designed to confirm specific features in both **batch** and **interactive** modes. These tests evaluate the shell’s handling of input/output redirection, pipes, wildcards, conditional logic, and execution types (bare vs pathnames).

➡️ **Full list of test cases and expected behavior is available in [`tests.md`](./tests.md).**

**Example Highlights:**

* `./mysh myscript.sh < input.txt > output.txt` — runs a batch script with redirected I/O
* `cat < input.txt | tr a-z A-Z > output.txt` — tests piping with both input and output redirection
* `ls *.txt | grep file > output3.txt` — chains wildcard expansion with piping and redirection
* Conditional execution:

  ```bash
  ls missingfile
  then echo success
  else echo failure
  ```

### 🧪 Testing Breakdown

Covered scenarios include:

* Batch execution with redirected input/output
* Interactive command handling and prompt loop
* Bare vs. pathname execution (`ls`, `./a.out`, etc.)
* Wildcard logic across prefix/infix/suffix positions
* Input/output redirection in combination with pipes
* Conditional branching (`then`/`else`) tied to last command status

### 🧱 Architecture & Logic

**Core Structs:**

* `Token`: Stores parsed tokens, wildcards, pipe locations, and counts
* `Command`: Structured version of a command including redirection paths and arguments

**Key Functions:**

* `tokenize()`: Parses input into structured tokens and identifies control operators
* `process_token()`: Converts `Token` into an executable `Command` object
* `handle_wildcard()`, `wildcard_match()`: Expands wildcard expressions manually
* `process_pipe()`: Splits and handles commands on either side of a single pipe
* `process_command()`: Determines command type (builtin, pathname, barename) and executes via `fork`/`execv`
* `exit_shell()`, `free_token()`, `free_command()`: Exit and memory management utilities
* `main()`: Manages batch vs. interactive input, last-exit conditional logic, and lifecycle control

### 📌 Limitations

* Only a **single pipe (`|`)** is supported per command
* If command output doesn’t include a newline (e.g., `cat`), the prompt (`mysh>`) may appear on the same line
* **Quoted strings and pathnames with spaces** are not supported. Shell does not yet interpret double quotes as grouped tokens.

### 📁 Example Usage

```bash
mysh> ls *.txt | grep hello > results.txt
mysh> cat < input.txt | tr a-z A-Z > output.txt
mysh> cd tests
mysh> which ls > path.txt
```

### 👤 Author

Ayush Kadakia
