# EATSHOW - Edinburgh Associative Thesaurus Viewer

A command-line utility for exploring word associations in the Edinburgh Associative Thesaurus (EAT). Originally developed at Rutherford Appleton Laboratory in 1988, with enhancements added in 2005.

## Overview

EATSHOW allows you to search for words in the Edinburgh Associative Thesaurus and view their associated words along with frequency statistics. The program operates in two modes:

- **Stimulus Mode**: Shows responses given when the query word was used as a stimulus
- **Response Mode**: Shows stimuli that elicited the query word as a response

## Requirements

- C compiler (gcc, cl, icl, or compatible)
- Edinburgh Associative Thesaurus data files:
  - `sr_concise` - Stimulus-Response data
  - `rs_concise` - Response-Stimulus data
  - `sr_index` - Stimulus-Response index
  - `rs_index` - Response-Stimulus index

## Compilation

```bash
# GCC
gcc -Wall eatshow.c -o eatshow

# Microsoft Visual C++
cl eatshow.c

# Intel C Compiler
icl -Wall eatshow.c
```

## Usage

### Interactive Mode

Run without arguments to enter interactive mode:

```bash
./eatshow
```

You'll be prompted to enter words. Press Enter (without typing a word) to exit.

### Command Line Mode

Pass words directly as arguments:

```bash
./eatshow word1 word2 word3
```

### Command Line Options

- `-a` - Display additional information about the application
- `-d` - Turn off results demarcation (no separating lines)
- `-f<file>` - Echo output to a file (appends if file exists)
- `-i<file>` - Read and process input from a file (one word per line)
- `-l<n>` - Limit output to first n results
- `-n` - Number the output results
- `-r` - Use Response mode (find stimuli for the given response)
- `-s` - Use Stimulus mode (default - find responses for the given stimulus)
- `-sw` - Toggle between Stimulus and Response modes (runtime switch)
- `-t` - Use tab-delimited output instead of spaces
- `-x` - Dump the complete index wordlist for the current mode
- `-?` - Display usage information

### Examples

**Basic word lookup:**
```bash
./eatshow man
```

**Number the results:**
```bash
./eatshow -n woman
```

**Switch modes during execution:**
```bash
./eatshow man -sw man
```
First shows responses when "man" was a stimulus, then shows stimuli where "man" was a response.

**Tab-delimited output to file:**
```bash
./eatshow -t -foutput.txt -n dog cat
```

**Response mode with tab output:**
```bash
./eatshow -r -t happy
```

**Process words from a file:**
```bash
./eatshow -i wordlist.txt
```

## Output Format

For each word found, the program displays:

1. **Associated Words**: Words that are linked to the query word
2. **Frequency Count**: Number of times each association occurred
3. **Proportion**: Frequency as a proportion of total responses
4. **Summary Statistics**:
   - Total number of different responses
   - Total count of all responses

## Data Files

The program expects the following data files in the current directory:

- `./sr_concise` - Contains 8,211 stimulus-response pairs
- `./rs_concise` - Contains 22,776 response-stimulus pairs
- `./sr_index` - Index for stimulus-response lookups
- `./rs_index` - Index for response-stimulus lookups

**Note**: File paths can be modified by changing the constants at the top of the source code.

## Features

- **Dual Mode Operation**: Search by stimulus or response
- **Flexible Output**: Tab or space-delimited formatting
- **File I/O**: Read word lists from files, echo results to files
- **Result Limiting**: Control number of results displayed
- **Runtime Mode Switching**: Toggle between modes without restarting
- **Error Handling**: Gracefully handles missing words and file access errors

## History

- **June 1988**: Original version by M.D. Wilson, Rutherford Appleton Laboratory
- **June 2005**: Enhanced by peetm (peet.morris@comlab.ox.ac.uk)
  - Added command-line word processing
  - Added multiple switches for output control
  - Added file input/output capabilities

## Edinburgh Associative Thesaurus

The Edinburgh Associative Thesaurus is a database of word associations collected from human subjects. It's widely used in psycholinguistics, cognitive science, and natural language processing research.

For more information, visit: http://www.eat.rl.ac.uk/

## License

Original code from Rutherford Appleton Laboratory. Modified version free to use and distribute.

## Notes

- Words are automatically converted to uppercase for searching
- The program creates an `es.log` file on first run in the current directory
- Inaccessible or missing data files will cause the program to exit with an error message
