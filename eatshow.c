
/*
 * Find words in the RAL version of the Edinburgh Associative Thesaurus.
 * *** EATSHOW.C ***
 * 
 * M.D. Wilson Informatics Department, Rutherford Appleton Laboratory,
 * Chilton, Didcot, Oxon, OQX OX11, U.K.
 * 
 * June 1988
 * 
 * compile with:
 * 
 * cc eatshow.c
 * 
 * The file names in the define statements should be changed for the local
 * installation.
 * 
 * The number of words in the files may change if they are altered and
 * these values should also be altered in the define statements. 
 * ========================================================================
 * ************************************************************************
 * Altered June 2005 by peetm (peet.morris@comlab.ox.ac.uk/peet.morris@cslab.com).
 * Added some extra switches: (run the app with -h for full list)
 *
 * Also added the ability for the program to be passed a series of words at the
 * commandline.
 *
 * compile with: gcc -Wall, cl -W4, icl -Wall etc - gcc eatshow.cpp
 * ************************************************************************
 * ========================================================================
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

 /* Handle strupr portability */
#ifdef _MSC_VER
    /* Microsoft compiler - use _strupr */
#define STRUPR _strupr
#else
    /* GCC or other compilers - provide custom implementation */
static char * strupr(char * s)
{
    char * p = s;
    while (*p)
    {
        *p = toupper((unsigned char)*p);
        p++;
    }
    return s;
}
#define STRUPR strupr
#endif

// Forward declarations.
//
static void doFileCloseOpen(char, FILE **, FILE **, int *);
static bool toggleDataBaseCheck(char *);
static void out(char *, int, float);
static bool nothingEntered(char *);
static void checkForWord(char *);
static void checkInitRun(void);
static void fromFile(char *);
static bool dumpWords(char *);
static void actOnFlag(char *);
static void resetFlags(void);
static void trimLF(char *);
static void trim(char *);
static void about(void);
static void usage(void);

static const char * const SRFILE  = "./sr_concise";  /* file containing s-r data */
static const char * const RSFILE  = "./rs_concise";  /* file containing r-s data */
static const char * const SRINDEX = "./sr_index";    /* s-r index file           */
static const char * const RSINDEX = "./rs_index";    /* r-s index file           */

static const int SRLENGTH = 8211;        /* number of headwords in s-r data      */
static const int RSLENGTH = 22776;       /* number of headwords in r-s data      */

#define MAXBUF 10000                     /* Maximum buffer size for words or association lists */

static const int WORD_FOUND = 0;         /* 'cue' found in the database          */

static char sourcef  = 's';              /* default source file is srfile        */
static bool bTabPad  = false;            /* use tabs defaults to false           */
static bool bNumber  = false;            /* number the ouputs                    */
static bool binitRun = true;             /* first run of this application?       */
static bool bDemark  = true;             /* demark (~~~~) results?               */
static bool bLimit   = false;            /* limit output to nLimit entries?      */

static int nLimit    = 0;

static char * echoFile = NULL;           /* default file to echo to is NULL      */
static char * inptFile = NULL;           /* file to read instructions from       */

static FILE * fp  = NULL;                /* main db file                         */
static FILE * fp1 = NULL;                /* index file for whatever fp points to */

static int index_length = 0;
    
// Entry point.
//
int main(int argc, char * argv[])
{
    auto int i;
            
    // Is this the first time this app's been run from this location?  Sets the global 'binitRun' to the result.
    //
    checkInitRun();

    // Check for 'flags' and act accordingly if found.
    //
    for(i = 1; i < argc; i++)
    {
        if(*argv[i] == '-' || *argv[i] == '/')
        {
            // actOnFlag won't return if -i used.
            //
            actOnFlag(argv[i]);
        }
        else
        {
            // ** Valid 'initial' switches come before non-switches (words) and runtime
            // switches like -sw (which, for -sw, should come after a word really).  
            // We break out of the for loop when/if we hit the first non-switch.
            //
            // e.g., consider: eatshow -ftest.txt -n -r man -sw man
            //
            // Here, -f -n and -r will be processed, but -sw won't [at this time] -
            // the break below will happen when 'man' is seen.
            //
            break;
        }
    }

    // actOnFlag may have altered sourcef from the default 's' (use the stimulus db)        
    // AND, we need to make sure we've a db open - this routine does that.
    //
    doFileCloseOpen(sourcef, &fp, &fp1, &index_length);
    
    // The leading switches are done - but are there are other args? They
    // should be words or the -switch dbs' flag if there are.
    //
    auto char cue[MAXBUF]; 

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
        
        // Exit as we're all done in commandline mode.
        //
        return 0;
    }
    else
    {
        // As printf returns the number of chars output, it works quite well
        // when used in an && like this.
        //
        while (printf("Enter a word>") && fgets(cue, MAXBUF, stdin) && !nothingEntered(cue))
        {
            checkForWord(cue);
        }
        
        return 0;
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
            fprintf(stderr, "cannot access the file: %s\n", SRFILE);
            
            exit(1);
        }

        if((*fp1 = fopen(SRINDEX, "r")) == NULL)
        {
            fprintf(stderr, "cannot access the file: %s\n", SRINDEX);
            
            exit(1);
        }

        *index_length = SRLENGTH;
    }

    if(sourcef == 'r')
    {
        if((*fp = fopen(RSFILE, "r")) == NULL)
        {
            fprintf(stderr, "cannot access the file: %s\n", RSFILE);
            
            exit(1);
        }

        if((*fp1 = fopen(RSINDEX, "r")) == NULL)
        {
            fprintf(stderr, "cannot access the file: %s\n", RSINDEX);
            
            exit(1);
        }

        *index_length = RSLENGTH;
    }
    
    return;
}



// Used to output stuff to the screen and, optionally, to a file.
//
static void out(char * out1, int count, float prop)
{
    auto const char * format = NULL;
    
    // Determine whether to use spaces or tabs in the output.
    //    
    format = bTabPad ? "%s\t%d\t%.2f\n" : "%-25s %3d %5.2f\n";
    
    printf(format, out1, count, prop);
    
    // Been given a file name to echo output to?
    //
    if(echoFile != NULL)
    {
        // YES.
        
        auto FILE * ech = NULL;
        
        if((ech = fopen(echoFile, "a")) != NULL)
        {
            fprintf(ech, format, out1, count, prop);
        }
        fclose(ech);
    }    
    
    return;
}                        



// The functionally 'main' routine - checks in the index/data files for relevant data and outputs it.
//
static void checkForWord(char * cue)
{
    auto int count;
    auto int fail     = 3;  // 3 = 'cue' word not found.
    auto int tot_rec  = 0;
    auto int tot_freq = 0;
    
    auto float prop   = 0;
    
	auto long int tail_address = 0;
    auto long int head_address = 0;

    auto char * out1;
    auto char * out2;
    auto char response_word[MAXBUF];
    auto char index_word   [MAXBUF];    
    
    doFileCloseOpen(sourcef, &fp, &fp1, &index_length);
    
    // Check that cue isn't either the 'toggle db' or 'dump index words' flags.
    //
    if(toggleDataBaseCheck(cue) || dumpWords(cue))
    {
        return;
    }
    
    trimLF(cue);
    
    STRUPR(cue);

    printf("\nLooking for: %s in %s MODE\n\n", cue, sourcef == 'r' ? "RESPONSE" : "STIMULUS");
    
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
        printf("eatshow: %s: not found\n", cue);
    }
    else // We found 'cue' - w00t!
    {
        if(fseek(fp, tail_address, 0) != -1)
        {
            if(fgets(response_word, MAXBUF, fp) == NULL)
            {
                printf("eatshow: %ld: bad address index file\n", tail_address);
            }
            else
            {         
                auto int nCount = 0;
                
                out1 = NULL;
                
                if(bDemark)
                {
                    puts("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
                }
                
                while(out1 = strtok(out1 == NULL ? response_word :  (char *) 0  , "|"))
                {
                    out2 = strtok((char *) 0, "|");
                    
                    count = atoi(out2);

                    // Limit the output to nLimit entries?
                    //
                    if(bLimit)
                    {
                        if(nCount >= nLimit)
                        {
                            break;
                        }
                    }
                
                    auto char buffer[100];

                    // Number the outputs?
                    //
                    if(bNumber)
                    {
                        ++nCount;
        
                        sprintf(buffer, "%4d: %s", nCount, out1);
                                        
                        prop = ((float) count / (float) tot_freq);
                                        
                        out(buffer, count, prop);
                    }
                    else
                    {
                        sprintf(buffer, "      %s", out1);

                        out(buffer, count, prop);
                    }
                }
                
                if(bDemark)
                {
                    puts("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
                }

                printf("\n\t%s %s\n\n", cue, sourcef == 'r' ? "was [one of] the 'response(s)' to the stimulli above" : "was [one of] the 'stimuli' to the responses above");
                    
                printf("\tNumber of different answers: %d\n", tot_rec);
                
                printf("\t Total count of all answers: %d\n\n", tot_freq);
                
            }
        }
        else
        {
            printf("eatshow: %ld: bad address index file\n", tail_address);
        }
            
        rewind(fp1);
    }
    
    return;
}



// Toggles the database being used (Response/Stimulus) if passed '-sw'.
//
// Returns: true if the database was toggled, else false.
//
static bool toggleDataBaseCheck(char * cue)
{
    if(!strcmp(cue, "-sw"))
    {
        if(sourcef == 's')
        {
            puts("\t>>Mode switched from Stimulus to Response");
            
            sourcef = 'r';
        }
        else
        {
            puts("\t>>Mode switched from Response to Stimulus");
                        
            sourcef = 's';
        }
        
        return true;
    }    
    else
    {
        return false;
    }
}



// Checks for s being either NULL (a null pointer) or *s being NULL (obviously, it does this latter
// check first!
//
static bool nothingEntered(char * s)
{
    if( s == NULL || *s == '\0')
    {
        return true;
    }
    else
    {
        return false;
    }
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
    auto FILE * fpCheck;
    
    // Is this the first time this app has been run (from this folder)?
    //
    binitRun = (fpCheck = fopen("es.log", "r")) == NULL ? true : false;
    
    // Flag the fact that we've been run by writing out a log file.
    // binitRun used later to give some 'first time' guidance.
    //
    if(binitRun)
    {
        if((fpCheck = fopen("es.log", "a")) != NULL)
        {
            fprintf(fpCheck, "eatshow run at %s %s\n", __DATE__, __TIME__);
            
            // Could leave this open at only close it as main() is about to exit.
            // Could be useful for various logging thingmys?
            //
            fclose(fpCheck);
        }
        
        fprintf(stderr, "============================================\n");
        fprintf(stderr, "Use eatshow -? for help on this application.\n");
        fprintf(stderr, "============================================\n");        
    }
    else
    {
        fclose(fpCheck);
    }
    
    return;
}



// Produces a raw dump of the words in the current db's index file - numbers the output.
//
static bool dumpWords(char * cue)
{
    if(!strcmp(cue, "-x"))
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
            
            printf("%5ld: %s\n", ++lcount, index_word);
        }

        printf("\nAlthough %ld entries were listed, as this is a raw dump of the index,\n", lcount);
        puts("the output will contain a number of duplicates.");
    
        rewind(fp1);     
        
        return true;
    }
    
    return false;
}




static void about(void)
{
    char timeBuff[sizeof(__TIME__) + 1];

    // Get rid of the seconds info.
    //    
    sprintf(timeBuff, "%s", __TIME__);
    //
    // Put a null terminator at the second colon position.
    //
    timeBuff[5] = '\0';
    
    puts("");
    puts("  ============================================================================");
    puts("    See http://www.eat.rl.ac.uk/");
    puts("");
    puts("    Modified the original source at the URL - mainly to add the -sw -t -f -n");
    puts("    options for my own use.\n\n\tpeetm - peet.morris@comlab.ox.ac.uk");
    puts("");
    printf("\t\t\tLast built: %s at %s", __DATE__, timeBuff);    
    puts("");
    puts("  ============================================================================");
    puts("");
    puts("  Some 'option' examples:");
    puts("");    
    puts("    eatshow -n          ... numbers the results");
    puts("");    
    puts("    eatshow -ftest.txt  ... echo output to the file test.txt: file is appended");
    puts("                            to if it exists, or created anew if it does not");
    puts("");    
    puts("    eatshow man -sw man ... starts eatshow in non-interactive mode: first");
    puts("                            outputs the responses when 'man' was used as a");
    puts("                            stimuli, then toggles (-sw) eatshow into response");
    puts("                            mode, in which the output lists words which were");
    puts("                            used as a stimulus, and where 'man' was a response");
    puts("");
    puts("    eatshow -r -t       ... starts eatshow in response mode, and instructs");
    puts("                            it to use tabs instead of spaces in its layouts");    
    puts("");    

    exit(1);    
}




// print proper usage and exit 
//
static void usage(void)
{                               
    puts("Usage: eatshow [-a -d -f -i -n -r -s -sw -t -x -?] [word_list]");
    puts("Find associates to words in the Edinburgh Associative Thesaurus");
    puts("");    
    puts("Outputs:");
    puts("\tThe total number of different answers, the count of");
    puts("\tall answers,  and  the list of triads of associated");
    puts("\ttypes, their individual frequencies, and proportion");
    puts("\tof occurrence.   The proportion of occurrence for a");
    puts("\tgiven type is its individual  frequency  divided by");
    puts("\tthe total count of all answers.");
    puts("");    
    puts("Switches:");
    puts("\t-a \t further info about this application");
    puts("\t-d \t turn off results demarcation");    
    puts("\t-f<file> echo screen output to a file");
    puts("\t-i<file> reads/processes input from a file a line at a time");
    puts("\t-l<n>\t limits the number of outputs to <n>");
    puts("\t-n \t number outputs");
    puts("\t-r \t use cue as response");
    puts("\t-s \t use cue as stimulus(default)");
    puts("\t-sw\t toggles the -r/-s mode [without restart] (runtime switch)");
    puts("\t-t \t tab-delimit output [default is to use spaces]");
    puts("\t-x \t dumps the index wordlist for the current mode (runtime switch)");
    puts("\t-? \t display these options");
    puts("");    
    puts("NOTE: If -i or a word_list is used, eatshow does not enter interactive mode");
    puts("");
    puts("In interactive mode, to return to the command prompt, simply hit return");
    puts("(do not enter a word). Alternately, enter Ctrl + Z");
    
    exit(1);
}



// Trims a LF off a string by inserting a '\0' in the location
// of the first LF found.
//
static void trimLF(char * string)
{
    auto char * p;
    
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
    auto char * p;
    
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
    
    switch(*f++)
    {
        case 's':                   // read s-r file.
            sourcef = 's';
            break;
            
        case 'r':
            sourcef = 'r';          // read r-s file.
            break;

        case 'f':                   // echo to file.
            if(*f != '\0')
            {
                echoFile = f;
            }
            break;
                        
        case 't':                   // tab output.
            bTabPad = true;
            break;
    
        case 'a':                   // show 'about'.
            about();
            break;
            
        case 'd':                   // add demarcation lines.
            bDemark = false;
            break;

        case 'n':                   // number outputs.
            bNumber = true;
            break;
            
        case 'l':                   // limit the number of outputs.
            if(*f != '\0')
            {
                bLimit  = true;
                nLimit  = atoi(f);
                bNumber = true;
            }
            break;

        case '?':                   // show usage.
            usage();
            break;
            
        case 'i':                   // read input from file.
            if(*f != '\0')
            {
                fromFile(f);
                exit(0);            // close here.
            }
            break;
            
        case NULL:                  // error.
            usage();
            exit(1);
            break;
            
        default:
            usage();
            exit(1);
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
        auto char * argv[10];
        auto int    argc = 0;        
        
        auto char   buffer[255];
        auto char * p = NULL;

        auto int    n = 0;

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
                // is there a lf or a cr on a seperate line?
                //
			    if(strlen(buffer) == 1)
                {
                    // Yes, ignore.
                    //
                    continue;
                }
            
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
        printf("Error opening %s\n", s);
    }
    
    return;
}



// Put app into default state.  Mostly used when we're reading directives from a file
// using the -i option.
//
static void resetFlags(void)
{
    echoFile = NULL;
    inptFile = NULL;
    bTabPad  = false;
    bDemark  = true;
    bNumber  = false;
    bLimit   = false;
    nLimit   = 0;
    sourcef  = 's';
    
    return;
}