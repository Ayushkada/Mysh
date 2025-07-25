## 🧪 Mysh Test Suite

This document outlines 12 manually written test cases to validate all supported features of the `Mysh` shell in both **batch** and **interactive** modes. Each test includes the command, description, and expected output behavior.

---

### 📂 Batch Mode Tests

**Test 1:**

```bash
./mysh myscript.sh < input.txt > output.txt
```

- Executes a batch script (`myscript.sh`) with redirected input and output.

**Test 2:**

```bash
cat myscript.sh | ./mysh
```

- Runs `myscript.sh` in batch mode via pipe from `cat`, simulating unnamed input.

---

### 💻 Interactive Mode Tests

**Test 1:**

```bash
pwd
```

- Should display the current working directory.

**Test 2:**

```bash
cd tests
```

- Changes directory to `tests`.

**Test 3:**

```bash
pwd
```

- Confirms if current directory is now `tests`.

**Test 4:**

```bash
which ls > output1.txt
```

- Redirects path of `ls` command (e.g., `/bin/ls`) to `output1.txt`.

**Test 5:**

```bash
cat < input1.txt | tr > output2.txt 'a-z' 'A-Z'
```

- Uses input redirection and pipe to convert lowercase text to uppercase.

**Test 6:**

```bash
gcc -o path_test/program path_test/program.c
./path_test/program
```

- Tests support for pathnames; expected to output `hello`.

**Test 7:**

```bash
ls file*.txt
```

- Wildcard match for files like `fileA.txt`, `fileB.txt`.

**Test 8:**

```bash
cat *B.txt
```

- Tests wildcard match with prefix (`fileB.txt`).

---

### 🔀 Combined Feature Tests

**Test 9:**

```bash
ls *.txt | grep file > output3.txt
```

- Combines wildcard expansion, piping, and output redirection.

**Test 10:**

```bash
ls *.txt | grep > output4.txt file < input2.txt
```

- Piping + output redirection + right-side input redirection. Input should come from `input2.txt`.

---

### ❗ Conditional Logic Tests

**Test 11:**

```bash
ls adwadwaw
then echo success
else echo failure
```

- Since `ls adwadwaw` fails, it should execute `echo failure`.

```bash
cd ..
then which ls
else pwd
```

- If `cd ..` succeeds, executes `which ls`.

```bash
ls
else exit
pwd
```

- Runs `ls`, skips `exit`, then runs `pwd`.

**Test 12:**

```bash
ls *.txt > output5.txt | exit hi
```

- Completes `ls *.txt`, prints `hi`, then exits.

---

### ⚠️ Known Constraints

- Only **one pipe** is supported per command.
- Output without trailing newlines may overlap with prompt (`mysh>`).
- **No support for spaces in paths** (quoted strings not parsed as grouped tokens).

---

Use these tests to validate feature coverage and identify edge case behavior.
