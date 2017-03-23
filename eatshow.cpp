/*
 * Find words in the RAL version of the Edinburgh Associative Thesaurus.
 * *** EATSHOW.C ***
 * 
 * M.D. Wilson Informatics Department, Rutherford Appleton Laboratory,
 * Chilton, Didcot, Oxon, OQX OX11, U.K.
 * 
 * June 1988
 * 
 * comlpile with:
 * 
 * cc eatshow.c
 * 
 * The file names in the define statements should be changed for the local
 * installation.
 * 
 * The number of words in the files may change if they are altered and
 * these values should also be altered in the define statements. 
 * ========================================================================
 *
 *
 * ************************************************************************
 * Modified summer 2005 by peetm (peet.morris@cslab.com).
 *
 * Added some extra switches: (run the app with -h for full list),  and made 
 * it a little more C++ like (but not much - no need to fix it!). Also added
 * the ability for  the  program to be  passed a series of words/switches at
 * the commandline.
 *
 * compile with: gcc -Wall, cl -W4, icl -Wall etc - gcc eatshow.cpp
 * ************************************************************************
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>


// Forward declarations.
//
static void doFileCloseOpen(char, FILE **, FILE **, int *);
static void out(int opt, const char * format, ...);
static void toggleDataBase(void);
static void checkForWord(char *);
static void checkInitRun(void);
static void actOnFlag(char *);
static void fromFile(char *);
static void resetFlags(void);
static void dumpWords(void);
static void trimLF(char *);
static void trim(char *);
static void about(void);
static void usage(void);


// Consts.
//
static const char * const SRFILE  = "./sr_concise";         /* file containing s-r data                             */
static const char * const RSFILE  = "./rs_concise";         /* file containing r-s data                             */
static const char * const SRINDEX = "./sr_index";           /* s-r index file                                       */
static const char * const RSINDEX = "./rs_index";           /* r-s index file                                       */

                                                            /* Constants to use with the out() function             */
static const int out_std   = 1;                             /* stdout output                                        */
static const int out_err   = 2;                             /* stderr output                                        */
static const int out_fle   = 4;                             /* echo file output (if given)                          */
static const int out_reg   = out_std | out_fle;             /* stdout and echo file output (if given)               */
static const int opt_all   = out_err | out_std | out_fle;   /* all of the above                                     */

static const int SRLENGTH = 8211;                           /* number of headwords in s-r data                      */
static const int RSLENGTH = 22776;                          /* number of headwords in r-s data                      */
static const int MAXBUF   = 10000;                          /* Maximum buffer size for words or association lists   */
static const int MAXPATH  = 260;

static const int WORD_FOUND = 0;                            /* 'cue' found in the database                          */


// Vars.

static char sourcef  = 's';                                 /* default source file is srfile                        */
static bool tabPad   = false;                               /* use tabs defaults to false                           */
static bool number   = false;                               /* number the ouputs                                    */
static bool initRun  = true;                                /* first run of this application?                       */
static bool demark   = true;                                /* demark (~~~~) results?                               */
static bool limit    = false;                               /* limit output to nLimit entries?                      */

static int numLimit    = 0;

static char * inptFile = NULL;                              /* file to read instructions from                       */

static FILE * echoFP = NULL;                                /* echoFile's file handle                               */
static FILE * fp     = NULL;                                /* main db file                                         */
static FILE * fp1    = NULL;                                /* index file for whatever fp points to                 */

static int index_length = 0;


#define RECURSIVE_GUARD
 
   
// Entry point.
//
int main(int argc, char * argv[])
{
    auto int i = 0;
    
    auto char cue[MAXBUF]; 

        
    // Is this the first time this app's been run from this location?  Sets the global 'initRun' to the result.
    //
    checkInitRun();

    // Check for 'flags' and act accordingly if found.
    //
    for(i = 1; i < argc; i++)
    {
        if(*argv[i] == '-')
        {
            // actOnFlag won't return if -i used.
            //
            actOnFlag(argv[i]);
        }
        else
        {
            // ** Valid 'initial' switches come before non-switches (words) 
            // We break out of the for loop when/if we hit the first non-wsitch.
            //
            // e.g., consider: eatshow -ftest.txt -n -r man -w man
            //
            // Here, -f -n and -r will be processed, but -w won't [at this time] -
            // the break below will happen when 'man' is seen.
            //
            break;
        }
    }

    // actOnFlag may have altered sourcef from the default 's' (use the stimulus db)        
    // AND, we need to make sure we've a db open - this routine does that.
    //
    doFileCloseOpen(sourcef, &fp, &fp1, &index_length);
    
    // i was set up in the for loop above - that loop may have exited before it
    // reached argc's value - see ** for why.
    //
    // NOTE: If i < argv, we're in commandline mode.
    //
    if(i < argc)
    {
        while(i < argc)
        {
            // Also checks for runtime switches.
            //
            checkForWord(argv[i++]);
        }
        
        // Mostly used here as it'll close echoFP if it's open.
        //
        resetFlags();
        
        // Exit as we're all done in commandline mode.
        //
        return EXIT_SUCCESS;
    }
    else
    {   
        // As printf returns the number of chars output, it works quite well
        // when used in an && like this.
        //
        while(printf("Enter a word>") && fgets(cue, sizeof(cue), stdin) && strlen(cue) != 1)
        {
            checkForWord(cue);
        }
        
        // Mostly used here as it'll close echoFP if it's open.
        //
        resetFlags();
        
        return EXIT_SUCCESS;
    }
}



// Opens appropriate data and index files - closes any that are already open first.
//
static void doFileCloseOpen(char sourcef, FILE ** fp, FILE ** fp1, int * index_length)
{
    if(*fp != NULL)
    {
        fclose(*fp);
    }
    
    if(*fp1 != NULL)
    {
        fclose(*fp1);
    }
    
    if(sourcef == 's')
    {
        if((*fp = fopen(SRFILE, "r")) == NULL)
        {
            out(out_err, "cannot access the file: %s\n", SRFILE);
            
            exit(EXIT_FAILURE);
        }

        if((*fp1 = fopen(SRINDEX, "r")) == NULL)
        {
            out(out_err, "cannot access the file: %s\n", SRINDEX);
            
            exit(EXIT_FAILURE);
        }

        *index_length = SRLENGTH;
    }

    if(sourcef == 'r')
    {
        if((*fp = fopen(RSFILE, "r")) == NULL)
        {
            out(out_err, "cannot access the file: %s\n", RSFILE);
            
            exit(EXIT_FAILURE);
        }

        if((*fp1 = fopen(RSINDEX, "r")) == NULL)
        {
            out(out_err, "cannot access the file: %s\n", RSINDEX);
            
            exit(EXIT_FAILURE);
        }

        *index_length = RSLENGTH;
    }
    
    return;
}



// The functionally 'main' routine - checks in the index/data files for relevant data and outputs it.
//
static void checkForWord(char * cue)
{

    auto int fail               = 3;  // 3 = 'cue' word not found.
    auto int tot_rec            = 0;
    auto int tot_freq           = 0;
    auto int count              = 0;
    auto int limitCount         = 0;    
    
    auto float prop             = 0;
    
    auto long int tail_address  = 0;
    auto long int head_address  = 0;

    auto char * out1            = NULL;
    auto char * out2            = NULL;

    auto char * tmp_p           = NULL;
        
    auto char response_word[MAXBUF];
    auto char index_word   [MAXBUF];    
    auto char buffer[100];
    
    const char * format         = tabPad ? "%s\t%d\t%.2f\n" : "%-25s %3d %5.2f\n";

            
    doFileCloseOpen(sourcef, &fp, &fp1, &index_length);
    
    // If it's a switch, don't go further here as actOnFlag should set stuff up etc.
    //
    if(*cue == '-')
    {
        actOnFlag(cue);
        
        return;
    }
    
    trimLF(cue);
    
    strupr(cue);

    out(out_std, "\nLooking for: %s in %s MODE\n\n", cue, sourcef == 'r' ? "RESPONSE" : "STIMULUS");
    
    while(!feof(fp1))
    { 
        if(fgets(index_word, 21, fp1) == NULL)
        {
            fail = 1;
            
            break;
        }

        if(fscanf(fp1, "%d %d %ld %ld\n", &tot_rec, &tot_freq, &head_address, &tail_address) != 4)
        {
            fail = 2;
            
            break;
        }

        trim(index_word);

        if(strcmp(index_word, cue) == WORD_FOUND)
        {
            fail = 0;
            
            break;
        }
    }

    if(fail != 0)
    {
        out(out_std, "eatshow: %s: not found\n", cue);
    }
    else // We found 'cue' - w00t!
    {   
        if(fseek(fp, tail_address, 0) != -1)
        {
            if(fgets(response_word, MAXBUF, fp) == NULL)
            {
                out(out_err, "eatshow: %ld: bad address index file\n", tail_address);
            }
            else
            {         
                out1 = NULL;
                
                if(demark)
                {
                    out(out_std, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
                }

                // BUG FIX:
                //
                // Occasionally response_word contains multiple delimiters, e.g. FAIL||1 instead of FAIL|1.
                // This causes *** below to crash.  This is a work around to remove duplicate ||s
                //
                while((tmp_p = strstr(response_word, "||")) != NULL)
                {
                    *tmp_p++ = (char)32;
                    *tmp_p   = (char)32;
                }

                while(out1 = strtok(out1 == NULL ? response_word : (char *)0, "|"))
                {
                    out2 = strtok((char *)0, "|");
                 
                    // ***
                    //
                    count = strtol(out2, (char **)NULL, 10);

                    // Limit the output to numLimit entries?
                    //
                    if(limit)
                    {
                        if(limitCount >= numLimit)
                        {
                            break;
                        }
                    }
                
                    ++limitCount;                    
                    
                    // Number the outputs?
                    //
                    if(number)
                    {
                        sprintf(buffer, "%4d: %s", limitCount, out1);
                                        
                        prop = ((float)count / (float)tot_freq);

                        // Determine whether to use spaces or tabs in the output.
                        //    

                        out(echoFP != NULL ? out_reg | out_fle : out_reg, format, buffer, count, prop);
                    }
                    else
                    {
                        sprintf(buffer, "      %s", out1);

                        format = tabPad ? "%s\t%d\t%.2f\n" : "%-25s %3d %5.2f\n";

                        out(echoFP != NULL ? out_reg | out_fle : out_reg, format, buffer, count, prop);                        
                    }
                }
                
                if(demark)
                {
                    puts("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
                }

                out(out_std, "\n\t%s %s\n\n", cue, sourcef == 'r' ? "was [one of] the 'response(s)' to the stimulli above" : "was [one of] the 'stimuli' to the responses above");
                    
                out(out_std, "\tNumber of different answers: %d\n", tot_rec);
                
                out(out_std, "\t Total count of all answers: %d\n\n", tot_freq);
                
            }
        }
        else
        {
            out(out_std, "eatshow: %ld: bad address index file\n", tail_address);
        }
            
        rewind(fp1);
    }
    
    return;
}



// Toggles the database being used (Response/Stimulus).
//
static void toggleDataBase(void)
{
    if(sourcef == 's')
    {
        out(out_std, "\t>>Mode switched from Stimulus to Response\n");
        
        sourcef = 'r';
    }
    else
    {
        out(out_std, "\t>>Mode switched from Response to Stimulus\n");
                    
        sourcef = 's';
    }
    
    return;
}





// Checks to see if the file es.log is in the same folder as the app.  If 'no' we're
// basically assuming that this is the first time this user has ever run eatshow.  The
// idea is that we could give some extra guidance on startup you see.
//
// Also, if fpCheck is made global, and not closed until we exit the while loop in main(),
// it could be used to log debug or other runtime info.
//
static void checkInitRun(void)
{
    auto FILE * fpCheck = NULL;
    
    
    // Is this the first time this app has been run (from this folder)?
    //
    initRun = (fpCheck = fopen("es.log", "r")) != NULL;
    
    // Flag the fact that we've been run by writing out a log file.
    // initRun used later to give some 'first time' guidance.
    //
    if(!initRun)
    {
        if((fpCheck = fopen("es.log", "a")) != NULL)
        {
            fprintf(fpCheck, "eatshow initially run on this machine on %s %s\n", __DATE__, __TIME__);
            
            // Could leave this open and only close it as main() is about to exit.
            // Could be useful for various logging thingmys?
            //
            fclose(fpCheck);
        }
        
        out(out_err, "==================================================================\n");
        out(out_err, "Use eatshow -? at the command prompt for help on this application.\n");
        out(out_err, "==================================================================\n");        
    }
    else
    {
        fclose(fpCheck);
    }
    
    return;
}



// Produces a raw dump of the words in the current db's index file - numbers the output.
//
static void dumpWords(void)
{
    auto char index_word[MAXBUF];    
    
    auto int  ndummy  = 0;
    auto long lcount  = 0;
    
    
    doFileCloseOpen(sourcef, &fp, &fp1, &index_length);
    
    while(!feof(fp1))
    {
        if(fgets(index_word, 21, fp1) == NULL)
        {
            break;
        }

        if(fscanf(fp1, "%d %d %d %d\n", &ndummy, &ndummy, &ndummy, &ndummy) != 4)
        {
            break;
        }

        trim(index_word);
        
        ++lcount;
        
        if(number)
        {
            out(echoFP != NULL ? out_reg | out_fle : out_reg, "%5ld: %s\n", lcount, index_word);
        }
        else
        {
            out(echoFP != NULL ? out_reg | out_fle : out_reg, "%s\n", index_word);
        }
    }

    out(out_err, "\nAlthough %ld entries were listed, as this is a raw dump of the index,\n", lcount);
    out(out_err, "the output will contain a number of duplicates.");

    rewind(fp1);     
}




static void about(void)
{
    auto char timeBuff[sizeof(__TIME__) + 1];
    

    // Get rid of the seconds info.
    //    
    sprintf(timeBuff, "%s", __TIME__);
    //
    // Put a null terminator at the second colon position.
    //
    timeBuff[5] = '\0';
    
    out(out_std, "\n");
    out(out_std, "  ============================================================================\n");
    out(out_std, "  eatshow v1.1.6\n");
    out(out_std, "\n\t\t\tLast built: %s at %s\n\n\n", __DATE__, timeBuff);    
    out(out_std, "    See http://www.eat.rl.ac.uk/ for the original source and information.\n");
    out(out_std, "\n");
    out(out_std, "    Modified the original source at the URL - mainly to add the -ws -t -f -n\n");
    out(out_std, "    options for my own use.\n\n\tpeetm - peet.morris@comlab.ox.ac.uk\n");
    out(out_std, "\n");
    out(out_std, "\n");
    out(out_std, "  ============================================================================\n");
    out(out_std, "\n  Hit Enter for more...\n");
    getchar();
    out(out_std, "  Some 'option' examples:\n");
    out(out_std, "\n");    
    out(out_std, "    eatshow -n          ... numbers the results\n");
    out(out_std, "\n");    
    out(out_std, "    eatshow -ftest.txt  ... echo output to the file test.txt: file is appended\n");
    out(out_std, "                            to if it exists, or created anew if it does not\n");
    out(out_std, "\n");    
    out(out_std, "    eatshow man -w man  ... starts eatshow in non-interactive mode: first\n");
    out(out_std, "                            outputs the responses when 'man' was used as a\n");
    out(out_std, "                            stimuli, then toggles (-w) eatshow into response\n");
    out(out_std, "                            mode, in which the output lists words which were\n");
    out(out_std, "                            used as a stimulus, and where 'man' was a response\n");
    out(out_std, "\n");
    out(out_std, "    eatshow -r -t       ... starts eatshow in response mode, and instructs\n");
    out(out_std, "                            it to use tabs instead of spaces in its layouts\n");    
    out(out_std, "\n");    

    return;;    
}




// print proper usage and exit 
//
static void usage(void)
{         
    out(out_std, "Usage: eatshow [-a -d -f -i -n -r -s -w -t -x -?] [word_list]\n");
    out(out_std, "Find associates to words in the Edinburgh Associative Thesaurus\n");
    out(out_std, "\n");    
    out(out_std, "Outputs:\n");
    out(out_std, "\tThe total number of different answers, the count of\n");
    out(out_std, "\tall answers,  and  the list of triads of associated\n");
    out(out_std, "\ttypes, their individual frequencies, and proportion\n");
    out(out_std, "\tof occurrence.   The proportion of occurrence for a\n");
    out(out_std, "\tgiven type is its individual  frequency  divided by\n");
    out(out_std, "\tthe total count of all answers.\n");
    out(out_std, "\n");    
    out(out_std, "Switches:\n");
    out(out_std, "\t-a \t further info about this application\n");
    out(out_std, "\t-d \t turn off results demarcation\n");    
    out(out_std, "\t-f<file> echo screen output to a file\n");
    out(out_std, "\t-i<file> reads/processes input from a file a line at a time\n");
    out(out_std, "\t-l<n>\t limits the number of outputs to <n>\n");
    out(out_std, "\t-n \t number outputs\n");
    out(out_std, "\t-r \t use cue as response\n");
    out(out_std, "\t-s \t use cue as stimulus(default)\n");
    out(out_std, "\t-w \t toggles the -r/-s mode [without restart]\n");
    out(out_std, "\t-t \t tab-delimit output [default is to use spaces]\n");
    out(out_std, "\t-x \t dumps the index wordlist for the current mode\n");
    out(out_std, "\t-? \t display these options\n");
    out(out_std, "[MORE]");
    getchar();
    out(out_std, "\n");    
    out(out_std, "NOTE: If -i or a word_list is used, eatshow does not enter interactive mode\n");
    out(out_std, "\n");
    out(out_std, "In interactive mode, to return to the command prompt, simply hit return\n");
    out(out_std, "(do not enter a word). Alternately, enter Ctrl + Z\n");
    
    exit(EXIT_SUCCESS);
}



// Trims a LF off a string by inserting a '\0' in the location
// of the first LF found.
//
static void trimLF(char * string)
{
    auto char * p = NULL;
    
    
    if((p = strchr(string, (char)10)) != NULL)
    {
        *p = '\0';
    }
    
    return;
}



// Trims spaces off (nominally -the end-) of a string by inserting a '\0' in the location
// of the first space char found.
//
static void trim(char * string)
{
    auto char * p = NULL;
    
    
    if((p = strchr(string, (char)' ')) != NULL)
    {
        *p = '\0';
    }
    
    return;
}



// Parses command line to set options.
//
static void actOnFlag(char * f)
{                                  
    f++;
    
    switch(*strlwr(f++))
    {
        case 's':                   // read s-r file.
            sourcef = 's';
            break;
        
        case 'w':
            toggleDataBase();
            break;
            
        case 'r':
            sourcef = 'r';          // read r-s file.
            break;

        case 'f':                   // echo to file.
            if(*f != '\0')
            {
                if(strlen(f) != 0)
                {
                    // Close the current echo file if we've got one.
                    //
                    if(echoFP != NULL)
                    {
                        fclose(echoFP);
                    }
    
                    echoFP = NULL;

                    // fopen doesn't like a \n in f.
                    //
                    if(strstr(f, "\n"))
                    {
                        *strstr(f, "\n") = '\0';
                    }

                    echoFP = fopen(f, "a");
                    
                    if(!echoFP)
                    {
                        out(out_err, "file %s could not be opened!\n", f);
                    }
                    else
                    {
                        out(out_err, "echo file -> %s opened successfully\n", f);
                    }
                }        
            }
            break;
                        
        case 't':                   // tab output.
            tabPad = !tabPad;
            out(out_err, "tab mode -> %s\n", tabPad ? "true" : "false");
            break;
    
        case 'a':                   // show 'about'.
            about();
            break;
            
        case 'd':                   // add demarcation lines.
            demark = !demark;
            out(out_err, "demark mode -> %s\n", demark ? "true" : "false");
            break;

        case 'n':                   // number outputs.
            number = !number;
            out(out_err, "number mode -> %s\n", number ? "true" : "false");
            break;
            
        case 'x':
            dumpWords();
            break;
            
        case 'l':                   // limit the number of outputs.
            limit  = !limit;
            
            if(*f != '\0')
            {
                numLimit  = strtol(f, (char **)NULL, 10);
                
                if(numLimit > 0)
                {
                    limit = true;
                }
            }
            out(out_err, "limit mode -> %s (%d)\n", limit ? "true" : "false", numLimit);
            break;

        case '?':                   // show usage.
            usage();
            break;
            
        case 'i':                   // read input from file.
            if(*f != '\0')
            {
                fromFile(f);
                
                exit(EXIT_SUCCESS); // close here.
            }
            break;
            
        case NULL:                  // error.
            usage();
            break;
            
        default:
            break;            
    }
    
    return;
}




// Function to support -i<filename> usage.
//
// s is a pointer to the file to read.
//
// Basically, this function generates an argc/argv pair from each line in the
// input file.  It then calls main() to have the commands processed.
//
void fromFile(char * s)
{
    auto FILE * fp = NULL;
    

    if((fp = fopen(s, "r")) != NULL)
    {
        auto char   buffer[255];
        auto char * argv[10];
        
        auto int    argc    = 0;        
        auto int    n       = 0;
        auto char * p       = NULL;

        // argv[0] is always the appname.
        //
        argv[0] = (char *)malloc(strlen("eatshow") + 1);
        //
        strcpy(argv[0], "eatshow");

        while(!feof(fp))
        {
            // Get a line from the file.
            //
            if(fgets(&buffer[0], 255, fp))
            {
                // Is there a lf or a cr on a seperate line?
                //
                if(strlen(buffer) == 1)
                {
                    // Yes, ignore.
                    //
                    continue;
                }
                        
                #ifdef RECURSIVE_GUARD

                // Protect against recursing to death?  We're reading a file, and we really
                // wouldn't like that file to contain another -i reference to ourselves.
                //
                if(strstr(buffer, "-i"))
                {
                    continue;
                }

                #endif

                // +++ This bit now parses the line, and creates the argv array.
                //
                p = strtok(&buffer[0], " ");

                while(p && argc != 10)
                {
                    argv[++argc] = (char *)malloc(strlen(p) + 1);
                    
                    strcpy(argv[argc], p);

                    p = strtok(NULL, " ");
                }

                // +++
                
                // Put back defaults ... number output, use tabs, demarcation etc.
                //
                resetFlags();
                
                // Call main for each suitably parsed line from the file.
                //
                main(argc + 1, argv);
                
                // Tidy alloc'ed memory away.
                //                
                for(n = 1; n < argc; n++)
                {
                    free(argv[n]);
                }
            }
        } 
        
        // Free 'eatshow' alloc'ed memory.
        //
        free(argv[0]);
        
        fclose(fp);
    }
    else
    {
        out(out_err, "Error opening %s\n", s);
    }
    
    return;
}



// Put app into default state.  Mostly used when we're reading directives from a file
// using the -i option.
//
static void resetFlags(void)
{
    if(echoFP != NULL)
    {
        fclose(echoFP);
    }
    
    echoFP      = NULL;
    inptFile    = NULL;
    tabPad      = false;
    demark      = true;
    number      = false;
    limit       = false;
    numLimit    = 0;
    sourcef     = 's';
    
    return;
}



// Used to output stuff to the screen and, optionally, to a file.
//
static void out(int opt, const char * format, ...)
{
    // Universally, this [buffer] isn't general purpose enough - ok for eatshow though
    // and protected by using vsnprintf below.
    //
    auto char buffer[100];

    auto va_list ap;
    
    va_start(ap, format);

    // Output to buffer according to what's passed in format and ap (no matter what).
    //
    if((unsigned)vsnprintf(buffer, sizeof buffer, format, ap) > sizeof buffer)
    {
        // Although the stack is protected by vsnprintf.
        //
        out(out_err, "function 'out' - output too long for buffer\n");
    }

    va_end(ap);

    // stdout output.
    //
    if(opt & out_std)
    {
        fputs(&buffer[0], stdout);
    }
    
    // File output if fp points to something.
    //
    if(opt & out_fle)
    {
            if(echoFP != NULL)
            {
                fputs(&buffer[0], echoFP);
            }
    }
    
    // stderr output.
    //
    if(opt & out_err)
    {
        fputs(&buffer[0], stderr);
    }
    
    // Invalid flags given?
    //
    if(opt < 1 || opt > opt_all)
    {
        // Better report this!
        //
        out(out_err, "function 'out' called using opt value of %d: which is invalid", opt);
    }

    // Just in case.
    //
    fflush(stdout);
    fflush(stderr);
    //
    if(echoFP)
    {
        fflush(echoFP);
    }
    
    return;
}


